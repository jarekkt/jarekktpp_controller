#include "iotest_io.h"
#include "services.h"
#include "middleware.h"


typedef struct
{
	uint32_t io_request;
	uint32_t io_special;
}iot_io_t;


iot_io_t iot_io;

const var_ptable_t   iotest_io_var_ptable[] SERMON_ATTR =  
{
  { "io_request",         &iot_io.io_request  ,       E_VA_UINT_FREE        },
  { "io_special",         &iot_io.io_special  ,       E_VA_UINT_FREE        },
};


void mwr_iotest_io_init()
{
    memset(&iot_io,0,sizeof(iot_io));

    srv_sermon_register(iotest_io_var_ptable,sizeof(iotest_io_var_ptable));
}



static void iotest_io_special(uint32_t io_special)
{
    if( (io_special & 0x01) != 0)
    {
  	  // Reset request
    }





    if( (io_special & 0x10) != 0)
    {
    	srv_serial_485_send(SRV_SERIAL_RS485_1,"PC_PORT1\r\n",0);
    }

    if( (io_special & 0x20) != 0)
    {
    	srv_serial_485_send(SRV_SERIAL_RS485_2,"PLC_PORT2\r\n",0);
    }


}


void iotest_io_control(uint32_t io_request)
{

      if( (io_request & 0x01) != 0)
      {
    	  gpio_test_mode = 1;
      }
      else if( (io_request & 0x02) != 0 )
      {
    	  gpio_test_mode = 0;
      }

#if 0
      if( (io_request & 0x04) != 0)
      {
    	  GPIO_Set(OUT_PUMP1);
      }
      else if( (io_request & 0x08) != 0 )
      {
    	  GPIO_Clr(OUT_PUMP1);
      }

      if( (io_request & 0x10) != 0)
      {
    	  GPIO_Set(OUT_PUMP2);
      }
      else if( (io_request & 0x20) != 0 )
      {
    	  GPIO_Clr(OUT_PUMP2);
      }

      if( (io_request & 0x40) != 0)
      {
    	  GPIO_Set(OUT_PUMP3);
      }
      else if( (io_request & 0x80) != 0 )
      {
    	  GPIO_Clr(OUT_PUMP3);
      }

      if( (io_request & 0x100) != 0)
      {
     	  GPIO_Set(OUT_PUMP4);
      }
      else if( (io_request & 0x200) != 0 )
      {
     	  GPIO_Clr(OUT_PUMP4);
      }
#endif
}




void mwr_iotest_io_task(void)
{

    if(iot_io.io_request != 0)
    {
      iotest_io_control(iot_io.io_request);
      iot_io.io_request = 0;
    }

    if(iot_io.io_special != 0)
    {
      iotest_io_special(iot_io.io_special);
      iot_io.io_special = 0;
    }

}


