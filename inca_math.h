
#if !defined(INCA_MATH_H)

//NOTE: Experiment with a potentially faster exp. See http://www.chokkan.org/software/dist/fastexp.c.html
static const double Log2E  =  1.4426950408889634073599;
static const double C1 = 6.93145751953125E-1;
static const double C2 = 1.42860682030941723212E-6;
union ieee754 {
    double d;
    unsigned short s[4];
};
inline double
Exp(double x)
{
	int n;
	double a, px;
	ieee754 u;

	/* n = floor(x / log 2) */
	a = Log2E * x;
	a -= (a < 0);
	n = (int)a;

	/* x -= n * log2 */
	px = (double)n;
	x -= px * C1;
	x -= px * C2;

	/* Compute e^x using a polynomial approximation. */
	a = 1.185268231308989403584147407056378360798378534739e-2;
	a *= x;
	a += 3.87412011356070379615759057344100690905653320886699e-2;
	a *= x;
	a += 0.16775408658617866431779970932853611481292418818223;
	a *= x;
	a += 0.49981934577169208735732248650232562589934399402426;
	a *= x;
	a += 1.00001092396453942157124178508842412412025643386873;
	a *= x;
	a += 0.99999989311082729779536722205742989232069120354073;

	/* Build 2^n in double. */
	u.d = 0;
	n += 1023;
	u.s[3] = (unsigned short)((n << 4) & 0x7FF0);

	return a * u.d;
}


#define Pi 3.141592653589793238462643383279502884

#define INCA_MATH_H
#endif