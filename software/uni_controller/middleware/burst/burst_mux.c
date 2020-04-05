/*!
    \file burst_mux.c

    \brief
*/


#include "system.h"
#include "services.h"
#include "middleware.h"
#include "burst_mux.h"



typedef struct
{
	uint32_t			address;
}burst_mux_t;

const char        hexd[] = "0123456789ABCDEF";

static void burst_mux_task(void * params);

burst_mux_t  bmux;


void burst_mux_init()
{
	memset(&bmux,0,sizeof(bmux));

	// Selects address pair - for RS485 channels
	bmux.address  = srv_gpio_get_address() << 1;
}










void burst_mux_serial_process(uint32_t idx,char * buffer,uint32_t len)
{
	burst_rcv_ctx_t		rcv_ctx;

	buffer[len] = 0;

	char * fstart;
	char * fend;

	rcv_ctx.channel	= idx;

	fstart = strch(buffer,'<');
	fend = strch(buffer,'>');

	if( (fstart != NULL) && (fend != NULL))
	{
		// Got standard encapsulated frame
		rcv_ctx.frame_format = RCV_FRAME_ENCAPSULATED;
	}
	else
	{
		// Try for pure gcode - direct message
		rcv_ctx.frame_format = RCV_FRAME_DIRECT;
		gcode_engine_command(buffer,&rcv_ctx);
	}

}





/*!
        \brief Function which calculates response frame crc and sends it out.

*/
static void burst_mux_send_response(uint32_t ch_idx,char * response, int length)
{
    int                   ii;
    uint8_t           crc = 0;



    if(length >=6 )
    {
       // Prepare message with CRC
       for(ii = 1;ii < length-6;ii++)
       {
           crc ^= response[ii];
       }

       response[length-5] = hexd[crc>>4];
       response[length-4] = hexd[crc&0x0F];

       switch(bmux.ch[ch_idx].type)
       {
			case CH_MASTER:
			case CH_SLAVE:
			{
				srv_serial_485_send(bmux.ch[ch_idx].serial_id,response,length);
			}break;

			case CH_DEBUG:
			{
				srv_serial_send(bmux.ch[ch_idx].serial_id,response,length);
			}break;
       }
    }

}


/*!
        \brief Function which processes messages sent from PC ( low-level frame processing)

*/
int burst_mux_execute_var(uint32_t ch_idx,char * command, int length)
{

    char            * var_name;
    char              var_name_addr[4];

    char            * var_value;
    char            * var_crc;
    char            * pBreake = 0;

    int32_t           ii;
    int32_t 		  var_name_len;
    char              resp_buffer[64];
    char              resp_value[64];
    int32_t           resp_len;
    char              crc;
    char              crc_calc;


    uint32_t 		  execute_store;
    int32_t   		  our_address;
    int32_t   		  msg_address;



    command[length] = 0;

#define TO_VAL(b_)  (  (((b_)>= '0')&&((b_)<='9')) ? (b_) - '0' : (toupper(b_)-'A'+10)  )

    /*
    	Check  crc
    */

    pBreake = strchr(command, ':');
    if(pBreake == NULL)
    {
        // Ignore frames without crc
        return execute_store;
    }
    else
    {
        length       = pBreake-command; // remove crc count from length
        *pBreake     = '\0';
        var_crc      = pBreake + 1;

        if(strcmp(var_crc,"$") != 0)
        {
            // No magic crc bypass
            crc = (char)strtol(var_crc,NULL,16);
            ii                 = 0;
            crc_calc = 0;
            while(command[ii] != 0)
            {
                    crc_calc = crc_calc ^ command[ii++];
            }

            if(crc != crc_calc)
            {
                // Ignore frames with wrong CRC
                return execute_store;
            }
        }
    }


	/*
			R/W request
	*/

	/*
			Find separator '='
	*/

	pBreake = strchr(command, '=');
	if(pBreake == NULL)
	{
			/*
					Read request
			 */
			var_name        = command;
			var_name_len    = length;
			var_value       = NULL;
	}
	else
	{
			/*
					Write request. Extract parameter name and value.
			*/
			var_name        = command;
			var_name_len    = pBreake-command;
			*pBreake        = '\0';
			var_value       = pBreake + 1;
	}



	switch(bmux.ch[ch_idx].type)
	{
		case CH_MASTER:
		{
			// Response to our request
			// Addresses already in the message
			our_address = -1;
		}break;

		case CH_SLAVE:
		{
			// Send our response with address
			our_address = srv_gpio_get_address();


			var_name_addr[0] = var_name[0];
			var_name_addr[1] = var_name[1];
			var_name_addr[2] = 0;

			var_name 		+=2;
			var_name_len	-=2;

			msg_address = strtol(var_name_addr,NULL,16);
			if(msg_address != our_address)
			{
				// Not for us
				return 0;
			}

		}break;

		case CH_DEBUG:
		{
			// Send our response without address
			our_address = -1;
		}break;
	}



	if(burst_process_variable(var_name,var_name_len,var_value,resp_value,sizeof(resp_value),&execute_store) > 0)
	{
		switch(bmux.ch[ch_idx].type)
		{
			case CH_MASTER:
			{
				// No response here
			}break;

			case CH_SLAVE:
			{
				// Send our response with address
				resp_len = snprintf(resp_buffer,sizeof(resp_buffer),"<%02lX%s=%s:$$>\r\n",our_address,var_name,resp_value);
				burst_mux_send_response(ch_idx,resp_buffer,resp_len);
			}break;

			case CH_DEBUG:
			{
				// Send our response without address
				resp_len = snprintf(resp_buffer,sizeof(resp_buffer),"<%s=%s:$$>\r\n",var_name,resp_value);
				burst_mux_send_response(ch_idx,resp_buffer,resp_len);
			}break;
		}

	}

    return execute_store;

}


