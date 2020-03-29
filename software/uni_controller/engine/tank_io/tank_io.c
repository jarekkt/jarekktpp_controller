#include "services.h"
#include "middleware.h"
#include "tank_io.h"


#define   TANK_IO_PERIOD    200 /*ms*/
#define   TANK_IO_FILTER   1000 /*ms*/

typedef struct
{
	uint32_t new_val;
	uint32_t new_read;
	uint32_t timeout;
	uint32_t tank_io_state;
}tank_io_t;


typedef struct
{
	uint32_t tank_io_polarity;
}tank_io_nv_t;


tank_io_t 		tank_io;
tank_io_nv_t 	tank_io_nv;


static void tank_io_eng_thread(void);

const var_ptable_t   tank_io_var_ptable[] SERMON_ATTR =
{
  { "tank_io",              &tank_io.tank_io_state,                   E_VA_UINT_FREE   },
  { "tank_io_polarity",     &tank_io_nv.tank_io_polarity,             E_VA_UINT_FREE   },
};

void tank_io_init_default(void)
{
	memset(&tank_io_nv,0,sizeof(tank_io_nv));

}


void tank_io_init(void)
{

    memset(&tank_io,0,sizeof(tank_io));

    srv_sermon_register(tank_io_var_ptable,DIM(tank_io_var_ptable));
    srv_nov_register(&tank_io_nv, sizeof(tank_io_nv),tank_io_init_default);

}

void tank_io_once(void)
{
    mwr_periodic_low_register(tank_io_eng_thread,TANK_IO_PERIOD);
}


static void tank_io_eng_thread(void)
{
	tank_io.new_read = 0;
/*
	if(GPIO_Get(IN_TANK1)!=0)tank_io.new_read |= 0x01;
	if(GPIO_Get(IN_TANK2)!=0)tank_io.new_read |= 0x02;
	if(GPIO_Get(IN_TANK3)!=0)tank_io.new_read |= 0x04;
	if(GPIO_Get(IN_TANK4)!=0)tank_io.new_read |= 0x08;


	tank_io.new_val = tank_io.new_read ^ tank_io_nv.tank_io_polarity;

	if(tank_io.tank_io_state != tank_io.new_val)
	{
		if(tank_io.timeout < (TANK_IO_FILTER/TANK_IO_PERIOD))
		{
			tank_io.timeout++;
		}
		else
		{
			tank_io.tank_io_state = tank_io.new_val;
		}
	}
	else
	{
		tank_io.timeout = 0;
	}

	if(tank_io.tank_io_state & 0x01)
	{
		GPIO_Set(OUT_PUMP1);
	}
	else
	{
		GPIO_Clr(OUT_PUMP1);
	}

*/


}
