
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
		exit(0);
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
WriteParameterValues(FILE *File, entity_handle ParameterHandle, parameter_type Type, inca_data_set *DataSet, index_set_h *IndexSets, index_t *Indexes, size_t IndexSetCount, size_t Level)
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
	
	const inca_model *Model = DataSet->Model;
	
	fprintf(File, "# Parameter file generated for %s V%s\n\n", Model->Name, Model->Version);
	
	fprintf(File, "index_sets:\n");
	for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{
		const index_set_spec &Spec = Model->IndexSetSpecs[IndexSetHandle];
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
		
		for(entity_handle ParameterHandle: DataSet->ParameterStorageStructure.Units[UnitIndex].Handles)
		{
			const parameter_spec &Spec = Model->ParameterSpecs[ParameterHandle];
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

static void
SetAllValuesForParameter(inca_data_set *DataSet, entity_handle ParameterHandle, parameter_value *Values, size_t Count)
{
	//NOTE: There are almost no safety checks in this function. The caller of the function is responsible for the checks!
	
	if(!DataSet->ParameterData)
	{
		AllocateParameterStorage(DataSet); //NOTE: Will fail if not all indexes have been set.
	}
	
	//TODO: Check that the values are in the Min-Max range? (issue warning only)
	
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	std::vector<index_set_h> &IndexSetStack = DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets;
	
	size_t Stride = DataSet->ParameterStorageStructure.Units[UnitIndex].Handles.size();
	
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ParameterHandle);
	parameter_value *Base = DataSet->ParameterData + Offset;
	
	for(size_t Idx = 0; Idx < Count; ++Idx)
	{
		*Base = Values[Idx];
		Base += Stride;
	}
}

static void
ReadParametersFromFile(inca_data_set *DataSet, const char *Filename)
{
	token_stream Stream(Filename);
	
	const inca_model *Model = DataSet->Model;
	
	int Mode = -1; // 0: index set mode, 1: parameter mode
	
	token *Token;
	
	while(true)
	{
		Token = Stream.ReadToken();
		
		if(Token->Type == TokenType_EOF)
		{
			break;
		}
		
		if(Token->Type == TokenType_UnquotedString)
		{
			if(!strcmp(Token->StringValue, "index_sets"))
			{
				Mode = 0;
			}
			else if(!strcmp(Token->StringValue, "parameters"))
			{
				Mode = 1;
			}
			else
			{
				Stream.PrintErrorHeader();
				std::cout << "Unknown command word: " << Token->StringValue << std::endl;
				exit(0);
			}
			Stream.ExpectToken(TokenType_Colon);
			
			Token = Stream.ReadToken();
			if(Token->Type == TokenType_EOF) break;
		}
		
		if(Mode == -1)
		{
			Stream.PrintErrorHeader();
			std::cout << " Expected initialization of an input mode using either the of the command words index_sets or parmeters" << std::endl;
			exit(0);
		}
		else if(Mode == 0)
		{
			if(Token->Type != TokenType_QuotedString)
			{
				Stream.PrintErrorHeader();
				std::cout << "Expected the quoted name of an index set." << std::endl;
				exit(0);
			}
			const char *IndexSetName = Token->StringValue;
			index_set_h IndexSet = GetIndexSetHandle(Model, IndexSetName);
			Stream.ExpectToken(TokenType_Colon);
			if(Model->IndexSetSpecs[IndexSet.Handle].Type == IndexSetType_Basic)
			{
				std::vector<const char *> Indexes;
				ReadQuotedStringList(Stream, Indexes);
				SetIndexes(DataSet, IndexSetName, Indexes);
			}
			else if(Model->IndexSetSpecs[IndexSet.Handle].Type == IndexSetType_Branched)
			{
				//TODO: Make a helper function for this too!
				std::vector<std::pair<const char *, std::vector<const char *>>> Indexes;
				Stream.ExpectToken(TokenType_OpenBrace);
				while(true)
				{
					Token = Stream.ReadToken();
					if(Token->Type == TokenType_CloseBrace)
					{
						if(Indexes.empty())
						{
							Stream.PrintErrorHeader();
							std::cout << "Expected one or more indexes for index set " << IndexSetName << std::endl;
							exit(0);
						}
						else
						{
							SetBranchIndexes(DataSet, IndexSetName, Indexes);
						}
						break;
					}				
					else if(Token->Type == TokenType_QuotedString)
					{
						Indexes.push_back({Token->StringValue, {}});
					}
					else if(Token->Type == TokenType_OpenBrace)
					{
						const char *IndexName = 0;
						std::vector<const char*> Inputs;
						while(true)
						{
							Token = Stream.ReadToken();
							if(Token->Type == TokenType_CloseBrace)
							{
								if(!IndexName || Inputs.empty())
								{
									Stream.PrintErrorHeader();
									std::cout << "No inputs in the braced list for one of the indexes of index set " << IndexSetName << std::endl;
									exit(0);
								}
								break;
							}
							else if(Token->Type == TokenType_QuotedString)
							{
								if(!IndexName) IndexName = Token->StringValue;
								else Inputs.push_back(Token->StringValue);
							}
							else
							{
								Stream.PrintErrorHeader();
								std::cout << "Expected either the quoted name of an index or a }" << std::endl;
								exit(0);
							}
						}
						Indexes.push_back({IndexName, Inputs});
					}
					else
					{
						Stream.PrintErrorHeader();
						std::cout << "Expected either the quoted name of an index or a }" << std::endl;
						exit(0);
					}
				}
			}
		}
		else if(Mode == 1)
		{	
			if(Token->Type != TokenType_QuotedString)
			{
				Stream.PrintErrorHeader();
				std::cout << "Expected the quoted name of a parameter" << std::endl;
				exit(0);
			}
			
			if(!DataSet->ParameterData)
			{
				AllocateParameterStorage(DataSet);
			}
			
			const char *ParameterName = Token->StringValue;
			entity_handle ParameterHandle = GetParameterHandle(Model, ParameterName);
			parameter_type Type = Model->ParameterSpecs[ParameterHandle].Type;
			size_t ExpectedCount = 1;
			size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
			for(index_set_h IndexSet : DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets)
			{
				ExpectedCount *= DataSet->IndexCounts[IndexSet.Handle];
			}

			Stream.ExpectToken(TokenType_Colon);

			if(Type == ParameterType_Time)
			{
				//NOTE: Since we can't distinguish date identifiers from quoted string identifiers, we have to handle them separately. Perhaps we should have a separate format for dates so that the parsing could be handled entirely by the lexer??
				std::vector<parameter_value> Values;
				Values.resize(ExpectedCount);
				
				for(size_t ValIdx = 0; ValIdx < ExpectedCount; ++ValIdx)
				{
					Values[ValIdx].ValTime = ParseSecondsSinceEpoch(Stream.ExpectQuotedString());
				}
				SetAllValuesForParameter(DataSet, ParameterHandle, Values.data(), Values.size());
			}
			else
			{
				std::vector<parameter_value> Values;
				Values.reserve(ExpectedCount);
				ReadParameterSeries(Stream, Values, Type);
				if(Values.size() != ExpectedCount)                                                                   
				{                                                                                                    
					Stream.PrintErrorHeader();                                                                       
					std::cout << "Did not get the expected number of values for " << ParameterName << std::endl;     
					exit(0);                                                                                         
				}                                                                                                    
				SetAllValuesForParameter(DataSet, ParameterHandle, Values.data(), Values.size());
			}
		}
	}
}

static void
ReadInputsFromFile(inca_data_set *DataSet, const char *Filename)
{
	const inca_model *Model = DataSet->Model;
	
	token_stream Stream(Filename);
	
	token *Token;
	
	u64 Timesteps = 0;
	bool FoundTimesteps = false;
	
	while(true)
	{
		Token = Stream.PeekToken();
		
		if(Token->Type == TokenType_EOF)
		{
			Stream.PrintErrorHeader();
			std::cout << "Expected one of the code words timesteps, start_date, inputs, additional_timeseries or index_set_dependencies" << std::endl;
			exit(0);
		}
		
		const char *Section = Stream.ExpectUnquotedString();
		Stream.ExpectToken(TokenType_Colon);

		if(strcmp(Section, "timesteps") == 0)
		{
			Timesteps = Stream.ExpectUInt();
			if(Timesteps == 0)
			{
				std::cout << "ERROR: Timesteps in the input file " << Filename << " is set to 0." << std::endl;
				exit(0);
			}
			AllocateInputStorage(DataSet, Timesteps);
			FoundTimesteps = true;
		}
		else if(strcmp(Section, "start_date") == 0)
		{
			s64 StartDate = ParseSecondsSinceEpoch(Stream.ExpectQuotedString());
			DataSet->InputDataHasSeparateStartDate = true;
			DataSet->InputDataStartDate = StartDate;
		}
		else if(strcmp(Section, "inputs") == 0)
		{
			break;
		}
		else if(strcmp(Section, "index_set_dependencies") == 0 || strcmp(Section, "additional_timeseries") == 0)
		{
			//NOTE: These are handled in a separate call, so we have to skip through them here.
			while(true)
			{
				Token = Stream.PeekToken();
				if(Token->Type == TokenType_UnquotedString) break; //We hit a new section;
				Stream.ReadToken(); //Otherwise consume the token and ignore it.
			}
		}
		else
		{
			Stream.PrintErrorHeader();
			std::cout << "Unrecognized section name " << Section << "." << std::endl;
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
		Token = Stream.ReadToken();
		if(Token->Type == TokenType_EOF)
		{
			break;
		}
		if(Token->Type != TokenType_QuotedString)
		{
			Stream.PrintErrorHeader();
			std::cout << "Expected the quoted name of an input" << std::endl;
			exit(0);
		}
		const char *InputName = Token->StringValue;
		
		input_h Input = GetInputHandle(Model, InputName);
		std::vector<const char *> IndexNames;
		
		Token = Stream.PeekToken();
		if(Token->Type == TokenType_OpenBrace)
		{
			ReadQuotedStringList(Stream, IndexNames);
		}
		
		Stream.ExpectToken(TokenType_Colon);
		
		const std::vector<index_set_h> &IndexSets = Model->InputSpecs[Input.Handle].IndexSetDependencies;
		if(IndexNames.size() != IndexSets.size())
		{
			Stream.PrintErrorHeader();
			std::cout << "Did not get the right amount of indexes for input " << InputName << std::endl;
			exit(0);
		}
		index_t Indexes[256]; //This could cause a buffer overflow, but will not do so in practice.
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
		Token = Stream.PeekToken();
		if(Token->Type == TokenType_Numeric)
		{
			FormatType = 0;
		}
		else if(Token->Type == TokenType_QuotedString)
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
			Stream.PrintErrorHeader();
			std::cout << "Inputs are to be provided either as a series of numbers or a series of dates together with numbers" << std::endl;
			exit(0);
		}
		
		if(FormatType == 0)
		{
			for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
			{
				*WriteTo = Stream.ExpectDouble();
				WriteTo += DataSet->InputStorageStructure.TotalCount;
			}
		}
		else //FormatType == 1
		{
			s64 StartDate = DataSet->InputDataStartDate;
			
			while(true)
			{
				s64 CurTimestep;
				
				Token = Stream.ReadToken(); //TODO: Use the PeekToken functionality above instead
				
				if(Token->Type == TokenType_QuotedString)
				{
					s64 Date = ParseSecondsSinceEpoch(Token->StringValue);
					
					CurTimestep = DayOffset(StartDate, Date); //NOTE: Only one-day timesteps currently supported.
					
					if(CurTimestep < 0 || CurTimestep >= (s64)Timesteps)
					{
						Stream.PrintErrorHeader();
						std::cout << "The date " << Token->StringValue << " falls outside the time period starting with the start date and continuing with the number of specified timesteps." << std::endl;
						exit(0);
					}
				}
				else if(Token->Type == TokenType_UnquotedString)
				{
					if(strcmp(Token->StringValue, "end_timeseries") == 0)
					{
						break;
					}
					else
					{
						Stream.PrintErrorHeader();
						std::cout << "Unexpected command word: " << Token->StringValue << std::endl;
						exit(0);
					}
				}
				else
				{
					Stream.PrintErrorHeader();
					std::cout << "Expected either a date (as a quoted string) or the command word end_timeseries." << std::endl;
					exit(0);
				}
				
				*(WriteTo + ((size_t)CurTimestep)*DataSet->InputStorageStructure.TotalCount) = Stream.ExpectDouble();
			}
		}
	}
}


static void
ReadInputDependenciesFromFile(inca_model *Model, const char *Filename)
{
	token_stream Stream(Filename);
	
	token *Token;

	int Mode = -1;
	while(true)
	{
		Token = Stream.PeekToken();
		if(Token->Type == TokenType_EOF)
			break;
		
		const char *Section = Stream.ExpectUnquotedString();
		Stream.ExpectToken(TokenType_Colon);

		if(strcmp(Section, "index_set_dependencies") == 0)
		{
			while(true)
			{
				Token = Stream.PeekToken();
				if(Token->Type == TokenType_QuotedString)
				{
					Token = Stream.ReadToken();
					const char *InputName = Token->StringValue;
					input_h Input = GetInputHandle(Model, InputName);
					std::vector<index_set_h> &IndexSets = Model->InputSpecs[Input.Handle].IndexSetDependencies;
					if(!IndexSets.empty()) //TODO: OR we could just clear it and give a warning..
					{
						Stream.PrintErrorHeader();
						std::cout << "Tried to set index set dependencies for the input " << InputName << " for a second time." << std::endl;
						exit(0);
					}
					Stream.ExpectToken(TokenType_Colon);
					
					std::vector<const char *> IndexSetNames;
					ReadQuotedStringList(Stream, IndexSetNames, false);
					
					for(const char *IndexSetName : IndexSetNames)
					{
						IndexSets.push_back(GetIndexSetHandle(Model, IndexSetName));
					}
				}
				else break;
			}
		}
		else if(strcmp(Section, "additional_timeseries") == 0)
		{
			while(true)
			{
				Token = Stream.PeekToken();
				if(Token->Type == TokenType_QuotedString)
				{
					Token = Stream.ReadToken();
					const char *InputName = CopyString(Token->StringValue); //TODO: Leaks, and there is no way to know if we should free it since it gets mixed with other input names that may be statically allocated. Need to make a better system.
					RegisterInput(Model, InputName);
				}
				else break;
			}
		}
		else if(strcmp(Section, "inputs") == 0)
		{
			//NOTE: "index_set_dependencies" and "additional_timeseries" are assumed to come before "inputs" in the file.
			// This is so that we don't have to skip through the entire inputs section on this read since it can be quite long.
			break;
		}
		else
		{
			//NOTE: We have to skip through other sections that are not relevant for this reading
			while(true)
			{
				Token = Stream.PeekToken();
				if(Token->Type == TokenType_UnquotedString) break; //We hit a new section;
				Stream.ReadToken(); //Otherwise consume the token and ignore it.
			}
		}
	}
}
