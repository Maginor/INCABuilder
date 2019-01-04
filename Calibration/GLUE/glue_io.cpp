


static void
ReadSetupFromFile(glue_setup *Setup, const char *Filename)
{
	token_stream Stream(Filename);
	
	token *Token;
	
	
	
	while(true)
	{
		Token = Stream.PeekToken();
		
		bool ReadObs = false;
		
		if(Token->Type == TokenType_EOF)
			break;

		const char *Section = Stream.ExpectUnquotedString();
		Stream.ExpectToken(TokenType_Colon);

		if(strcmp(Section, "num_runs") == 0)
		{
			size_t NumRuns = Stream.ExpectUInt();
			if(NumRuns == 0)
			{
				Stream.PrintErrorHeader();
				std::cout << "Expected at least 1 run." << std::endl;
				exit(0);
			}
			Setup->NumRuns = NumRuns;
		}
		else if(strcmp(Section, "num_threads") == 0)
		{
			size_t NumThreads = (size_t)Stream.ExpectUInt();
			if(NumThreads == 0)
			{
				Stream.PrintErrorHeader();
				std::cout << "Expected at least 1 thread." << std::endl;
				exit(0);
			}
			Setup->NumThreads = NumThreads;
		}
		else if(strcmp(Section, "parameter_calibration") == 0)
		{
			ReadParameterCalibration(Stream, Setup->Calibration, ParameterCalibrationReadDistribution);
		}
		else if(strcmp(Section, "objectives") == 0)
		{
			ReadObs = true;
		}
		else if(strcmp(Section, "quantiles") == 0)
		{
			ReadDoubleSeries(Stream, Setup->Quantiles);
		}

		if(ReadObs)
		{
			//TODO: This currently just reads exactly one objective. We may allow for multiple objectives later.
			
			glue_objective Objective;
			Token = Stream.ReadToken();
			if(Token->Type != TokenType_QuotedString)
			{
				Stream.PrintErrorHeader();
				std::cout << "Expected the quoted name of a result series." << std::endl;
				exit(0);
			}
			Objective.Modeled = CopyString(Token->StringValue); //NOTE: Leaks!
			
			ReadQuotedStringList(Stream, Objective.IndexesModeled, true);
			
			Token = Stream.ExpectToken(TokenType_QuotedString);
			Objective.Observed = CopyString(Token->StringValue); //NOTE: Leaks!
			
			ReadQuotedStringList(Stream, Objective.IndexesObserved, true);
			
			Token = Stream.ExpectToken(TokenType_UnquotedString);
			if(strcmp(Token->StringValue, "mean_average_error") == 0)
			{
				Objective.PerformanceMeasure = GLUE_PerformanceMeasure_MeanAverageError;
			}
			else if(strcmp(Token->StringValue, "nash_sutcliffe") == 0)
			{
				Objective.PerformanceMeasure = GLUE_PerformanceMeasure_NashSutcliffe;
			}
			//else if ...
			else
			{
				Stream.PrintErrorHeader();
				std::cout << "The name " << Token->StringValue << " is not recognized as the name of an implemented performance measure" << std::endl;
			}
			Objective.Threshold = Stream.ExpectDouble();
			Objective.OptimalValue = Stream.ExpectDouble();
			Objective.Maximize = Stream.ExpectBool();
			
			Setup->Objectives.push_back(Objective);
		}
	}
	
	//TODO: Check that the file contained enough data?
}

#if 0

//TODO: This format does not support having linked parameters, so we have to rething it!!

static void
WriteGLUEResultsToDatabase(const char *Dbname, glue_setup *Setup, glue_results *Results, inca_data_set *DataSet)
{
	sqlite3 *Db;
	int rc = sqlite3_open_v2(Dbname, &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
	if(rc != SQLITE_OK)
	{
		std::cout << "ERROR: Unable to open database " << Dbname << " Message: " << sqlite3_errmsg(Db) << std::endl;
		exit(0);
	}
	
	rc = sqlite3_exec(Db, "BEGIN TRANSACTION;", 0, 0, 0);
	
	sqlite3_stmt *CreateTableStmt;
	
	const char *CreateParameterTable =
		"CREATE TABLE Parameters (ID INTEGER NOT NULL PRIMARY KEY, Name TEXT, Indexes TEXT);";
	rc = sqlite3_prepare_v2(Db, CreateParameterTable, -1, &CreateTableStmt, 0);
	rc = sqlite3_step(CreateTableStmt);
	rc = sqlite3_finalize(CreateTableStmt);
		
	const char *CreateRunTable =
		"CREATE TABLE Run (ID INTEGER NOT NULL PRIMARY KEY, Performance DOUBLE, WeightedPerformance DOUBLE);";
	rc = sqlite3_prepare_v2(Db, CreateRunTable, -1, &CreateTableStmt, 0);
	rc = sqlite3_step(CreateTableStmt);
	rc = sqlite3_finalize(CreateTableStmt);
		
	const char *CreateParameterSetsTable =
		"CREATE TABLE ParameterSets (ParameterID INTEGER, RunID INTEGER, Value DOUBLE, FOREIGN KEY(ParameterID) REFERENCES Parameters(ID), FOREIGN KEY(RunID) REFERENCES Run(ID));";
	rc = sqlite3_prepare_v2(Db, CreateParameterSetsTable, -1, &CreateTableStmt, 0);
	rc = sqlite3_step(CreateTableStmt);
	rc = sqlite3_finalize(CreateTableStmt);
	
	const char *CreateQuantileTable =
		"CREATE TABLE Quantiles (ID INTEGER NOT NULL PRIMARY KEY, Quantile DOUBLE)";
	rc = sqlite3_prepare_v2(Db, CreateQuantileTable, -1, &CreateTableStmt, 0);
	rc = sqlite3_step(CreateTableStmt);
	rc = sqlite3_finalize(CreateTableStmt);
		
	const char *CreateDistributionTable =
		"CREATE TABLE Distribution (QuantileID INTEGER, Date BIGINT, Value DOUBLE, FOREIGN KEY(QuantileID) REFERENCES Quantiles(ID));";
	rc = sqlite3_prepare_v2(Db, CreateDistributionTable, -1, &CreateTableStmt, 0);
	rc = sqlite3_step(CreateTableStmt);
	rc = sqlite3_finalize(CreateTableStmt);
	
	
	const char *InsertParameterInfo = "INSERT INTO Parameters (ID, Name, Indexes) VALUES (?, ?, ?);";
	
	sqlite3_stmt *InsertParameterInfoStmt;
	rc = sqlite3_prepare_v2(Db, InsertParameterInfo, -1, &InsertParameterInfoStmt, 0);
	for(size_t Par = 0; Par < Setup->Calibration.size(); ++Par)
	{
		int ParID = (int)Par + 1;
		
		//TODO: Instead having the indexes as text (which is difficult to parse in an automatic system later), we should maybe instead hook the parameter ID to the structure ID you get when you export it using WriteParametersToDatabase from inca_database_io.cpp, however that is complicated..
		const char *ParameterName = Setup->Calibration[Par].ParameterName;
		rc = sqlite3_bind_int(InsertParameterInfoStmt, 1, ParID);
		rc = sqlite3_bind_text(InsertParameterInfoStmt, 2, ParameterName, -1, SQLITE_STATIC);
		char Indexes[4096];
		char *Pos = Indexes;
		for(const char *Index : Setup->Calibration[Par].Indexes)
		{
			Pos += sprintf(Pos, " \"%s\"", Index);
		}
		rc = sqlite3_bind_text(InsertParameterInfoStmt, 3, Indexes, -1, SQLITE_STATIC);
		
		rc = sqlite3_step(InsertParameterInfoStmt);
		rc = sqlite3_reset(InsertParameterInfoStmt);
	}
	rc = sqlite3_finalize(InsertParameterInfoStmt);
	
	
	const char *InsertRunInfo = "INSERT INTO Run (ID, Performance, WeightedPerformance) VALUES (?, ?, ?);";
	
	const char *InsertParameterSetInfo = "INSERT INTO ParameterSets (ParameterID, RunID, Value) VALUES (?, ?, ?);";
	
	sqlite3_stmt *InsertRunInfoStmt;
	rc = sqlite3_prepare_v2(Db, InsertRunInfo, -1, &InsertRunInfoStmt, 0);
	
	sqlite3_stmt *InsertParameterSetInfoStmt;
	rc = sqlite3_prepare_v2(Db, InsertParameterSetInfo, -1, &InsertParameterSetInfoStmt, 0);
	
	for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
	{
		int RunID = (int)Run + 1;
		double Performance = Results->RunData[Run].PerformanceMeasures[0].first;			//TODO: Support multiple objectives eventually
		double WeightedPerformance = Results->RunData[Run].PerformanceMeasures[0].second;
		rc = sqlite3_bind_int(InsertRunInfoStmt, 1, RunID);
		rc = sqlite3_bind_double(InsertRunInfoStmt, 2, Performance);
		rc = sqlite3_bind_double(InsertRunInfoStmt, 3, WeightedPerformance);
		
		rc = sqlite3_step(InsertRunInfoStmt);
		rc = sqlite3_reset(InsertRunInfoStmt);
		
		for(size_t Par = 0; Par < Setup->Calibration.size(); ++Par)
		{
			int ParID = (int)Par + 1;
			double Value = Results->RunData[Run].RandomParameters[Par].ValDouble;
			
			rc = sqlite3_bind_int(InsertParameterSetInfoStmt, 1, ParID);
			rc = sqlite3_bind_int(InsertParameterSetInfoStmt, 2, RunID);
			rc = sqlite3_bind_double(InsertParameterSetInfoStmt, 3, Value);
			
			rc = sqlite3_step(InsertParameterSetInfoStmt);
			rc = sqlite3_reset(InsertParameterSetInfoStmt);
		}
	}
	rc = sqlite3_finalize(InsertRunInfoStmt);
	rc = sqlite3_finalize(InsertParameterSetInfoStmt);
	
	
	const char *InsertQuantileInfo = "INSERT INTO Quantiles (ID, Quantile) VALUES (?, ?);";
	sqlite3_stmt *InsertQuantileInfoStmt;
	rc = sqlite3_prepare_v2(Db, InsertQuantileInfo, -1, &InsertQuantileInfoStmt, 0);
	
	const char *InsertDistributionInfo = "INSERT INTO Distribution (QuantileID, Date, Value) VALUES (?, ?, ?);";
	sqlite3_stmt *InsertDistributionInfoStmt;
	rc = sqlite3_prepare_v2(Db, InsertDistributionInfo, -1, &InsertDistributionInfoStmt, 0);
	
	s64 StartDate = GetStartDate(DataSet);
	
	for(size_t QuantileIdx = 0; QuantileIdx < Setup->Quantiles.size(); ++QuantileIdx)
	{
		int QuantileID = (int)QuantileIdx + 1;
		double Quantile = Setup->Quantiles[QuantileIdx];
		rc = sqlite3_bind_int(InsertQuantileInfoStmt, 1, QuantileID);
		rc = sqlite3_bind_double(InsertQuantileInfoStmt, 2, Quantile);
		
		rc = sqlite3_step(InsertQuantileInfoStmt);
		rc = sqlite3_reset(InsertQuantileInfoStmt);
		
		for(size_t Timestep = 0; Timestep < Results->PostDistribution[QuantileIdx].size(); ++Timestep)
		{
			s64 Date = StartDate + (s64)Timestep * 86400; //NOTE: Dependent on the size of the timestep...
			double Value = Results->PostDistribution[QuantileIdx][Timestep];
			rc = sqlite3_bind_int(InsertDistributionInfoStmt, 1, QuantileID);
			rc = sqlite3_bind_int64(InsertDistributionInfoStmt, 2, Date);
			rc = sqlite3_bind_double(InsertDistributionInfoStmt, 3, Value);
			
			rc = sqlite3_step(InsertDistributionInfoStmt);
			rc = sqlite3_reset(InsertDistributionInfoStmt);
		}
	}
	
	rc = sqlite3_finalize(InsertQuantileInfoStmt);
	rc = sqlite3_finalize(InsertDistributionInfoStmt);
	
	rc = sqlite3_exec(Db, "COMMIT;", 0, 0, 0);
	
	sqlite3_close(Db);
}
#endif