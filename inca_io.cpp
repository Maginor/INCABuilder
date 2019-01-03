
static void
WriteParameterValue(FILE *File, parameter_value Value, parameter_type Type)
{
	switch(Type)
	{
		case ParameterType_Double:
		fprintf(File, "%.15g", Value.ValDouble);
		break;
		
		case ParameterType_Bool:
		fprintf(File, "%s", Value.ValBool ? "true" : "false");
		break;
		
		case ParameterType_UInt:
		fprintf(File, "%llu", (unsigned long long)(Value.ValUInt)); //TODO: check correctness. May depend on sizeof(long long unsigned int)==8
		break;
		
		case ParameterType_Time:
		s32 Year, Month, Day;
		YearMonthDay(Value.ValTime, &Year, &Month, &Day);
		fprintf(File, "\"%d-%d-%d\"", Year, Month, Day);
		break;
	}
}

static void
WriteParameterValues(FILE *File, handle_t ParameterHandle, parameter_type Type, inca_data_set *DataSet, index_set_h *IndexSets, index_t *Indexes, size_t IndexSetCount, size_t Level)
{
	if(IndexSetCount == 0)
	{
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ParameterHandle);
		parameter_value Value = DataSet->ParameterData[Offset];
		WriteParameterValue(File, Value, Type);
		return;
	}
	
	index_set_h IndexSet = IndexSets[Level];
	size_t IndexCount = DataSet->IndexCounts[IndexSet.Handle];
	
	for(index_t Index = 0; Index < IndexCount; ++Index)
	{
		Indexes[Level] = Index;
		if(Level + 1 == IndexSetCount)
		{
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Indexes, IndexSetCount, DataSet->IndexCounts, ParameterHandle);
			parameter_value Value = DataSet->ParameterData[Offset];
			WriteParameterValue(File, Value, Type);
			if(Index + 1 != IndexCount) fprintf(File, " ");
		}
		else
		{
			WriteParameterValues(File, ParameterHandle, Type, DataSet, IndexSets, Indexes, IndexSetCount, Level + 1);
			if(Index + 1 != IndexCount)
			{
				if(Level + 2 == IndexSetCount) fprintf(File, "\n");
				else fprintf(File, "\n\n");
			}
		}
	}
}

static void
WriteParametersToFile(inca_data_set *DataSet, const char *Filename)
{
	if(!DataSet->ParameterData)
	{
		std::cout << "ERROR: Tried to write parameters to a file before parameter data was allocated." << std::endl;
		exit(0);
	}
	
	FILE *File = fopen(Filename, "w");
	if(!File)
	{	
		std::cout << "Tried to open file " << Filename << ", but were not able to." << std::endl;
		return;
	}
	
	inca_model *Model = DataSet->Model;
	
	fprintf(File, "# Parameter file generated for %s V%s\n\n", Model->Name, Model->Version);
	
	fprintf(File, "index_sets:\n");
	for(handle_t IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{
		index_set_spec &Spec = Model->IndexSetSpecs[IndexSetHandle];
		fprintf(File, "\"%s\" : {", Spec.Name);
		for(index_t IndexIndex = 0; IndexIndex < DataSet->IndexCounts[IndexSetHandle]; ++IndexIndex)
		{
			if(Spec.Type == IndexSetType_Basic)
			{
				if(IndexIndex != 0) fprintf(File, " ");
				fprintf(File, "\"%s\"", DataSet->IndexNames[IndexSetHandle][IndexIndex]);
			}
			else if(Spec.Type == IndexSetType_Branched)
			{
				if(IndexIndex != 0) fprintf(File, " ");
				size_t InputCount = DataSet->BranchInputs[IndexSetHandle][IndexIndex].Count;
				if(InputCount > 0) fprintf(File, "{");
				fprintf(File, "\"%s\"", DataSet->IndexNames[IndexSetHandle][IndexIndex]);
				for(size_t InputIdx = 0; InputIdx < InputCount; ++InputIdx)
				{
					index_t InputIndexIndex = DataSet->BranchInputs[IndexSetHandle][IndexIndex].Inputs[InputIdx];
					fprintf(File, " \"%s\"", DataSet->IndexNames[IndexSetHandle][InputIndexIndex]);
				}
				if(InputCount > 0) fprintf(File, "}");
			}
			else
			{
				assert(0);
			}
		}
		fprintf(File, "}\n");
	}
	
	fprintf(File, "\nparameters:\n");
	for(size_t UnitIndex = 0; UnitIndex < DataSet->ParameterStorageStructure.Units.size(); ++UnitIndex)
	{
		std::vector<index_set_h> &IndexSets = DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets;
		fprintf(File, "######################");
		if(IndexSets.empty()) fprintf(File, " (no index sets)");
		for(index_set_h IndexSet : IndexSets)
		{
			fprintf(File, " \"%s\"", GetName(Model, IndexSet));
		}
		fprintf(File, " ######################\n");
		
		for(handle_t ParameterHandle: DataSet->ParameterStorageStructure.Units[UnitIndex].Handles)
		{
			parameter_spec &Spec = Model->ParameterSpecs[ParameterHandle];
			fprintf(File, "\"%s\" :", Spec.Name);
			bool PrintedPnd = false;
			if(IsValid(Spec.Unit))
			{
				fprintf(File, "     #(%s)", GetName(Model, Spec.Unit));
				PrintedPnd = true;
			}
			if(Spec.Type != ParameterType_Bool)
			{
				if(!PrintedPnd) fprintf(File, "     #");
				PrintedPnd = true;
				fprintf(File, " [");
				WriteParameterValue(File, Spec.Min, Spec.Type);
				fprintf(File, ", ");
				WriteParameterValue(File, Spec.Max, Spec.Type);
				fprintf(File, "]");
			}
			if(Spec.Description)
			{
				if(!PrintedPnd) fprintf(File, "     #");
				fprintf(File, " %s", Spec.Description);
			}
			fprintf(File, "\n");
			
			size_t IndexSetCount = IndexSets.size();
			index_t *CurrentIndexes = AllocClearedArray(index_t, IndexSetCount);
			
			WriteParameterValues(File, ParameterHandle, Spec.Type, DataSet, IndexSets.data(), CurrentIndexes, IndexSetCount, 0);
			
			fprintf(File, "\n\n");
			free(CurrentIndexes);
		}
	}
	
	fclose(File);
}

enum io_file_token_type
{
	TokenType_Unknown = 0,
	TokenType_UnquotedString,
	TokenType_QuotedString,
	TokenType_Colon,
	TokenType_OpenBrace,
	TokenType_CloseBrace,
	TokenType_Numeric,
	TokenType_Bool,
	TokenType_EOF,
};

struct io_file_token
{
	io_file_token_type Type;
	char *StringValue;
	u64 BeforeComma;
	u64 AfterComma;
	u64 Exponent;
	bool HasComma;
	bool IsNegative;
	bool HasExponent;
	bool ExponentIsNegative;
	size_t DigitsAfterComma;
	bool IsNAN;
	bool BoolValue;
};

struct token_stream
{
	const char *Filename;
	u32 StartLine;
	u32 StartColumn;
	u32 Line;
	u32 Column;
	u32 PreviousColumn;
	
	FILE *File;
	
	std::vector<const char *> AllocatedStrings;
	
	token_stream(const char *Filename)
	{
		this->Filename = Filename;
		File = fopen(Filename, "r");
		if(!File)
		{	
			std::cout << "Tried to open file " << Filename << ", but was not able to." << std::endl;
			exit(0);
		}
		
		StartLine = 0; StartColumn = 0; Line = 0; Column = 0; PreviousColumn = 0;
	}
	
	~token_stream()
	{
		if(File) fclose(File);
		for(const char *Str: AllocatedStrings) free((void *)Str); //NOTE: Seems a little inefficient, but since we only allocate for quoted/unquoted strings it should not matter too much. Would be bad if we also did it for all numerical values say.
	}
};

static double
GetDoubleValue(io_file_token& Token)
{
	if(Token.IsNAN)
	{
		return std::numeric_limits<double>::quiet_NaN();
	}
	
	//TODO: Check that this does not create numeric errors.
	
	double BeforeComma = (double)Token.BeforeComma;
	double AfterComma = (double)Token.AfterComma;
	for(size_t I = 0; I < Token.DigitsAfterComma; ++I) AfterComma *= 0.1;
	double Value = BeforeComma + AfterComma;
	if(Token.IsNegative) Value = -Value;
	if(Token.HasExponent)
	{
		//TODO: Check that exponent is not too large.
		double multiplier = Token.ExponentIsNegative ? 0.1 : 10.0;
		for(u64 Exponent = 0; Exponent < Token.Exponent; ++Exponent)
		{
			Value *= multiplier;
		}
	}
	return Value;
}

static bool
IsAlpha(char c)
{
	return isalpha(c) || c == '_';
}

static void
PrintStreamErrorHeader(token_stream &Stream)
{
	std::cout << "ERROR: In file " << Stream.Filename << " line " << (Stream.StartLine+1) << " column " << (Stream.StartColumn) << ": ";
}

static bool
MultiplyByTenAndAdd(u64 *Number, u64 Addendand)
{
	u64 MaxU64 = 0xffffffffffffffff;
	if( (MaxU64 - Addendand) / 10  < *Number) return false;
	*Number = *Number * 10 + Addendand;
	return true;
}

//TODO: Return a token instead of having it as an input.
static void
ReadToken(token_stream &Stream, io_file_token &Token)
{
	Token = {};
	
	char TokenBuffer[512]; //TODO: Yes, do error handling on size etc.
	for(size_t I = 0; I < 512; ++I) TokenBuffer[I] = 0;
	size_t TokenBufferPos = 0;
	
	s32 NumericPos = 0;
	
	bool TokenHasStarted = false;
	
	char c;
	
	bool SkipLine = false;
	
	while(true)
	{
		char c = (char)fgetc(Stream.File);
		if(c == '\n')
		{
			++Stream.Line;
			Stream.PreviousColumn = Stream.Column;
			Stream.Column = 0;
		}
		else ++Stream.Column;
		
		if(c == EOF)
		{
			if(!TokenHasStarted)
			{
				Token.Type = TokenType_EOF;
			}
			break;
		}
		
		if(SkipLine)
		{
			if(c == '\n') SkipLine = false;
			continue;
		}
		
		if(!TokenHasStarted && isspace(c)) continue;
		
		if(!TokenHasStarted)
		{
			if(c == ':') Token.Type = TokenType_Colon;
			else if(c == '{') Token.Type = TokenType_OpenBrace;
			else if(c == '}') Token.Type = TokenType_CloseBrace;
			else if(c == '"') Token.Type = TokenType_QuotedString;
			else if(c == '-' || c == '.' || isdigit(c)) Token.Type = TokenType_Numeric;
			else if(IsAlpha(c)) Token.Type = TokenType_UnquotedString;
			else if(c == '#')
			{
				SkipLine = true;
				continue;
			}
			else
			{
				PrintStreamErrorHeader(Stream);
				std::cout << "Found a token of unknown type" << std::endl;
				exit(0);
			}
			TokenHasStarted = true;
			Stream.StartLine = Stream.Line;
			Stream.StartColumn = Stream.Column;
		}
		
		if(Token.Type == TokenType_Colon || Token.Type == TokenType_OpenBrace || Token.Type == TokenType_CloseBrace)
		{
			return;
		}
		
		if(Token.Type == TokenType_QuotedString)
		{
			if(c == '"' && TokenBufferPos > 0)
			{
				break;
			}
			else if (c != '"')
			{
				if(c == '\n')
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Newline within quoted string." << std::endl;
					exit(0);
				}
				TokenBuffer[TokenBufferPos] = c;
				++TokenBufferPos;
			}
		}
		else if(Token.Type == TokenType_UnquotedString)
		{
			if(!IsAlpha(c))
			{
				char ug = ungetc((int)c, Stream.File);
				if(ug == '\n')
				{
					Stream.Line--;
					Stream.Column = Stream.PreviousColumn;
				}
				else
					Stream.Column--;
				
				break;
			}
			else
			{
				TokenBuffer[TokenBufferPos] = c;
				++TokenBufferPos;
			}
		}
		else if(Token.Type == TokenType_Numeric)
		{
			if(c == '-')
			{
				if( (Token.HasComma && !Token.HasExponent) || (Token.IsNegative && !Token.HasExponent) || NumericPos != 0)
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Misplaced minus in numeric literal." << std::endl;
					exit(0);
				}
				
				if(Token.HasExponent)
				{
					Token.ExponentIsNegative = true;
				}
				else
				{
					Token.IsNegative = true;
				}
				NumericPos = 0;
			}
			else if(c == '+')
			{
				if(!Token.HasExponent || NumericPos != 0)
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Misplaced plus in numeric literal." << std::endl;
					exit(0);
				}
				//ignore the plus.
			}
			else if(c == '.')
			{
				if(Token.HasExponent)
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Comma in exponent in numeric literal." << std::endl;
					exit(0);
				}
				if(Token.HasComma)
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "More than one comma in a numeric literal." << std::endl;
					exit(0);
				}
				NumericPos = 0;
				Token.HasComma = true;
			}
			else if(c == 'e' || c == 'E')
			{
				if(Token.HasExponent)
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "More than one exponent sign ('e' or 'E') in a numeric literal." << std::endl;
					exit(0);
				}
				NumericPos = 0;
				Token.HasExponent = true;
			}
			else if(isdigit(c))
			{
				if(Token.HasExponent)
				{
					MultiplyByTenAndAdd(&Token.Exponent, (u64)(c - '0'));
					u64 MaxExponent = 308; //NOTE: This is not a really thorough test, because we could overflow still..
					if(Token.Exponent > MaxExponent)
					{
						PrintStreamErrorHeader(Stream);
						std::cout << "Too large exponent in numeric literal" << std::endl;
						exit(0);
					}
				}
				else if(Token.HasComma)
				{
					if(!MultiplyByTenAndAdd(&Token.AfterComma, (u64)(c - '0')))
					{
						PrintStreamErrorHeader(Stream);
						std::cout << "Numeric overflow after comma in numeric literal (too many digits)." << std::endl;
						exit(0);
					}
					Token.DigitsAfterComma++;
				}
				else
				{
					if(!MultiplyByTenAndAdd(&Token.BeforeComma, (u64)(c - '0')))
					{
						PrintStreamErrorHeader(Stream);
						std::cout << "Numeric overflow in numeric literal (too many digits). If this is a double, try to use scientific notation instead." << std::endl;
						exit(0);
					}
				}
				++NumericPos;
			}
			
			else
			{
				char ug = ungetc((int)c, Stream.File);
				if(ug == '\n')
				{
					Stream.Line--;
					Stream.Column = Stream.PreviousColumn;
				}
				else
					Stream.Column--;
				
				break;
			}
		}
		
	}
	
	if(Token.Type == TokenType_UnquotedString)
	{
		if(strcmp(TokenBuffer, "true") == 0)
		{
			Token.Type = TokenType_Bool;
			Token.BoolValue = true;
		}
		else if(strcmp(TokenBuffer, "false") == 0)
		{
			Token.Type = TokenType_Bool;
			Token.BoolValue = false;
		}
		else if(strcmp(TokenBuffer, "NaN") == 0 || strcmp(TokenBuffer, "nan") == 0 || strcmp(TokenBuffer, "Nan") == 0)
		{
			Token.Type = TokenType_Numeric;
			Token.IsNAN = true;
		}
	}
	
	if(Token.Type == TokenType_UnquotedString || Token.Type == TokenType_QuotedString)
	{
		Token.StringValue = (char *)malloc(sizeof(char)*(TokenBufferPos + 2));
		Token.StringValue[TokenBufferPos + 1] = 0;
		strcpy(Token.StringValue, TokenBuffer);
		Stream.AllocatedStrings.push_back(Token.StringValue);
	}
}

static void
ExpectToken(token_stream &Stream, io_file_token& Token, io_file_token_type Type)
{
	//NOTE: WARNING: This has to match the io_file_token_type enum!!!
	const char *TokenNames[9] =
	{
		"(unknown)",
		"unquoted string",
		"quoted string",
		":",
		"{",
		"}",
		"number",
		"boolean",
		"(end of file)",
	};
	
	ReadToken(Stream, Token);
	if(Token.Type != Type)
	{
		PrintStreamErrorHeader(Stream);
		std::cout << "Expected a token of type " << TokenNames[Type] << ", got a " << TokenNames[Token.Type];
		if(Token.Type == TokenType_QuotedString || Token.Type == TokenType_UnquotedString)
		{
			std::cout << " (" << Token.StringValue << ")";
		}
		std::cout << std::endl;
		exit(0);
	}
}

static void
AssertInt(token_stream &Stream, io_file_token& Token)
{
	if(Token.HasComma || Token.HasExponent || Token.IsNAN)
	{
		PrintStreamErrorHeader(Stream);
		std::cout << "Got a value of type double when expecting an integer." << std::endl;
		exit(0);
	}
}

static void
AssertUInt(token_stream &Stream, io_file_token& Token)
{
	AssertInt(Stream, Token);
	if(Token.IsNegative)
	{
		PrintStreamErrorHeader(Stream);
		std::cout << "Got a signed value when expecting an unsigned integer." << std::endl;
		exit(0);
	}
}


static void
SetAllValuesForParameter(inca_data_set *DataSet, const char *Name, void *Values, size_t Count)
{
	if(!DataSet->ParameterData)
	{
		AllocateParameterStorage(DataSet); //NOTE: Will fail if not all indexes have been set.
	}
	
	inca_model *Model = DataSet->Model;
	handle_t ParameterHandle = GetParameterHandle(Model, Name);
	if(!ParameterHandle) return;
	
	//TODO: Check that the values are in the Min-Max range? (issue warning only)
	
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	std::vector<index_set_h> &IndexSetStack = DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets;
	
	size_t Stride = DataSet->ParameterStorageStructure.Units[UnitIndex].Handles.size();
	size_t DesiredCount = DataSet->ParameterStorageStructure.TotalCountForUnit[UnitIndex] / Stride;
	
	if(DesiredCount != Count)
	{
		std::cout << "ERROR: Used the SetAllValuesForParameter function with the Parameter " << Name << ", but did not provide the right amount of values." << std::endl;
		exit(0);
	}
	
	parameter_value *In = (parameter_value *)Values;
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ParameterHandle);
	parameter_value *Base = DataSet->ParameterData + Offset;
	
	
	for(size_t Idx = 0; Idx < Count; ++Idx)
	{
		*Base = In[Idx];
		Base += Stride;
	}
}


static void
ReadParametersFromFile(inca_data_set *DataSet, const char *Filename)
{
	token_stream Stream(Filename);
	
	inca_model *Model = DataSet->Model;
	
	int Mode = -1; // 0: index set mode, 1: parameter mode
	
	io_file_token Token = {};
	
	while(true)
	{
		ReadToken(Stream, Token);
		
		if(Token.Type == TokenType_EOF)
		{
			break;
		}
		
		if(Token.Type == TokenType_UnquotedString)
		{
			if(!strcmp(Token.StringValue, "index_sets"))
			{
				Mode = 0;
			}
			else if(!strcmp(Token.StringValue, "parameters"))
			{
				Mode = 1;
			}
			else
			{
				PrintStreamErrorHeader(Stream);
				std::cout << "Unknown command word: " << Token.StringValue << std::endl;
				exit(0);
			}
			ExpectToken(Stream, Token, TokenType_Colon);
			
			ReadToken(Stream, Token);
			if(Token.Type == TokenType_EOF) break;
		}
		
		if(Mode == -1)
		{
			PrintStreamErrorHeader(Stream);
			std::cout << " Expected initialization of an input mode using either the of the command words index_sets or parmeters" << std::endl;
			exit(0);
		}
		else if(Mode == 0)
		{
			if(Token.Type != TokenType_QuotedString)
			{
				PrintStreamErrorHeader(Stream);
				std::cout << "Expected the quoted name of an index set." << std::endl;
				exit(0);
			}
			const char *IndexSetName = Token.StringValue;
			index_set_h IndexSet = GetIndexSetHandle(Model, IndexSetName);
			ExpectToken(Stream, Token, TokenType_Colon);
			ExpectToken(Stream, Token, TokenType_OpenBrace);
			if(Model->IndexSetSpecs[IndexSet.Handle].Type == IndexSetType_Basic)
			{
				std::vector<const char *> Indexes;
				while(true)
				{
					ReadToken(Stream, Token);
					if(Token.Type == TokenType_CloseBrace)
					{
						if(Indexes.empty())
						{
							PrintStreamErrorHeader(Stream);
							std::cout << "Expected one or more indexes for index set " << IndexSetName << std::endl;
							exit(0);
						}
						else
						{
							SetIndexes(DataSet, IndexSetName, Indexes); 
						}
						break;
					}
					else if(Token.Type == TokenType_QuotedString)
					{
						Indexes.push_back(Token.StringValue);
					}
					else
					{
						PrintStreamErrorHeader(Stream);
						std::cout << "Expected either a quoted name of an index or a }" << std::endl;
						exit(0);
					}
				}
			}
			else if(Model->IndexSetSpecs[IndexSet.Handle].Type == IndexSetType_Branched)
			{
				std::vector<std::pair<const char *, std::vector<const char *>>> Indexes;
				while(true)
				{
					ReadToken(Stream, Token);
					if(Token.Type == TokenType_CloseBrace)
					{
						if(Indexes.empty())
						{
							PrintStreamErrorHeader(Stream);
							std::cout << "Expected one or more indexes for index set " << IndexSetName << std::endl;
							exit(0);
						}
						else
						{
							SetBranchIndexes(DataSet, IndexSetName, Indexes);
						}
						break;
					}				
					else if(Token.Type == TokenType_QuotedString)
					{
						Indexes.push_back({Token.StringValue, {}});
					}
					else if(Token.Type == TokenType_OpenBrace)
					{
						const char *IndexName = 0;
						std::vector<const char*> Inputs;
						while(true)
						{
							ReadToken(Stream, Token);
							if(Token.Type == TokenType_CloseBrace)
							{
								if(!IndexName || Inputs.empty())
								{
									PrintStreamErrorHeader(Stream);
									std::cout << "No inputs in the braced list for one of the indexes of index set " << IndexSetName << std::endl;
									exit(0);
								}
								break;
							}
							else if(Token.Type == TokenType_QuotedString)
							{
								if(!IndexName) IndexName = Token.StringValue;
								else Inputs.push_back(Token.StringValue);
							}
							else
							{
								PrintStreamErrorHeader(Stream);
								std::cout << "Expected either the quoted name of an index or a }" << std::endl;
								exit(0);
							}
						}
						Indexes.push_back({IndexName, Inputs});
					}
					else
					{
						PrintStreamErrorHeader(Stream);
						std::cout << "Expected either the quoted name of an index or a }" << std::endl;
						exit(0);
					}
				}
			}
		}
		else if(Mode == 1)
		{	
			if(Token.Type != TokenType_QuotedString)
			{
				PrintStreamErrorHeader(Stream);
				std::cout << "Expected the quoted name of a parameter" << std::endl;
				exit(0);
			}
			
			if(!DataSet->ParameterData)
			{
				AllocateParameterStorage(DataSet);
			}
			
			const char *ParameterName = Token.StringValue;
			handle_t ParameterHandle = GetParameterHandle(Model, ParameterName);
			parameter_type Type = Model->ParameterSpecs[ParameterHandle].Type;
			size_t ExpectedCount = 1;
			size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
			for(index_set_h IndexSet : DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets)
			{
				ExpectedCount *= DataSet->IndexCounts[IndexSet.Handle];
			}

			ExpectToken(Stream, Token, TokenType_Colon);
			parameter_value *Values = AllocClearedArray(parameter_value, ExpectedCount);
			
			for(size_t ValIdx = 0; ValIdx < ExpectedCount; ++ValIdx)
			{
				if(Type == ParameterType_Bool)
				{
					ExpectToken(Stream, Token, TokenType_Bool);
					Values[ValIdx].ValBool = Token.BoolValue;
				}
				else if(Type == ParameterType_Time)
				{
					ExpectToken(Stream, Token, TokenType_QuotedString);
					Values[ValIdx].ValTime = ParseSecondsSinceEpoch(Token.StringValue);
				}
				else
				{
					ExpectToken(Stream, Token, TokenType_Numeric);
					if(Type == ParameterType_Double)
					{
						Values[ValIdx].ValDouble = GetDoubleValue(Token);
					}
					else if(Type == ParameterType_UInt)
					{
						AssertUInt(Stream, Token);
						Values[ValIdx].ValUInt = Token.BeforeComma;
					}
					else { assert(0); }
				}
			}
			SetAllValuesForParameter(DataSet, ParameterName, Values, ExpectedCount);
			free(Values);
		}
	}
}


static void
DlmWriteResultSeriesToFile(inca_data_set *DataSet, const char *Filename, std::vector<const char *> ResultNames, const std::vector<std::vector<const char *>> &Indexes, char Delimiter)
{
	size_t WriteSize = DataSet->TimestepsLastRun;
	size_t NumSeries = ResultNames.size();
	
	double **ResultSeries = AllocClearedArray(double *, NumSeries);
	for(size_t Idx = 0; Idx < NumSeries; ++Idx)
	{
		ResultSeries[Idx] = AllocClearedArray(double, WriteSize);
		GetResultSeries(DataSet, ResultNames[Idx], Indexes[Idx], ResultSeries[Idx], WriteSize);
	}
	
	FILE *file = fopen(Filename, "w");
	if(!file)
	{	
		std::cout << "Tried to open file " << Filename << ", but were not able to." << std::endl;
	}
	else
	{
		for(size_t Timestep = 0; Timestep < WriteSize; ++Timestep)
		{
			for(size_t Idx = 0; Idx < NumSeries; ++Idx)
			{
				fprintf(file, "%f", ResultSeries[Idx][Timestep]);
				if(Idx < NumSeries-1) fprintf(file, "%c", Delimiter);
			}
			
			fprintf(file, "\n");
		}
		fclose(file);
	}
	
	for(size_t Idx = 0; Idx < NumSeries; ++Idx) free(ResultSeries[Idx]);
	free(ResultSeries);	
}


static void
ReadInputsFromFile(inca_data_set *DataSet, const char *Filename)
{
	inca_model *Model = DataSet->Model;
	
	token_stream Stream(Filename);
	
	io_file_token Token = {};
	
	u64 Timesteps = 0;
	bool FoundTimesteps = false;
	
	while(true)
	{
		ReadToken(Stream, Token);
		
		if(Token.Type == TokenType_UnquotedString)
		{
			if(strcmp(Token.StringValue, "timesteps") == 0)
			{
				ExpectToken(Stream, Token, TokenType_Colon);
				ExpectToken(Stream, Token, TokenType_Numeric);
				AssertUInt(Stream, Token);
				Timesteps = Token.BeforeComma;
				if(Timesteps == 0)
				{
					std::cout << "ERROR: Timesteps in the input file " << Filename << " is set to 0." << std::endl;
					exit(0);
				}
				AllocateInputStorage(DataSet, Timesteps);
				FoundTimesteps = true;
			}
			else if(strcmp(Token.StringValue, "start_date") == 0)
			{
				ExpectToken(Stream, Token, TokenType_Colon);
				ExpectToken(Stream, Token, TokenType_QuotedString);
				s64 StartDate = ParseSecondsSinceEpoch(Token.StringValue);
				DataSet->InputDataHasSeparateStartDate = true;
				DataSet->InputDataStartDate = StartDate;
			}
			else if(strcmp(Token.StringValue, "inputs") == 0)
			{
				ExpectToken(Stream, Token, TokenType_Colon);
				break;
			}
			else if(strcmp(Token.StringValue, "index_set_dependencies") != 0 && strcmp(Token.StringValue, "additional_timeseries") != 0)
			{
				//NOTE: We don't actually read the dependencies here
				PrintStreamErrorHeader(Stream);
				std::cout << "Expected one of the code words timesteps, start_date, inputs, additional_timeseries or index_set_dependencies" << std::endl;
				exit(0);
			}
		}
		else if(Token.Type == TokenType_EOF)
		{
			std::cout << "ERROR: Could not find the code word inputs to start reading the inputs in file " << Filename << std::endl;
			exit(0);
		}
	}
	
	if(Timesteps == 0)
	{
		std::cout << "ERROR: in input file " << Filename << ", did not find the codeword timesteps to declare how many timesteps of inputs are provided." << std::endl;
		exit(0);
	}
	
	if(!DataSet->InputDataHasSeparateStartDate)
	{
		DataSet->InputDataStartDate = GetStartDate(DataSet); //NOTE: This reads the "Start date" parameter.
	}
	
	while(true)
	{
		ReadToken(Stream, Token);
		if(Token.Type == TokenType_EOF)
		{
			break;
		}
		if(Token.Type != TokenType_QuotedString)
		{
			PrintStreamErrorHeader(Stream);
			std::cout << "Expected the quoted name of an input" << std::endl;
			exit(0);
		}
		const char *InputName = Token.StringValue;
		
		input_h Input = GetInputHandle(Model, InputName);
		std::vector<const char *> IndexNames;
		
		ReadToken(Stream, Token);
		if(Token.Type == TokenType_OpenBrace)
		{
			while(true)
			{
				ReadToken(Stream, Token);
				if(Token.Type == TokenType_CloseBrace)
				{
					break;
				}
				else if(Token.Type == TokenType_QuotedString)
				{
					IndexNames.push_back(Token.StringValue);
				}
				else
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Expected the quoted name of an index or }" << std::endl;
					exit(0);
				}
			}
			
			ExpectToken(Stream, Token, TokenType_Colon);
			
		}
		else
		{
			if(Token.Type != TokenType_Colon)
			{
				PrintStreamErrorHeader(Stream);
				std::cout << "Expected a :" << std::endl;
				exit(0);
			}
		}
		
		std::vector<index_set_h> &IndexSets = Model->InputSpecs[Input.Handle].IndexSetDependencies;
		if(IndexNames.size() != IndexSets.size())
		{
			PrintStreamErrorHeader(Stream);
			std::cout << "Did not get the right amount of indexes for input " << InputName << std::endl;
			exit(0);
		}
		index_t Indexes[256]; //OK, Ok, this could give a buffer overflow, but will not in practice.
		for(size_t IdxIdx = 0; IdxIdx < IndexNames.size(); ++IdxIdx)
		{
			Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx]);
		}
		
		size_t Offset;
		if(IndexNames.empty())
		{
			Offset = OffsetForHandle(DataSet->InputStorageStructure, Input.Handle);
		}
		else
		{
			Offset = OffsetForHandle(DataSet->InputStorageStructure, Indexes, IndexSets.size(), DataSet->IndexCounts, Input.Handle);
		}
		double *WriteTo = DataSet->InputData + Offset;
		
		//NOTE: For the first timestep, try to figure out what format the data was provided in.
		int FormatType = -1;
		ReadToken(Stream, Token);
		if(Token.Type == TokenType_Numeric)
		{
			FormatType = 0;
		}
		else if(Token.Type == TokenType_QuotedString)
		{
			FormatType = 1;
			double *WriteNanTo = WriteTo;
			//NOTE: For this format, the default value is NaN (signifying a missing value).
			for(u64 Timestep = 0; Timestep < Timesteps; ++ Timestep)
			{
				*WriteNanTo = std::numeric_limits<double>::quiet_NaN();
				WriteNanTo += DataSet->InputStorageStructure.TotalCount;
			}
		}
		else
		{
			PrintStreamErrorHeader(Stream);
			std::cout << "Inputs are to be provided either as a series of numbers or a series of dates together with numbers" << std::endl;
			exit(0);
		}
		
		if(FormatType == 0)
		{
			for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
			{
				if(Timestep != 0) ExpectToken(Stream, Token, TokenType_Numeric); //We already read the token for timestep 0. TODO: We really should have PeekToken function...
				double Value = GetDoubleValue(Token);
				*WriteTo = Value;
				WriteTo += DataSet->InputStorageStructure.TotalCount;
			}
		}
		else //FormatType == 1
		{
			bool Beginning = true;
			
			s64 StartDate = DataSet->InputDataStartDate;;
			
			while(true)
			{
				s64 CurTimestep;
				
				if(!Beginning) ReadToken(Stream, Token);
				
				Beginning = false;
				if(Token.Type == TokenType_QuotedString)
				{
					s64 Date = ParseSecondsSinceEpoch(Token.StringValue);
					
					CurTimestep = DayOffset(StartDate, Date); //NOTE: Only one-day timesteps currently supported.
					
					if(CurTimestep < 0 || CurTimestep >= (s64)Timesteps)
					{
						PrintStreamErrorHeader(Stream);
						std::cout << "The date " << Token.StringValue << " falls outside the time period that we have allocated input storage for." << std::endl;
						exit(0);
					}
				}
				else if(Token.Type == TokenType_UnquotedString)
				{
					if(strcmp(Token.StringValue, "end_timeseries") == 0)
					{
						break;
					}
					else
					{
						PrintStreamErrorHeader(Stream);
						std::cout << "Unexpected command word: " << Token.StringValue << std::endl;
						exit(0);
					}
				}
				else
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Expected either a date (as a quoted string) or the command word end_timeseries." << std::endl;
					exit(0);
				}
				
				ExpectToken(Stream, Token, TokenType_Numeric);
				double Value = GetDoubleValue(Token);
				*(WriteTo + ((size_t)CurTimestep)*DataSet->InputStorageStructure.TotalCount) = Value;
			}
		}
	}
}


static void
ReadInputDependenciesFromFile(inca_model *Model, const char *Filename)
{
	token_stream Stream(Filename);
	
	io_file_token Token = {};

	int Mode = -1;
	while(true)
	{
		ReadToken(Stream, Token);
		if(Token.Type == TokenType_EOF)
		{
			// HMM: it seems that if we have an automated process where input files can be anything, then this print is annoying so we disable it.
			//std::cout << "NOTE: Did not find any dependencies of inputs on index sets in the file " << Filename << std::endl;
			break;
		}
		else if(Token.Type == TokenType_UnquotedString)
		{
			if(strcmp(Token.StringValue, "index_set_dependencies") == 0)
			{
				Mode = 0;
				ExpectToken(Stream, Token, TokenType_Colon);
			}
			else if(strcmp(Token.StringValue, "additional_timeseries") == 0)
			{
				Mode = 1;
				ExpectToken(Stream, Token, TokenType_Colon);
			}
			else if(strcmp(Token.StringValue, "inputs") == 0)
			{
				//NOTE: "index_set_dependencies" and "additional_timeseries" are assumed to come before "inputs" in the file.
				break;
			}
		}
		else
		{
			if(Mode == 0)
			{
				if(Token.Type == TokenType_QuotedString)
				{
					const char *InputName = Token.StringValue;
					input_h Input = GetInputHandle(Model, InputName);
					std::vector<index_set_h> &IndexSets = Model->InputSpecs[Input.Handle].IndexSetDependencies;
					if(!IndexSets.empty()) //TODO: OR we could just clear it and give a warning..
					{
						PrintStreamErrorHeader(Stream);
						std::cout << "Tried to set index set dependencies for the input " << InputName << " for a second time" << std::endl;
						exit(0);
					}
					ExpectToken(Stream, Token, TokenType_Colon);
					ExpectToken(Stream, Token, TokenType_OpenBrace);
					while(true)
					{
						ReadToken(Stream, Token);
						if(Token.Type == TokenType_QuotedString)
						{
							index_set_h IndexSet = GetIndexSetHandle(Model, Token.StringValue);
							IndexSets.push_back(IndexSet);
						}
						else if(Token.Type == TokenType_CloseBrace)
							break;
						else
						{
							PrintStreamErrorHeader(Stream);
							std::cout << "Expected the quoted name of an index or a }" << std::endl;
							exit(0);
						}
					}
				}
				else
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Expected the quoted name of an input or a codeword to start a new section of the file." << std::endl;
				}
			}
			else if(Mode == 1)
			{
				if(Token.Type == TokenType_QuotedString)
				{
					const char *InputName = CopyString(Token.StringValue); //TODO: Leaks, and there is no way to know if we should free it since it gets mixed with other input names that may be statically allocated. Need to make a better system.
					RegisterInput(Model, InputName);
				}
				else
				{
					PrintStreamErrorHeader(Stream);
					std::cout << "Expected the quoted name of an input or a codeword to start a new section of the file." << std::endl;
				}
			}
		}
	}
}
