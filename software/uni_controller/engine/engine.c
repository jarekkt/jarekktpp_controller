#include "system.h" 
#include "common.h" 
#include "engine.h" 


void engine_init(void)
{
	gcode_engine_init();
	motion_engine_init();
}

void engine_once(void)
{ 
	gcode_engine_once();
	motion_engine_once();
}
