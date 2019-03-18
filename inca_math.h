

inline double
LinearInterpolate(double X, double MinX, double MaxX, double MinY, double MaxY)
{
	double XX = (X - MinX) / (MaxX - MinX);
	return MinY + (MaxY - MinY)*XX;
}

inline double
LinearResponse(double X, double MinX, double MaxX, double MinY, double MaxY)
{
	if(X <= MinX) return MinY;
	if(X >= MaxX) return MaxY;
	double XX = (X - MinX) / (MaxX - MinX);
	return MinY + (MaxY - MinY)*XX;
}

inline double
SCurveResponse(double X, double MinX, double MaxX, double MinY, double MaxY)
{
	if(X <= MinX) return MinY;
	if(X >= MaxX) return MaxY;
	double XX = (X - MinX) / (MaxX - MinX);
	double T = (3.0 - 2.0*XX)*XX*XX;
	return MinY + (MaxY - MinY)*T;
}

inline double
InverseGammaResponse(double X, double MinX, double MaxX, double MinY, double MaxY)
{
	const double GammaMinX = 1.461632144968362341262;
	const double GammaMin  = 0.885603194410888;
	
	if(X <= MinX) return MinY;
	
	double XX = (X - MinX) / (MaxX - MinX);
	
	double T = GammaMin / tgamma(XX * GammaMinX);
	
	return MinY + (MaxY - MinY) * T;
}