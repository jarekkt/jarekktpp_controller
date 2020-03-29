#include "adc_srv.h"





volatile uint16_t adc_vars[ADC_CH_CNT];

extern ADC_HandleTypeDef hadc1;


void      srv_adc_init(void)
{
  // if(HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_vars, 5) != HAL_OK)
   //{
	// /* Start Error */
	// fw_assert(0);
   //}
}

void      srv_adc_once(void)
{

}


uint32_t  srv_adc_get(uint32_t ch)
{
    if(ch < ADC_CH_CNT)
    {
        return (uint32_t)adc_vars[ch];
    }
    else
    {
        return 0;
    }

}

uint32_t srv_adc_dump(int mode,char * buffer, uint32_t buffer_size)
{
  uint32_t length;

  length = snprintf(buffer,buffer_size,"ADC: T1 %4d T2 %4d T3 %d T4 %4d  PRE %4d \r\n",
      adc_vars[ADC_CH_TANK1],
      adc_vars[ADC_CH_TANK1],
      adc_vars[ADC_CH_TANK1],
      adc_vars[ADC_CH_TANK1],
      adc_vars[ADC_CH_PREASURE]
     );


  return length;
}
