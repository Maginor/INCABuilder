

static void
ReadSetupFromFile(glue_setup *Setup, const char *Filename)
{
	token_stream Stream(Filename);
	
	io_file_token Token = {};
	
	int Mode = -1;
	
	while(true)
	{
		ReadToken(Stream, Token);
		
		if(Token.Type == TokenType_EOF)
		{
			break;
		}
		if(Token.Type == TokenType_UnquotedString)
		{
			if(strcmp(Token.StringValue, "num_runs") == 0)
			{
				ExpectToken(Stream, Token, TokenType_Colon);
				ExpectToken(Stream, Token, TokenType_Numeric);
				AssertUInt(Stream, Token);
				size_t NumRuns = (size_t)Token.BeforeComma;
				if(NumRuns == 0)
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Expected at least 1 run." << std::endl;
					exit(0);
				}
				Setup->NumRuns = NumRuns;
			}
			else if(strcmp(Token.StringValue, "parameter_calibration") == 0)
			{
				ExpectToken(Stream, Token, TokenType_Colon);
				Mode = 0;
			}
			else if(strcmp(Token.StringValue, "objectives") == 0)
			{
				ExpectToken(Stream, Token, TokenType_Colon);
				Mode = 1;
			}		
		}
		else if(Mode == 0)
		{
			glue_parameter_calibration ParSetting;
			if(Token.Type != TokenType_QuotedString)
			{
				PrintStreamErrorHeader(Stream);
				std::cout << "Expected the quoted name of a parameter" << std::endl;
				exit(0);
			}
			ParSetting.ParameterName = CopyString(Token.StringValue);  //NOTE: Leaks!!
			ExpectToken(Stream, Token, TokenType_OpenBrace);
			while(true)
			{
				ReadToken(Stream, Token);
				if(Token.Type == TokenType_CloseBrace)
				{
					break;
				}
				else if(Token.Type == TokenType_QuotedString)
				{
					ParSetting.Indexes.push_back(CopyString(Token.StringValue)); //NOTE: Leaks!
				}
				else
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Expected the quoted name of an index" << std::endl;
					exit(0);
				}
			}
			
			ExpectToken(Stream, Token, TokenType_UnquotedString);
			if(strcmp(Token.StringValue, "uniform") == 0)
			{
				ParSetting.Distribution = GLUE_ParameterDistribution_Uniform;
			}
			//else if ...
			else
			{
				PrintStreamErrorHeader(Stream);
				std::cout << "The name " << Token.StringValue << " is not recognized as an implemented random distribution." << std::endl;
				exit(0);
			}
			//TODO: Other parameter types?
			ExpectToken(Stream, Token, TokenType_Numeric);
			ParSetting.Min.ValDouble = GetDoubleValue(Token);
			ExpectToken(Stream, Token, TokenType_Numeric);
			ParSetting.Max.ValDouble = GetDoubleValue(Token);
			Setup->CalibrationSettings.push_back(ParSetting);

		}
		else if(Mode == 1)
		{
			glue_objective Objective;
			if(Token.Type != TokenType_QuotedString)
			{
				PrintStreamErrorHeader(Stream);
				std::cout << "Expected the quoted name of a result series." << std::endl;
				exit(0);
			}
			Objective.Modeled = CopyString(Token.StringValue); //NOTE: Leaks!
			ExpectToken(Stream, Token, TokenType_OpenBrace);
			while(true)
			{
				ReadToken(Stream, Token);
				if(Token.Type == TokenType_CloseBrace)
				{
					break;
				}
				else if(Token.Type == TokenType_QuotedString)
				{
					Objective.IndexesModeled.push_back(CopyString(Token.StringValue)); //NOTE: Leaks!
				}
				else
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Expected the quoted name of an index" << std::endl;
					exit(0);
				}
			}
			ExpectToken(Stream, Token, TokenType_QuotedString);
			Objective.Observed = CopyString(Token.StringValue); //NOTE: Leaks!
			ExpectToken(Stream, Token, TokenType_OpenBrace);
			while(true)
			{
				ReadToken(Stream, Token);
				if(Token.Type == TokenType_CloseBrace)
				{
					break;
				}
				else if(Token.Type == TokenType_QuotedString)
				{
					Objective.IndexesObserved.push_back(CopyString(Token.StringValue)); //NOTE: Leaks!
				}
				else
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Expected the quoted name of an index" << std::endl;
					exit(0);
				}
			}
			ExpectToken(Stream, Token, TokenType_UnquotedString);
			if(strcmp(Token.StringValue, "mean_average_error") == 0)
			{
				Objective.PerformanceMeasure = GLUE_PerformanceMeasure_MeanAverageError;
			}
			else if(strcmp(Token.StringValue, "nash_sutcliffe") == 0)
			{
				Objective.PerformanceMeasure = GLUE_PerformanceMeasure_NashSutcliffe;
			}
			//else if ...
			else
			{
				PrintStreamErrorHeader(Stream);
				std::cout << "The name " << Token.StringValue << " is not recognized as the name of an implemented performance measure" << std::endl;
			}
			ExpectToken(Stream, Token, TokenType_Numeric);
			Objective.Threshold = GetDoubleValue(Token);
			ExpectToken(Stream, Token, TokenType_Numeric);
			Objective.OptimalValue = GetDoubleValue(Token);
			ExpectToken(Stream, Token, TokenType_Bool);
			Objective.Maximize = Token.BoolValue;
			
			Setup->Objectives.push_back(Objective);
		}
		else
		{
			PrintStreamErrorHeader(Stream);
			std::cout << "Unexpected token." << std::endl;
			exit(0);
		}
	}
	
	//TODO: Check that the file contained enough data?
}