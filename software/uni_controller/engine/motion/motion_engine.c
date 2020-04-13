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
	uint32_t mode;
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

void motion_engine_run(uint32_t axis_idx,int32_t dist_001mm,int32_t speed_001mm_s)
{

}

void motion_engine_stop(uint32_t axis_mask,uint32_t abort_mask)
{

}

uint32_t motion_engine_run_status(void)
{
	uint32_t result = 0;

	return result;
}



