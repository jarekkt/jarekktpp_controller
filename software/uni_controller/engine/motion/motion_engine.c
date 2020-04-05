/*!
    \file motion_engine.c

    \brief
*/


#include "system.h"
#include "services.h"
#include "middleware.h"
#include "engine.h"

#include "motion_engine.h"

typedef enum
{
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
	AXIS_CNT
}axis_e;




typedef struct
{
	int32_t pos_step_pulses;
	int32_t pos_encoder_pulses;

	int32_t pos_001mm;
	int32_t speed_001mm;
	int32_t accel_001mm;
	int32_t jerk_001mm;



}motion_axis_t;


typedef struct
{
	uint32_t  enc_10000pulses2um;
	uint32_t  step_10000pulses2um;
	uint32_t  safe_speed_001mm_s;

	uint32_t  endpos_min_mask;
	uint32_t  endpos_max_mask;

}motion_axis_cfg_t;





typedef struct
{
	motion_axis_t 		axis[AXIS_CNT];

}motion_ctx_t;


typedef struct
{
	uint32_t 			step_freq;
	uint32_t			estop_mask;
	motion_axis_cfg_t   axis_cfg[AXIS_CNT];
}motion_nv_data_t;


motion_ctx_t 	  mctx;
motion_nv_data_t  mctx_nv VAR_NV_ATTR;


void motion_engine_init_default(void)
{
	memset(&mctx_nv,0,sizeof(mctx_nv));
}

void motion_engine_init(void)
{
	memset(&mctx,0,sizeof(mctx));

	srv_nov_register(&mctx_nv,sizeof(mctx_nv),motion_engine_init_default);
}


void motion_engine_once(void)
{

}

void motion_engine_run(uint32_t axis_idx,int32_t dist_001mm,int32_t speed_001mm_s,int32_t accel_001mm_s2,int32_t jerk_001mm_s3)
{

}

uint32_t motion_engine_run_status(void)
{
	uint32_t result = 0;

	return result;
}



