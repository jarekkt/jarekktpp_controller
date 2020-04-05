#ifndef MOTION_ENGINE_H
#define MOTION_ENGINE_H
 
#include "system.h"

void motion_engine_init(void);
void motion_engine_once(void);

void motion_engine_run(uint32_t axis_idx,int32_t dist_001mm,int32_t speed_001mm_s,int32_t accel_001mm_s2,int32_t jerk_001mm_s3);
uint32_t motion_engine_run_status(void);




#endif // MOTION_ENGINE_H
