
//NOTE: This file is for common functionality between all calibration/uncertainty analysis



//NOTE: Specification of calibration of one (or several linked) parameter(s).
struct parameter_calibration
{
	std::vector<const char *> ParameterNames;
	std::vector<std::vector<const char *>> ParameterIndexes;
	double Min;
	double Max;
	double InitialGuess;
};

static void
ReadParameterCalibration(token_stream &Stream, std::vector<parameter_calibration> &CalibOut)
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
				Calib.ParameterNames.push_back(CopyString(Token->StringValue));  //NOTE: The copied string leaks unless somebody frees it later
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
		
		Calib.Min = Stream.ExpectDouble();
		Calib.Max = Stream.ExpectDouble();
		Calib.InitialGuess = Stream.ExpectDouble(); //TODO: Maybe we should not always require this..
		//TODO: Optional distribution?
		
		CalibOut.push_back(Calib);
		
	}
}

inline void
SetCalibrationValue(inca_data_set *DataSet, parameter_calibration &Calibration, double Value)
{
	for(size_t ParIdx = 0; ParIdx < Calibration.ParameterNames.size(); ++ParIdx)
	{
		SetParameterDouble(DataSet, Calibration.ParameterNames[ParIdx], Calibration.ParameterIndexes[ParIdx], Value);
	}
}