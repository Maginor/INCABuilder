
// #define INCA_SOLVER_FUNCTION(Name) Name(double h, u32 n, double* x0, double* wk, inca_solver_equation_function &EquationFunction)

#if !defined(INCA_SOLVERS_H)

#include <math.h>


INCA_SOLVER_FUNCTION(IncaEuler)
{
	//NOTE: This is not meant to be used as a proper solver, it is just an illustration of how a solver function works.
	
	double haccum = 0.0;
	while(haccum < 1.0)
	{
		EquationFunction(x0, wk);
		
		for(u32 Idx = 0; Idx < n; ++Idx)
		{
			x0[Idx] += h*wk[Idx];
		}
		
		haccum += h;
	}
}

INCA_SOLVER_FUNCTION(IncaDascru)
{
	double x, hmin, xs, hs, q, h3, r, e;
    int ib1, ib2, sw, i, j, ijk0, ijk1, ijk2, be, bh=1, br=1, bx=1;

	//NOTE: Substituting a = 0.0, b = 1.0
/*
    if ( a == b )
    {
        for (i=0; i<n; i++)
        {
            x0[i] = 0.0;
        }

        return;
    }
*/

    ib1 = n + n;
    ib2 = ib1 + n;
    //hmin = 0.01 * fabs( h );
	hmin = 0.01 * h;

/*
    h = SIGN( fabs( h ), ( b - a ) );
    x = double( a );
*/
	x = 0.0;

    while ( br )
    {
        xs = x;

        for (j=0; j<n; j++)
        {
            ijk0 = n + j;
            wk[ijk0] = x0[j];
        }

FT:     hs = h;
        //q = x + h - b;
		q = x + h - 1.0;
        be = 1;

        if (!((h > 0.0 && q >= 0.0) || (h < 0.0 && q <= 0.0))) goto TT;

        //h = b - x;
		h = 1.0 - x;
        br = 0;
TT:     h3 = h / 3.0;

        for (sw=0; sw<5; sw++)
        {

            EquationFunction( x0, wk );

            for (i=0; i<n; i++)
            {
                q = h3 * wk[i];
                ijk0 = n + i;
                ijk1 = ib1 + i;
                ijk2 = ib2 + i;

                switch( sw )
                {
                    case 0  :   r = q;
                                wk[ijk1] = q;
                                break;

                    case 1  :   r = 0.5 * ( q + wk[ijk1] );
                                break;

                    case 2  :   r = 3.0 * q;
                                wk[ijk2] = r;
                                r= 0.375 * ( r + wk[ijk1] );
                                break;

                    case 3  :   r = wk[ijk1] + 4.0 * q;
                                wk[ijk1] = r;
                                r = 1.5 * ( r - wk[ijk2] );
                                break;

                    case 4  :   r = 0.5 * ( q + wk[ijk1] );
                                q = fabs( r + r - 1.5 * ( q + wk[ijk2] ) );
                                break;
                }

                x0[i] = wk[ijk0] + r;

                if ( sw == 4 )
                {
                    e = fabs( x0[i] );
                    r = 0.0005;

                    if ( e >= 0.001 ) r = e * 0.0005;
                    if ( q < r || !bx ) goto SXYFV;

                    br = 1;
                    bh = 0;
                    h = 0.5 * h;

                    if ( fabs(h) < hmin )
                    {
						#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))
                        h = SIGN( hmin, h );
						#undef SIGN
                        bx = 0;
                    }

                    for (j=0; j<n; j++)
                    {
                        ijk0 = n + j;
                        x0[j] = wk[ijk0];
                    }

                    x = xs;
                    goto FT;

SXYFV:	            if ( q >= ( 0.03125 * r ) ) be=0;
                }
            }

            if ( sw == 0 ) x = x + h3;
            if ( sw == 2 ) x = x + 0.5 * h3;
            if ( sw == 3 ) x = x + 0.5 * h;
        }

        if ( be && bh && br )
        {
            h = h + h;
            bx = 1;
        }

        bh = 1;
    }

    h = hs;

    if ( bx || be )
    {
        return;
    }
}


#define INCA_SOLVERS_H
#endif
