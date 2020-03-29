#include "system.h" 
#include "common.h" 
#include "engine.h" 


void engine_init(void)
{
	tank_io_init();
}

void engine_once(void)
{ 
	tank_io_once();
}
