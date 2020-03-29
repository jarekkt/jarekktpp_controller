/*!
    \file burst_mux.c

    \brief
*/


#include "system.h"
#include "services.h"
#include "middleware.h"

#include "version.h"


#define BURST_MUX_TIMEOUT 50

typedef enum
{
	CH_MASTER,
	CH_SLAVE,
	CH_DEBUG
}ch_type_e;


typedef struct
{

    uint8_t  	RxBuffer[128];
    uint32_t 	Timestamp;

    uint32_t 	timeout;

    int      	RxCnt;
    int      	RxMsgCounter;
    int      	RxMsgOk;

    ch_type_e 	type;
    uint32_t    serial_id;

}burst_serial_data_t;

typedef struct
{
	burst_serial_data_t  ch[3];

	xSemaphoreHandle    sema_recv;
}burst_mux_t;

const char        hexd[] = "0123456789ABCDEF";

static void burst_mux_task(void * params);

burst_mux_t  bmux;


void burst_mux_init()
{
	memset(&bmux,0,sizeof(bmux));

}


static void burst_mux_cc(uint8_t cc,uint32_t idx,portBASE_TYPE * woken)
{
   uint32_t    				timestamp;
   burst_serial_data_t	  * serial_ch;

   timestamp   = HAL_GetTick();
   serial_ch   = &bmux.ch[idx];

   if(serial_ch->RxMsgOk == 0)
   {

      /*!
          \req    <b>Debug message character timeout</b>
                  There is 2seconds inter-charater message timeout.
                  This should allow entering the message manually, but should prevent TX signal distrubances from forming
                  valid message over time.

      */
      if( (timestamp - serial_ch->Timestamp) > serial_ch->timeout )
      {
          serial_ch->RxCnt  = 0;
      }

      serial_ch->Timestamp = timestamp;


      if(cc == '<')
      {
          serial_ch->RxCnt = 0;
          serial_ch->RxBuffer[serial_ch->RxCnt++] = cc;
      }
      else
      {
         if(serial_ch->RxCnt < sizeof(serial_ch->RxBuffer))
         {
              serial_ch->RxBuffer[serial_ch->RxCnt++] = cc;

              if(cc == '>')
              {
                  serial_ch->RxMsgCounter++;
                  serial_ch->RxMsgOk = 1;

                  xSemaphoreGiveFromISR(bmux.sema_recv,woken);
              }
         }
         else
         {
              serial_ch->RxCnt = 0;
         }
      }
   }
}


static void burst_mux_rcv_cc(uint32_t portId,uint8_t cc,portBASE_TYPE * woken)
{
	burst_mux_cc(cc,0,woken);
}

static void burst_mux_rcv_cc_pc(uint32_t portId,uint8_t cc,portBASE_TYPE * woken)
{
	burst_mux_cc(cc,1,woken);
}

static void burst_mux_rcv_cc_plc(uint32_t portId,uint8_t cc,portBASE_TYPE * woken)
{
	burst_mux_cc(cc,2,woken);
}


void burst_mux_once()
{
	vSemaphoreCreateBinary(bmux.sema_recv);

	bmux.ch[0].timeout 		= 2000;
	bmux.ch[0].type    		= CH_DEBUG;
	bmux.ch[0].serial_id	= SRV_SERIAL_DEBUG;
	srv_serial_rcv_callback(SRV_SERIAL_DEBUG,burst_mux_rcv_cc);

	bmux.ch[1].timeout 		= 10000;
	bmux.ch[1].type    		= CH_SLAVE;
	bmux.ch[1].serial_id	= SRV_SERIAL_PC_CHANNEL;
	srv_serial_485_rcv_callback(SRV_SERIAL_PC_CHANNEL,burst_mux_rcv_cc_pc);
	srv_serial_485_enable(SRV_SERIAL_PC_CHANNEL,1);

	bmux.ch[2].timeout 		= 10000;
	if(srv_gpio_get_address() == 0)
	{
		bmux.ch[2].type    		= CH_MASTER;
	}
	else
	{
		bmux.ch[2].type    		= CH_SLAVE;
	}
	bmux.ch[2].serial_id	= SRV_SERIAL_PLC_CHANNEL;
	srv_serial_485_rcv_callback(SRV_SERIAL_PLC_CHANNEL,burst_mux_rcv_cc_plc);
	srv_serial_485_enable(SRV_SERIAL_PLC_CHANNEL,1);





	xTaskCreate( burst_mux_task, "Burst", 6 * configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY   + 1, NULL );
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
				// Not for usb
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


static void burst_mux_serial_process(uint32_t idx)
{
	uint32_t execute_store;

	if(bmux.ch[idx].RxCnt >= 6)
	{
        execute_store = burst_mux_execute_var(idx,&bmux.ch[idx].RxBuffer[1],bmux.ch[idx].RxCnt-2);

		if((bmux.ch[idx].type == CH_DEBUG) && (execute_store!=0))
		{
			tsk_storage_activate();
		}
	}

}


static void burst_mux_serial_rcv(void)
{
    uint32_t cnt= 0;
    uint32_t ii;


    xSemaphoreTake(bmux.sema_recv,BURST_MUX_TIMEOUT/portTICK_RATE_MS);

    do
    {
		cnt = 0;
		for(ii=0;ii<3;ii++)
		{
			if(bmux.ch[ii].RxMsgOk != 0)
			{
				burst_mux_serial_process(ii);
				cnt++;
			}
			bmux.ch[ii].RxCnt 	= 0;
			bmux.ch[ii].RxMsgOk = 0;
		}
    }while (cnt != 0);

}
/*!
        \brief Burst task function. Sends periodically burst frames ( if enabled by PC message).

*/

static void burst_mux_task(void * params)
{
    while(1)
    {

    	burst_mux_serial_rcv();

           /*!
               \sto  Storage activation after debug port serial command operation.
                     There is no writing when variable was only read ( only variable write commands trigger storage by default).
                     Note that this may trigger NOV update even when serial command actually
                     changed nothing, but there were pending writes to NOV ( waiting for its 7 minute period)

           */



        fw_stack_check();
    }
}
