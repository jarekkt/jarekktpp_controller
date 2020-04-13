#ifndef PARAMS_H
#define PARAMS_H
 
#include "system.h"

void params_init(void);
void params_once(void);

typedef enum
{
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
	AXIS_CNT
}axis_idx_e;

typedef struct
{
	int32_t   pulses_step_100mm;
	int32_t   pulses_enc_100mm;

	uint32_t  endpos_min_mask;
	uint32_t  endpos_park_mask;
	uint32_t  endpos_max_mask;

	int32_t   endpos_min_value;
	uint32_t  endpos_park_value;
	int32_t   endpos_max_value;

	int32_t   speed_001mm;
	uint32_t  speed_safe_001mm_s;

	int32_t   accel_001mm;
	int32_t   jerk_001mm;

}axis_params_t;


typedef struct
{
	int32_t  pos_001mm;

}axis_state_t;


typedef struct
{
	axis_params_t 		axis[AXIS_CNT];
	uint32_t			estop_mask;
}params_nv_ctx_t;

typedef struct
{
	axis_state_t 		axis[AXIS_CNT];
}params_ctx_t;


#endif // PARAMS_H
