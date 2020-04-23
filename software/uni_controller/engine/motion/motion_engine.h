#ifndef MOTION_ENGINE_H
#define MOTION_ENGINE_H
 
#include "system.h"

void motion_engine_init(void);
void motion_engine_once(void);


void motion_engine_run(uint32_t axis_idx,int32_t dist_001mm,int32_t speed_001mm_s);
void motion_engine_run_home(uint32_t axis_idx,int32_t speed_001mm_s);
void motion_engine_stop(uint32_t axis_mask,uint32_t abort_mask);
uint32_t motion_engine_run_status(void);




#endif // MOTION_ENGINE_H
