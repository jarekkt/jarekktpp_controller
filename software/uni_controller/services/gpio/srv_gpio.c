#include "system.h"
#include "services.h"
#include "gpio_srv.h"


volatile int32_t gpio_test_mode = 0;

typedef struct
{
	uint32_t  address;
}gpio_data_t;


gpio_data_t gpio;

void      srv_gpio_init(void)
{
	uint32_t val = 0;


	memset(&gpio,0,sizeof(gpio));

	if(GPIO_Get(ADDR0)!=0)val |= 0x01;
	if(GPIO_Get(ADDR1)!=0)val |= 0x02;
	if(GPIO_Get(ADDR2)!=0)val |= 0x04;
	if(GPIO_Get(ADDR3)!=0)val |= 0x08;

	gpio.address = val;

}

void  srv_gpio_once(void)
{

}

uint32_t  srv_gpio_get_address(void)
{
	return gpio.address;
}
