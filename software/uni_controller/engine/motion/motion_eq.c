/*!
    \file motion_scurve.c

    \brief
*/


#include "system.h"
#include "math.h"

//
//  https://www.codeproject.com/Articles/798474/To-Solve-a-Cubic-Equation
//



static double Xroot(double  a, double  x)
{
   double i = 1;
   if (a < 0)
   {
       i = -1;
   }

   return (i * exp( log(a*i)/x));
}



int32_t cubic_eq(double a1, double b, double c, double d, double * results)
{
  double 	 a, p, q, u, v;
  double 	 r, alpha;
  int32_t	 res;

  double	 x1_real = NAN,x2_real = NAN,x3_real = NAN;


  res = 0;
  if (a1 != 0)
  {
     a = b / a1;
     b = c / a1;
     c = d / a1;

     p = -(a * a / 3.0) + b;
     q = (2.0 / 27.0 * a * a * a) - (a * b / 3.0) + c;
     d = q * q / 4.0 + p * p * p / 27.0;

     if (abs(d) < 1e-11)
     {
         d = 0;
     }

     // 3 cases D > 0, D == 0 and D < 0
     if (d > 1e-20)
     {
         u = Xroot(-q / 2.0 + sqrt(d), 3.0);
         v = Xroot(-q / 2.0 - sqrt(d), 3.0);

         x1_real = u + v - a / 3.0;
         /*
         x2.real = -(u + v) / 2.0 - a / 3.0;
         x2.imag = sqrt(3.0) / 2.0 * (u - v);
         x3.real = x2.real;
         x3.imag = -x2.imag;
         */
         res = 1;
     }
     else if (abs(d) <= 1e-20)
     {
         u = Xroot(-q / 2.0, 3.0);
         v = Xroot(-q / 2.0, 3.0);
         x1_real = u + v - a / 3.0;
         x2_real = -(u + v) / 2.0 - a / 3.0;
         res = 2;
     }
     else // (d < -1e-20)
     {
         r = sqrt(-p * p * p / 27.0);
         alpha = atan(sqrt(-d) / -q * 2.0);
         if (q > 0)  // if q > 0 the angle becomes 2 * M_PI - alpha
         {
             alpha = 2.0 * M_PI - alpha;
         }

         x1_real = Xroot(r, 3.0) * (cos((6.0 * M_PI - alpha) / 3.0) + cos(alpha / 3.0)) - a / 3.0;
         x2_real = Xroot(r, 3.0) * (cos((2.0 * M_PI + alpha) / 3.0) + cos((4.0 * M_PI - alpha) / 3.0)) - a / 3.0;
         x3_real = Xroot(r, 3.0) * (cos((4.0 * M_PI + alpha) / 3.0) + cos((2.0 * M_PI - alpha) / 3.0)) - a / 3.0;
         res = 3;
     }
  }

  results[0] = x1_real;
  results[1] = x2_real;
  results[2] = x3_real;

  return res;
}


int32_t quad_eq(double a, double b, double c,double *results)
{
	double delta,p_delta;

	delta = b*b - 4.0 *a *c;

	if( (delta < 0) || (a==0))
	{
		return 0;
	}

	p_delta = sqrt(delta);

	results[0] = (-b-p_delta)/(2.0*a);
	results[1] = (-b+p_delta)/(2.0*a);


	return 2;
}


