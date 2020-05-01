#ifndef MOTION_SCURVE_H
#define MOTION_SCURVE_H
 
#include "system.h"
#include "motion_engine.h"


typedef struct
{
	float T1;  		   // Speed    increase
	float T2;  		   // Constant speed
	float T3;  		   // Speed    decrease
	float T11,T12,T13; // Speed increase phase - concave, linear,convex period

	float T11_s;	   // Distance for T11 phase
	float T11_v;	   // Speed at the end of T11 phase

	float T12_s;	   // Distance for T12 phase
	float T12_v;	   // Speed at the end of T12 phase

	float T13_s;	   // Distance for T13 phase
	float T13_v;	   // Speed at the end of T13 phase

	float T1_s;		   // Distance for T1 phase
	float T2_s;		   // Distance for T2 phase


}motion_calc_t;



void motion_scurve_calc(motion_calc_t * calc,
						uint32_t 		dist_001mm,
						uint32_t 		speed_safe_001mm_s,
						uint32_t 		speed_001mm_s,
						uint32_t 		accel_001mm_s2,
						uint32_t 		jerk_001mm_s3
);



#endif // MOTION_SCURVE_H
