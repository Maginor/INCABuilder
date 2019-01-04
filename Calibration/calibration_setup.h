
//NOTE: This file is for common functionality between all calibration/uncertainty analysis

enum parameter_distribution
{
	ParameterDistribution_Uniform,
	//TODO: Support a few more!!!!
};

//NOTE: Specification of calibration of one (or several linked) parameter(s).
struct parameter_calibration
{
	std::vector<const char *> ParameterNames;
	std::vector<std::vector<const char *>> ParameterIndexes;
	parameter_distribution Distribution;
	double Min;
	double Max;
	double InitialGuess;
};

const u64 ParameterCalibrationReadDistribution   = 0x1;
const u64 ParameterCalibrationReadInitialGuesses = 0x2;

static void
ReadParameterCalibration(token_stream &Stream, std::vector<parameter_calibration> &CalibOut, u64 Flags = 0)
{
	//NOTE: This function assumes that the Stream has been opened and has already consumed the tokens 'parameter_calibration' and ':'
	
	while(true)
	{
		bool WeAreInLink = false;
		parameter_calibration Calib;
		
		while(true)
		{
			token *Token = Stream.PeekToken();
			if(Token->Type == TokenType_QuotedString)
			{
				const char *ParameterName = CopyString(Token->StringValue); //NOTE: The copied string leaks unless somebody frees it later
				
				Calib.ParameterNames.push_back(ParameterName);  
				Stream.ReadToken(); //NOTE: Consumes the token we peeked.
				
				std::vector<const char *> Indexes;
				ReadQuotedStringList(Stream, Indexes, true); //NOTE: The copied strings leak unless we free them later
				Calib.ParameterIndexes.push_back(Indexes);
				
				if(!WeAreInLink) break;
			}
			else if(Token->Type == TokenType_UnquotedString)
			{
				if(WeAreInLink)
				{
					Stream.PrintErrorHeader();
					std::cout << "Unexpected token inside link." << std::endl;
					exit(0);
				}
				
				if(strcmp(Token->StringValue, "link") == 0)
				{
					WeAreInLink = true;
					Stream.ReadToken(); //NOTE: Consumes the token we peeked.
					Stream.ExpectToken(TokenType_OpenBrace);
				}
				else
				{
					//NOTE: We hit another code word that signifies a new section of the file.
					//NOTE: Here we should not consume the peeked token since the caller needs to be able to read it.
					return;
				}
			}
			else if(WeAreInLink && Token->Type == TokenType_CloseBrace)
			{
				Stream.ReadToken(); //NOTE: Consumes the token we peeked.
				break;
			}
			else if(Token->Type == TokenType_EOF)
			{
				if(WeAreInLink)
				{
					Stream.PrintErrorHeader();
					std::cout << "File ended unexpectedly." << std::endl;
					exit(0);
				}
				
				return;
			}
			else
			{
				Stream.PrintErrorHeader();
				std::cout << "Unexpected token." << std::endl;
				exit(0);
			}
		}
		WeAreInLink = false;
		
		if(Flags & ParameterCalibrationReadDistribution)
		{
			const char *DistrName = Stream.ExpectUnquotedString();
			if(strcmp(DistrName, "uniform") == 0)
			{
				Calib.Distribution = ParameterDistribution_Uniform;
			}
			//else if ...
			else
			{
				Stream.PrintErrorHeader();
				std::cout << "Unsupported distribution: " << DistrName << std::endl;
				exit(0);
			}
		}
		
		Calib.Min = Stream.ExpectDouble();
		Calib.Max = Stream.ExpectDouble();
		if(Flags & ParameterCalibrationReadInitialGuesses)
		{
			Calib.InitialGuess = Stream.ExpectDouble();
		}
		//TODO: Optional distribution?
		
		CalibOut.push_back(Calib);
		
	}
}

inline void
SetCalibrationValue(inca_data_set *DataSet, parameter_calibration &Calibration, double Value)
{
	for(size_t ParIdx = 0; ParIdx < Calibration.ParameterNames.size(); ++ParIdx)
	{
		SetParameterValue(DataSet, Calibration.ParameterNames[ParIdx], Calibration.ParameterIndexes[ParIdx], Value);
	}
}