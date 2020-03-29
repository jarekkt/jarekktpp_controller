/*! 
    \file   timer_srv.h

    \brief  File with support for timers
    
    
*/


#ifndef TIMER_SRV_H
#define TIMER_SRV_H

#define TIMER_SRV_SLOW_HZ    1000    /* Hz */
#define TIMER_SRV_FAST_HZ    10000   /* Hz */
#define TIMER_SRV_1MHZ       1000000 /* Hz */


#include "system.h"



typedef void  ( /* ATTRIBUTE_IN_RAM must be in the target function */ *timer_callback_fn)(void);

void srv_timer_init(void);
void srv_timer_once(void);


void srv_udelay(uint32_t cnt);



// For slow callbacks, no FreeRTOS support
// NOTE: This functions must be in RAM 
//       and do not touch FLASH base code
void srv_timer_callback_slow_add(timer_callback_fn fn);

// For fast callbacks, no FreeRTOS support
// NOTE: This functions must be in RAM 
//       and do not touch FLASH base code
void srv_timer_callback_fast_add(timer_callback_fn fn);


typedef uint16_t srv_hwio_timestamp_t; 

void     srv_hwio_delay_ms(uint32_t ms);
void     srv_hwio_delay_us(uint32_t us);
uint32_t             srv_hwio_timestamp_ms_tick(void);
srv_hwio_timestamp_t srv_hwio_timestamp_ms(void);
srv_hwio_timestamp_t srv_hwio_timestamp_us(void);
uint32_t srv_hwio_timestamp_diff( srv_hwio_timestamp_t t_new,srv_hwio_timestamp_t t_old);
void     srv_hwio_process_timers_when_irq_disable(void);







#endif //TIMER_SRV s
