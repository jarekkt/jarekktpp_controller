#ifndef MOTION_ENGINE_H
#define MOTION_ENGINE_H
 
#include "system.h"

typedef enum
{
	MF_START_CONCAVE,
	MF_START_LINEAR,
	MF_START_CONVEX,
	MF_CONSTANT_SPEED,
	MF_STOP_CONVEX,
	MF_STOP_LINEAR,
	MF_STOP_CONCAVE,
	MF_PHASES_CNT
}motion_phases_e;

typedef struct
{
	uint32_t    pulse_count;

	uint32_t	speed_fract;
	int32_t	    accel_fract;
	int32_t		jerk_fract;
}motion_phase_t;

typedef struct
{
	uint32_t			accu;
	motion_phases_e		phase;
	motion_phase_t  	mf[MF_PHASES_CNT];
}motion_timer_t;




void motion_engine_init(void);
void motion_engine_once(void);


void motion_engine_run(uint32_t axis_idx,int32_t dist_001mm,int32_t speed_001mm_s);
void motion_engine_run_home(uint32_t axis_idx,int32_t speed_001mm_s);
void motion_engine_stop(uint32_t axis_mask,uint32_t abort_mask);
uint32_t motion_engine_run_status(void);




#endif // MOTION_ENGINE_H
