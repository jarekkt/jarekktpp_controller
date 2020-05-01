/*!
    \file motion_engine.c

    \brief
*/


#include "system.h"
#include "services.h"
#include "middleware.h"
#include "engine.h"

#include "motion_engine.h"




typedef struct
{
	uint32_t 			mode;
	uint32_t			mt_active;
	motion_timer_t		mt[AXIS_CNT];
}motion_ctx_t;

typedef struct
{
	uint32_t 			step_freq;

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

void motion_engine_run_home(uint32_t axis_idx,int32_t speed_001mm_s)
{

}

void motion_engine_run(uint32_t axis_idx,int32_t dist_001mm,int32_t speed_001mm_s)
{

}

void motion_engine_stop(uint32_t axis_mask,uint32_t abort_mask)
{

}




void motion_engine_io_timer(void)
{

}



uint32_t motion_engine_step_axis(motion_timer_t * mt,uint32_t mask)
{
	uint32_t prev_accu;
	uint32_t pulse = 0;

next:
	if(mt->mf[mt->phase].pulse_count > 0)
	{
		mt->mf[mt->phase].pulse_count--;

		prev_accu = mt->accu;
		mt->mf[mt->phase].accel_fract += mt->mf[mt->phase].jerk_fract;
		mt->mf[mt->phase].speed_fract += mt->mf[mt->phase].accel_fract;
		mt->accu += mt->mf[mt->phase].speed_fract;

		if(prev_accu >  mt->accu)
		{
			pulse = mask;
		}
	}
	else
	{
		mt->accu = 0;

		mt->phase++;
		if(mt->phase != MF_PHASES_CNT)
		{
			goto next;
		}
		else
		{
			mctx.mt_active &= ~mask;
		}
	}

	return pulse;
}


void motion_engine_step_timer(void)
{
	uint32_t step_pulse = 0;

	if(mctx.mt_active & 0x01)
	{
		step_pulse |= motion_engine_step_axis(&mctx.mt[AXIS_X],0x01);
	}

	if(mctx.mt_active & 0x02)
	{
		step_pulse |= motion_engine_step_axis(&mctx.mt[AXIS_Y],0x02);
	}

	if(mctx.mt_active & 0x04)
	{
		step_pulse |= motion_engine_step_axis(&mctx.mt[AXIS_Z],0x04);
	}

	if(step_pulse)
	{

	}
}



uint32_t motion_engine_run_status(void)
{
	return mctx.mt_active;
}



