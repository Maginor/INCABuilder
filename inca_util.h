
#if !defined(INCA_UTIL_H)

#include <chrono>
#include <ctime>

#define Min(A, B) ((A) < (B) ? (A) : (B))
#define Max(A, B) ((A) > (B) ? (A) : (B))
#define Square(A) ((A)*(A))

#define Pi 3.141592653589793238462643383279502884

inline void *
AllocClearedArray_(size_t ElementSize, size_t ArrayLen)
{
	void *Result = malloc(ElementSize * ArrayLen);
	memset(Result, 0, ElementSize * ArrayLen);
	return Result;
}
#define AllocClearedArray(Type, ArrayLen) (Type *)AllocClearedArray_(sizeof(Type), ArrayLen)

inline void *
CopyArray_(size_t ElementSize, size_t VecLen, void *Data)
{
	void *Result = malloc(ElementSize * VecLen);
	memcpy(Result, Data, ElementSize * VecLen);
	return Result;
}

inline char *
CopyString(const char *Str)
{
	size_t Len = strlen(Str);
	char *Result = (char *)CopyArray_(1, Len + 1, (void *)Str);
	Result[Len] = 0;
	return Result;
}

struct timer
{
	std::chrono::time_point<std::chrono::high_resolution_clock> Begin;
};

inline timer
BeginTimer()
{
	//TODO: Make OS specific timers to get the best granularity possible (i.e. QueryPerformanceCounter on Windows etc.)
	timer Result;
	Result.Begin = std::chrono::high_resolution_clock::now();
	return Result;
}

inline u64
GetTimerMilliseconds(timer *Timer)
{
	auto End = std::chrono::high_resolution_clock::now();
	double u64 = std::chrono::duration_cast<std::chrono::milliseconds>(End - Timer->Begin).count();
	return u64;
}


inline bool
IsLeapYear(int Year)
{
	if(Year % 4 != 0) return false;
	if(Year % 100 != 0) return true;
	if(Year % 400 != 0) return false;
	return true;
}

//NOTE: Apparently the c++ standard library can not do this for us until c++20, so we have to do it ourselves... (could use boost::ptime, but it has to be compiled separately, and that is asking a lot of the user...)
//NOTE: does not account for leap seconds, but that should not be a problem.
inline s64
ParseSecondsSinceEpoch(const char *DateString)
{
	int Day, Month, Year;
	int MonthOffset[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334}; //, 365};
	int Found = sscanf(DateString, "%d-%d-%d", &Year, &Month, &Day);
	if(Found != 3)
	{
		std::cout << "ERROR: Dates have to be on the format YYYY-MM-DD, we got " << DateString <<std::endl;
		exit(0);
	}
	s64 Result = 0;
	if(Year > 1970)
	{
		for(int Y = 1970; Y < Year; ++Y)
		{
			Result += (365 + IsLeapYear(Y))*24*60*60;
		}
	}
	else if(Year < 1970)
	{
		for(int Y = 1969; Y >= Year; --Y)
		{
			Result -= (365 + IsLeapYear(Y))*24*60*60;
		}
	}
	
	Result += (MonthOffset[Month-1] + ((Month >= 3) && IsLeapYear(Year)))*24*60*60;
	Result += (Day-1)*24*60*60;
	return Result;
}

inline u32
DayOfYear(s64 SecondsSinceEpoch, s32* YearOut)
{
	s32 Year = 1970;
	u32 DayOfYear = 0;
	s64 SecondsLeft = SecondsSinceEpoch;
	if(SecondsLeft > 0)
	{
		while(true)
		{
			s64 SecondsThisYear = (365+IsLeapYear(Year))*24*60*60;
			if(SecondsLeft >= SecondsThisYear)
			{
				Year++;
				SecondsLeft -= SecondsThisYear;
			}
			else break;
		}
		DayOfYear = SecondsLeft / (24*60*60);
	}
	else if(SecondsLeft < 0)
	{
		SecondsLeft = -SecondsLeft;
		Year = 1969;
		s64 SecondsThisYear;
		while(true)
		{
			SecondsThisYear = (365+IsLeapYear(Year))*24*60*60;
			if(SecondsLeft > SecondsThisYear)
			{
				Year--;
				SecondsLeft -= SecondsThisYear;
			}
			else break;
		}
		s64 DaysThisYear = 365 + IsLeapYear(Year);

		SecondsThisYear = DaysThisYear*24*60*60;
		DayOfYear = (SecondsThisYear - SecondsLeft) / (24*60*60);
	}
	
	*YearOut = Year;
	return DayOfYear + 1;
}

inline void
YearMonthDay(s64 SecondsSinceEpoch, s32* YearOut, s32 *MonthOut, s32 *DayOut)
{
	u32 MonthOffset[12] = {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
	u32 Day = DayOfYear(SecondsSinceEpoch, YearOut);

	for(s32 Month = 0; Month < 12; ++Month)
	{
		if(Day <= MonthOffset[Month])
		{
			*MonthOut = (Month+1);
			if(Month == 0) *DayOut = Day;
			else *DayOut = (s32)Day - (s32)MonthOffset[Month-1];
			break;
		}
	}
}




#define INCA_UTIL_H
#endif