/*!
    \file motion_scurve.c

    \brief
*/


#include "system.h"
#include "services.h"
#include "middleware.h"
#include "engine.h"

#include "motion_scurve.h"
#include "motion_eq.h"


typedef struct
{
	 double m_dist;
	 double m_speed;
	 double m_speed0;
	 double m_accel;
	 double m_jerk;
	 double half_dist;
	 double calc_dist;
	 double calc_speed;

	 motion_calc_t * calc;
}self_t;





double solve_quad(double a, double b, double c)
{
	int32_t	 eq_num;
	double   eq_results[3];

	 eq_num = quad_eq(a,b,c,eq_results);

	 if(eq_num > 0)
	 {
		 return eq_results[0];
	 }
	 else
	 {
		 return 0;
	 }
}


double solve_qubic(double a, double b, double c,double d)
{
	int32_t	 eq_num;
	double   eq_results[3];

	eq_num = cubic_eq(a,b,c,d,eq_results);

	if(eq_num > 0)
	{
	  return eq_results[0];
	}
	else
	{
	  return 0;
	}
}


void motion_scurve_calc(motion_calc_t * calc,
						int32_t 		dist_001mm,
						uint32_t 		speed_safe_001mm_s,
						uint32_t 		speed_001mm_s,
						uint32_t 		accel_001mm_s2,
						uint32_t 		jerk_001mm_s3
)
{
	 self_t self;

	 if(dist_001mm > 0)
	 {
		 calc->dir = 1;
	 }
	 else
	 {
		 calc->dir  = -1;
		 dist_001mm = -dist_001mm;
	 }



	 self.m_dist    = (double)dist_001mm / 1000.0;
	 self.m_speed   = (double)speed_001mm_s / 1000.0;
	 self.m_speed0  = (double)speed_safe_001mm_s / 1000.0;
	 self.m_accel   = (double)accel_001mm_s2 / 1000.0;
	 self.m_jerk    = (double)jerk_001mm_s3  / 1000.0;

	 self.half_dist = self.m_dist / 2;
	 self.calc 		= calc;



	 // Calculate distance with ideal curve for given acceleration and jerk
	 self.calc_dist = (2*self.m_speed0 + self.m_accel * self.m_accel / self.m_jerk) * self.m_accel/self.m_jerk;

	 if(self.calc_dist > self.half_dist)
	 {
		 // Too much - we are not destined to reach full acceleration, so tune it down
		 self.m_accel = solve_qubic(1/(self.m_jerk*self.m_jerk), (2*self.m_speed0/self.m_jerk), 0, -self.half_dist);
	 }

	 self.calc_speed = self.m_speed0 + self.m_accel * self.m_accel/self.m_jerk;

	 if(self.calc_speed > self.m_speed)
	 {
		 // Too much - we are not destined to reach full speed so tune both acceleration and speed
	     self.m_accel  = sqrt((self.m_speed - self.m_speed0)*self.m_jerk);
		 self.m_speed  = self.m_speed0 + self.m_accel * self.m_accel/self.m_jerk;
	 }

	 self.calc->T11 = self.m_accel / self.m_jerk;
	 self.calc->T13 = self.calc->T11;
	 self.calc->T12 = (self.m_speed - self.m_speed0 - (self.m_accel * self.m_accel)/self.m_jerk)/self.m_accel;

	 self.calc->T11_s = (self.m_speed0 + (self.m_accel * self.m_accel)/(6*self.m_jerk))*(self.m_accel/self.m_jerk);
	 self.calc->T11_v =  self.m_speed0 + (self.m_accel * self.m_accel)/(2*self.m_jerk);

	 self.calc->T12_s =  self.calc->T11_v * self.calc->T12 + self.m_accel * self.calc->T12 * self.calc->T12 / 2;
	 self.calc->T12_v =  self.m_speed - (self.m_accel * self.m_accel) /(2*self.m_jerk);

	 self.calc->T13_s =  (self.calc->T12_v + (self.m_accel * self.m_accel)/(3*self.m_jerk))*(self.m_accel/self.m_jerk);
	 self.calc->T13_v =  self.calc->T12_v + self.m_accel *  self.calc->T13 - self.m_jerk *  self.calc->T13 *  self.calc->T13 /2;

	 self.calc->T1_s = self.calc->T11_s + self.calc->T12_s + self.calc->T13_s;

	 if(self.calc->T1_s > self.half_dist)
	 {
		 // Distance overshoot, linear period needs to be shorter
		 self.calc_dist = self.half_dist - self.calc->T11_s;

		 self.calc->T12 = solve_quad(
				 	 	 	 self.m_accel/2,
				 	 	  	 self.calc->T11_v+ self.m_accel*self.calc->T11,
							 self.calc->T11* self.calc->T11_v+self.m_accel*self.calc->T11*self.calc->T11/2-self.m_jerk*self.calc->T11*self.calc->T11*self.calc->T11/6-self.calc_dist);


		 self.calc->T12_s =  self.calc->T11_v * self.calc->T12 + self.m_accel * self.calc->T12 * self.calc->T12 / 2;
		 self.calc->T12_v =  self.m_speed - (self.m_accel * self.m_accel) /(2*self.m_jerk);

		 self.calc->T13_s =  (self.calc->T12_v + (self.m_accel * self.m_accel)/(3*self.m_jerk))*(self.m_accel/self.m_jerk);
		 self.calc->T13_v =  self.calc->T12_v + self.m_accel *  self.calc->T13 - self.m_jerk *  self.calc->T13 *  self.calc->T13 /2;

		 self.calc->T1_s  = self.calc->T11_s + self.calc->T12_s + self.calc->T13_s;

	 }

	 if(self.calc->T13_v > self.m_speed)
	 {
		 // Too big top speed - even if distance is ok
		 self.calc->T12 = (self.m_speed  -  self.calc->T11_v + self.m_jerk * self.calc->T13 *self.calc->T13 /2 - self.m_accel * self.calc->T13)/  self.m_accel;

		 self.calc->T12_s =  self.calc->T11_v * self.calc->T12 + self.m_accel * self.calc->T12 * self.calc->T12 / 2;
		 self.calc->T12_v =  self.m_speed - (self.m_accel * self.m_accel) /(2*self.m_jerk);

		 self.calc->T13_s =  (self.calc->T12_v + (self.m_accel * self.m_accel)/(3*self.m_jerk))*(self.m_accel/self.m_jerk);
		 self.calc->T13_v =  self.calc->T12_v + self.m_accel *  self.calc->T13 - self.m_jerk *  self.calc->T13 *  self.calc->T13 /2;

		 self.calc->T1_s  = self.calc->T11_s + self.calc->T12_s + self.calc->T13_s;

	 }

	 self.calc->T1 = self.calc->T11 + self.calc->T12 + self.calc->T13;
	 self.calc->T2 = 2* (self.half_dist - self.calc->T1_s)/ self.m_speed;

	 // These may be different then specified
	 self.calc->accel = self.m_accel;
	 self.calc->speed = self.m_speed;
	 self.calc->jerk  = self.m_jerk;


}




















