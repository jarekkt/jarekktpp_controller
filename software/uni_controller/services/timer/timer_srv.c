#include "services.h"
#include "timer_srv.h"


#define TIMER_CALLBACK_CNT 2


typedef struct
{
  TIM_HandleTypeDef  hTim5;
  TIM_HandleTypeDef  hTim4;
  TIM_HandleTypeDef  hTim3;
  TIM_HandleTypeDef  hTim2;


  timer_callback_fn  time_fast_cb[TIMER_CALLBACK_CNT];
  uint32_t           time_fast_cnt;   

  timer_callback_fn  time_slow_cb[TIMER_CALLBACK_CNT];
  uint32_t           time_slow_cnt;


  uint32_t           tim_clock;

  uint32_t           tim_tick;
}timer_srv_t;


timer_srv_t tsrv;



void ATTRIBUTE_IN_RAM TIM2_IRQHandler()  
{
  int ii;


  tsrv.tim_tick++;

  for(ii = 0; ii < tsrv.time_slow_cnt;ii++)
  {
    tsrv.time_slow_cb[ii]();
  }

  __HAL_TIM_CLEAR_IT(&tsrv.hTim2, TIM_IT_UPDATE);
  

}

void ATTRIBUTE_IN_RAM TIM3_IRQHandler() 
{
  int ii;

  for(ii = 0; ii < tsrv.time_fast_cnt;ii++)
  {
    tsrv.time_fast_cb[ii]();
  }

  __HAL_TIM_CLEAR_IT(&tsrv.hTim3, TIM_IT_UPDATE);
}

void  ATTRIBUTE_IN_RAM   srv_hwio_process_timers_when_irq_disable(void)
{
  if(__HAL_TIM_GET_FLAG(&tsrv.hTim2,TIM_IT_UPDATE)!= 0)
  {
      TIM2_IRQHandler();
  }

  if(__HAL_TIM_GET_FLAG(&tsrv.hTim3,TIM_IT_UPDATE)!= 0)
  {
      TIM3_IRQHandler();
  }
}

void srv_timer_once(void)
{

}



void srv_timer_callback_slow_add(timer_callback_fn fn)
{
  if(tsrv.time_slow_cnt < TIMER_CALLBACK_CNT)
  {
    tsrv.time_slow_cb[tsrv.time_slow_cnt] = fn;
    tsrv.time_slow_cnt++;
  }
  

}

void srv_timer_callback_fast_add(timer_callback_fn fn )
{
  if(tsrv.time_fast_cnt < TIMER_CALLBACK_CNT)
  {
    tsrv.time_fast_cb[tsrv.time_fast_cnt] = fn;
    tsrv.time_fast_cnt++;
  }
}



void srv_timer_init(void)
{
    memset(&tsrv,0,sizeof(tsrv));

    tsrv.tim_clock = SystemCoreClock / 4;


    /* Free running hardware tick counter  - 1ms */

    __HAL_RCC_TIM4_CLK_ENABLE();

    tsrv.hTim4.Instance           = TIM4;
    tsrv.hTim4.Init.Prescaler     = (tsrv.tim_clock/ 1000) - 1;
    tsrv.hTim4.Init.CounterMode   = TIM_COUNTERMODE_UP;
    tsrv.hTim4.Init.Period        = -1;
    tsrv.hTim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
    HAL_TIM_Base_Init(&tsrv.hTim4);

    HAL_TIM_Base_Start(&tsrv.hTim4); // Trying to start the base counter



   /* Free running hardware tick counter  - 1us */

    __HAL_RCC_TIM5_CLK_ENABLE();

    tsrv.hTim5.Instance           = TIM5;
    tsrv.hTim5.Init.Prescaler     = (tsrv.tim_clock/ 1000000) - 1;
    tsrv.hTim5.Init.CounterMode   = TIM_COUNTERMODE_UP;
    tsrv.hTim5.Init.Period        = -1;
    tsrv.hTim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
    HAL_TIM_Base_Init(&tsrv.hTim5);

    HAL_TIM_Base_Start(&tsrv.hTim5); // Trying to start the fast base counter



    /* Service callback timer - slow ( 1kHz) */

    __HAL_RCC_TIM2_CLK_ENABLE();

    tsrv.hTim2.Instance           = TIM2;
    tsrv.hTim2.Init.Prescaler     = (tsrv.tim_clock/ TIMER_SRV_1MHZ) - 1;
    tsrv.hTim2.Init.CounterMode   = TIM_COUNTERMODE_UP;
    tsrv.hTim2.Init.Period        = TIMER_SRV_1MHZ / TIMER_SRV_SLOW_HZ;
    tsrv.hTim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
    HAL_TIM_Base_Init(&tsrv.hTim2);

    HAL_TIM_Base_Start_IT(&tsrv.hTim2); 

    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);


    /* Service callback timer - fast ( 10 kHz) */

    __HAL_RCC_TIM3_CLK_ENABLE();

    tsrv.hTim3.Instance           = TIM3;
    tsrv.hTim3.Init.Prescaler     = (tsrv.tim_clock/ TIMER_SRV_1MHZ) - 1;
    tsrv.hTim3.Init.CounterMode   = TIM_COUNTERMODE_UP;
    tsrv.hTim3.Init.Period        = TIMER_SRV_1MHZ / TIMER_SRV_FAST_HZ;
    tsrv.hTim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
    HAL_TIM_Base_Init(&tsrv.hTim3);

    HAL_TIM_Base_Start_IT(&tsrv.hTim3); 


    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);



}



void ATTRIBUTE_IN_RAM srv_hwio_delay_ms(uint32_t ms) 
{
   uint32_t  tick;
   uint32_t  diff;

   tick = __HAL_TIM_GET_COUNTER(&tsrv.hTim4);

   do
   {
      diff = __HAL_TIM_GET_COUNTER(&tsrv.hTim4) - tick;
   }while( diff < ms);
}

void ATTRIBUTE_IN_RAM srv_hwio_delay_us(uint32_t us) 
{
   uint32_t  tick;
   uint32_t  diff;

   tick = __HAL_TIM_GET_COUNTER(&tsrv.hTim5);

   do
   {
      diff = __HAL_TIM_GET_COUNTER(&tsrv.hTim5) - tick;
   }while( diff < us);
}

__INLINE uint32_t ATTRIBUTE_IN_RAM srv_hwio_timestamp_ms_tick(void)
{
  return tsrv.tim_tick;
}


__INLINE srv_hwio_timestamp_t ATTRIBUTE_IN_RAM srv_hwio_timestamp_ms(void)
{
  return __HAL_TIM_GET_COUNTER(&tsrv.hTim4);
}

__INLINE srv_hwio_timestamp_t ATTRIBUTE_IN_RAM srv_hwio_timestamp_us(void)
{
  return __HAL_TIM_GET_COUNTER(&tsrv.hTim5);
}


__INLINE uint32_t ATTRIBUTE_IN_RAM srv_hwio_timestamp_diff(srv_hwio_timestamp_t t_new,srv_hwio_timestamp_t t_old)
{
    return (uint32_t)((uint16_t)(t_new-t_old));
}


__INLINE void ATTRIBUTE_IN_RAM srv_udelay(uint32_t cnt)
{
  volatile int ii;

  do
  {  
    __DSB();
    if(cnt == 0)
    {
      break;
    }

    for (ii = 0; ii < 100;ii++)
    {
      __NOP();
    };
   
    cnt --;
  }while(1);

}
