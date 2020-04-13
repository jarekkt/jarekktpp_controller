/*!
    \file gcode_engine.c

    \brief
*/


#include "system.h"
#include "services.h"
#include "middleware.h"
#include "engine.h"

#include "gcode_parser.h"




typedef struct
{
	int32_t		  is_inch;
}gcode_ctx_t;


typedef struct
{
	gcode_ctx_t  gcx;
}gcode_data_t;


typedef struct
{
	int32_t dummy;
}gcode_nv_data_t;


gcode_data_t     gcd;
gcode_nv_data_t  gcd_nv VAR_NV_ATTR;


void gcode_engine_init_default(void)
{
	memset(&gcd_nv,0,sizeof(gcd_nv));
	memset(&gcd,0,sizeof(gcd));
}

void gcode_engine_init(void)
{
	gcode_parser_init();

	memset(&gcd,0,sizeof(gcd));

	srv_nov_register(&gcd_nv,sizeof(gcd_nv),gcode_engine_init_default);
}


void gcode_engine_once(void)
{

}


void gcode_engine_command(const char * cmd_line, const void * rcv_ctx)
{

}



