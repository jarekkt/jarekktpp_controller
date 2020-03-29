#ifndef ADC_SRV_H
#define ADC_SRV_H

/*! \file   ASC_srv.h

    \brief  Header for analog output driver
    
    
*/

#include "system.h"

typedef enum
{
  ADC_CH_OUT = 0,
  ADC_CH_TANK1,
  ADC_CH_TANK2,
  
  ADC_CH_TANK3,
  ADC_CH_TANK4,
  ADC_CH_PREASURE,


  ADC_CH_CNT
}adc_ch_e;


void      srv_adc_init(void);
void      srv_adc_once(void);
uint32_t  srv_adc_get(uint32_t ch);
uint32_t  srv_adc_dump(int mode,char * buffer, uint32_t buffer_size);



#endif // DAC_SRV_H
 
