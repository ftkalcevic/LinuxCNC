
//#if __GNUC_PREREQ(4,4)
#define isinf(x) __builtin_isinf((x))
//#else
//#define isinf(x) ({ double v=((x)); !isnan(v) && isnan(v-v); })
//#endif


extern double floor(double);

double
round(double x)
{
   double t;
   if (isinf (x))
     return (x);

   if (x >= 0.0) 
    {
      t = floor(x);
      if (t - x <= -0.5)
	t += 1.0;
      return (t);
    } 
   else 
    {
      t = floor(-x);
      if (t + x <= -0.5)
	t += 1.0;
      return (-t);
    }
}

double
__powidf2 (double x, int m)
{
  unsigned int n = m < 0 ? -m : m;
  double y = n % 2 ? x : 1;
  while (n >>= 1)
    {
      x = x * x;
      if (n % 2)
    y = y * x;
    }
  return m < 0 ? 1/y : y;
}

long int
lround (double x)
{
      return (long int) round (x);
}

