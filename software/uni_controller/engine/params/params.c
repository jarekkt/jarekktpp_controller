/*!
    \file motion_engine.c

    \brief
*/


#include "system.h"
#include "services.h"
#include "middleware.h"
#include "engine.h"

#include "params.h"



params_ctx_t 	 pctx;
params_nv_ctx_t  pctx_nv VAR_NV_ATTR;


void params_init_default(void)
{
	memset(&pctx_nv,0,sizeof(pctx_nv));
}

void params_init(void)
{
	memset(&pctx,0,sizeof(pctx));

	srv_nov_register(&pctx_nv,sizeof(pctx_nv),params_init_default);
}


void params_once(void)
{

}

