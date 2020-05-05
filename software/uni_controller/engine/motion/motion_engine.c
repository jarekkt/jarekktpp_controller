/*!
    \file motion_engine.c

    \brief
*/


#include "system.h"
#include "services.h"
#include "middleware.h"
#include "engine.h"

#include "motion_engine.h"
#include "motion_scurve.h"


typedef struct
{
	uint32_t 			mode;
	uint32_t			mt_active;

	volatile uint32_t	irq_mask;


	uint32_t			home_axis;

	uint32_t			abort_mask;
	uint32_t			stop_mask;
	uint32_t			start_mask;

	motion_timer_t		mt[AXIS_CNT];
}motion_ctx_t;

typedef struct
{
	uint32_t 			step_freq;

}motion_nv_data_t;


motion_ctx_t 	  mctx;
motion_nv_data_t  mctx_nv VAR_NV_ATTR;


static void motion_engine_tmr_step(void);
static void motion_engine_tmr_endpos(void);


void motion_engine_init_default(void)
{
	memset(&mctx_nv,0,sizeof(mctx_nv));
}

void motion_engine_init(void)
{
	memset(&mctx,0,sizeof(mctx));

	srv_nov_register(&mctx_nv,sizeof(mctx_nv),motion_engine_init_default);
}


void motion_engine_reconfig(void)
{
	// Step calculations generation
	srv_timer_callback_step(mctx_nv.step_freq,motion_engine_tmr_step);

	// Pulse length calculation - step frequency to 0.1us step pulse units
	srv_timer_pulse_period( (10*1000000 / mctx_nv.step_freq)/2);
}



void motion_engine_once(void)
{
	srv_timer_callback_fast_add(motion_engine_tmr_endpos);
	motion_engine_reconfig();
}

void motion_engine_stop(uint32_t axis_mask,uint32_t abort_mask)
{
	mctx.stop_mask  |= axis_mask;
	mctx.abort_mask |= abort_mask;

	mctx.irq_mask = 1;

	while(mctx.irq_mask != 0)
	{
		//TODO - add reasonable timeout/task switch
	}

}

void motion_engine_start(uint32_t axis_mask)
{
	mctx.start_mask  |= axis_mask;
}


void motion_engine_run_home(uint32_t axis_idx,int32_t speed_001mm_s,int32_t accel_001mm_s2,int32_t jerk_001mm_s3)
{
	int32_t dist;

	if(axis_idx >AXIS_CNT)
	{
		fw_assert(0);
		return;
	}

	mctx.home_axis |= (1<<axis_idx);

	// Calculate max possible distance
	dist = (ppctx_nv->axis[axis_idx].endpos_max_value - ppctx_nv->axis[axis_idx].endpos_min_value) + MM_TO_001(10);

	if(speed_001mm_s > 0)
	{
		dist = - dist;
	}


	motion_engine_run(axis_idx,dist,speed_001mm_s,accel_001mm_s2,jerk_001mm_s3);

}



void motion_engine_convert(uint32_t axis_idx,int32_t dist_001mm,int32_t pos_001mm,const motion_calc_t  * calc,const axis_params_t * axis,motion_timer_t * tmr)
{
	float T2;  		   // Constant speed
	float T3;  		   // Speed    decrease
	float T11,T12,T13; // Concave, linear and convex period

	float T11_s;	   // Distance for T11 phase
	float T11_v;	   // Speed at the end of T11 phase

	float T12_s;	   // Distance for T12 phase
	float T12_v;	   // Speed at the end of T12 phase

	float T13_s;	   // Distance for T13 phase
	float T13_v;	   // Speed at the end of T13 phase

	float T1_s;		   // Distance for T1 phase
	float T2_s;		   // Distance for T2 phase

	float accel;
	float speed;
	float jerk;


#define C_POS_2_PULSE(pos_001mm_,axis_idx_)	(int32_t)( (((int64_t)pos_001mm_) * ((int64_t)ppctx_nv->axis[axis_idx_].pulses_step_100mm)) / 100000)
#define C_POS_2_ENC(pos_001mm_,axis_idx_)	(int32_t)( (((int64_t)pos_001mm_) * ((int64_t)ppctx_nv->axis[axis_idx_].pulses_enc_100mm)) / 100000)



	tmr->accu  = 0;
	tmr->phase = MF_START_CONCAVE;

	tmr->pos_next.pos_001mm    = pos_001mm;
	tmr->pos_next.pulse_count  = C_POS_2_PULSE(pos_001mm,axis_idx);
	tmr->pos_next.enc_count    = C_POS_2_ENC(pos_001mm,axis_idx);








}




void motion_engine_run(uint32_t axis_idx,int32_t pos_001mm,int32_t speed_001mm_s,int32_t accel_001mm_s2,int32_t jerk_001mm_s3)
{
	motion_calc_t  calc;
	int32_t		   dist_001mm;

	// Abort the move if in progress
	if((mctx.mt_active & (1<<axis_idx)) != 0)
	{
		motion_engine_stop(0,(1<<axis_idx));
	}

	dist_001mm = mctx.mt[axis_idx].pos_cur.pos_001mm - pos_001mm;

	motion_scurve_calc(&calc, ppctx_nv->axis[axis_idx].speed_safe_001mm_s,dist_001mm,speed_001mm_s,accel_001mm_s2,jerk_001mm_s3);
	motion_engine_convert(axis_idx,dist_001mm,pos_001mm,&calc,&ppctx_nv->axis[axis_idx],&mctx.mt[axis_idx]);

	motion_engine_start(1<<axis_idx);
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




uint32_t motion_engine_run_status(void)
{
	return mctx.mt_active;
}



void motion_engine_tmr_masks()
{
	int ii;

	mctx.mt_active   &= mctx.abort_mask;
	mctx.mt_active   |= mctx.start_mask;
	mctx.abort_mask  = 0;

	if(mctx.stop_mask != 0)
	{
		for(ii = 0; ii < AXIS_CNT;ii++)
		{
			if(mctx.mt_active & (mctx.stop_mask & (1<<ii)))
			{
				switch(mctx.mt[ii].phase)
				{
					case MF_START_CONCAVE:
					{

					}break;

					case MF_START_LINEAR:
					{

					}break;

					case MF_START_CONVEX:
					{

					}break;

					case MF_CONSTANT_SPEED:
					{

					}break;

					case MF_STOP_CONVEX:
					case MF_STOP_LINEAR:
					case MF_STOP_CONCAVE:
					case MF_PHASES_CNT:
					{
						// Already in stopping phase
					}break;
				}
			}
		}
		mctx.stop_mask = 0;
	}
}


static void motion_engine_tmr_step(void)
{
	if(mctx.mt_active & 0x01)
	{
		if(motion_engine_step_axis(&mctx.mt[AXIS_X],0x01) != 0)
		{
			TMR_TIRGGER_X();
		}
	}

	if(mctx.mt_active & 0x02)
	{
		if( motion_engine_step_axis(&mctx.mt[AXIS_Y],0x02)!=0)
		{
			TMR_TIRGGER_Y();
		}
	}

	if(mctx.mt_active & 0x04)
	{
		if(motion_engine_step_axis(&mctx.mt[AXIS_Z],0x04)!=0)
		{
			TMR_TIRGGER_Z();
		}
	}


	if(mctx.irq_mask != 0)
	{
		motion_engine_tmr_masks();

		mctx.irq_mask = 0;
	}


}

static void motion_engine_tmr_endpos(void)
{

}


