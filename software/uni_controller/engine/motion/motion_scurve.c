/*!
    \file motion_scurve.c

    \brief
*/


#include "system.h"
#include "services.h"
#include "middleware.h"
#include "engine.h"

#include "motion_scurve.h"



void motion_scurve_calc(motion_calc_t * calc,
						uint32_t 		dist_001mm,
						uint32_t 		speed_safe_001mm_s,
						uint32_t 		speed_001mm_s,
						uint32_t 		accel_001mm_s2,
						uint32_t 		jerk_001mm_s3
)
{

	 double m_dist    = (double)dist_001mm / 1000.0;
	 double m_speed   = (double)speed_001mm_s / 1000.0;
	 double m_speed0  = (double)speed_safe_001mm_s / 1000.0;
	 double m_accel   = (double)accel_001mm_s2 / 1000.0;
	 double m_jerk    = (double)jerk_001mm_s3  / 1000.0;












}




















