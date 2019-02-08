
enum token_type
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

//NOTE: WARNING: This has to match the token_type enum!!!
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

struct token
{
	token_type Type;
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
	
	double GetDoubleValue();
	u64    GetUIntValue() { return BeforeComma; };
};

double
token::GetDoubleValue()
{
	if(this->IsNAN)
	{
		return std::numeric_limits<double>::quiet_NaN();
	}
	
	//TODO: Check that this does not create numeric errors.
	
	double BeforeComma = (double)this->BeforeComma;
	double AfterComma = (double)this->AfterComma;
	for(size_t I = 0; I < this->DigitsAfterComma; ++I) AfterComma *= 0.1;
	double Value = BeforeComma + AfterComma;
	if(this->IsNegative) Value = -Value;
	if(this->HasExponent)
	{
		//TODO: Check that exponent is not too large.
		double multiplier = this->ExponentIsNegative ? 0.1 : 10.0;
		for(u64 Exponent = 0; Exponent < this->Exponent; ++Exponent)
		{
			Value *= multiplier;
		}
	}
	return Value;
}

struct token_stream
{
	const char *Filename;
	u32 StartLine;
	u32 StartColumn;
	u32 Line;
	u32 Column;
	u32 PreviousColumn;
	
	FILE *File;
	
	std::vector<token> Tokens;
	s64 AtToken;
	
	token_stream(const char *Filename)
	{
		this->Filename = Filename;
		File = fopen(Filename, "r");
		if(!File)
		{
			INCA_FATAL_ERROR("ERROR: Tried to open file " << Filename << ", but was not able to.");
		}
		
		StartLine = 0; StartColumn = 0; Line = 0; Column = 0; PreviousColumn = 0;
		AtToken = -1;
		Tokens.reserve(500); //NOTE: This will usually speed things up.
	}
	
	~token_stream()
	{
		if(File) fclose(File);
		for(token& Token : Tokens)
		{
			if(Token.StringValue)
			{
				free((void *)Token.StringValue);
			}
		}			
	}
	
	//NOTE! Any pointer to a previously read token may be invalidated if you read or peek a new one.
	token * ReadToken();
	token * PeekToken(size_t PeekAhead = 1);
	token * ExpectToken(token_type);
	
	double ExpectDouble();
	u64    ExpectUInt();
	bool   ExpectBool();
	s64    ExpectDate();
	const char * ExpectQuotedString();
	const char * ExpectUnquotedString();
	
	void PrintErrorHeader(bool CurrentColumn=false);
	
};

static bool
IsAlpha(char c)
{
	return isalpha(c) || c == '_';
}

void
token_stream::PrintErrorHeader(bool CurrentColumn)
{
	u32 Col = StartColumn;
	if(CurrentColumn) Col = Column;
	INCA_PARTIAL_ERROR("ERROR: In file " << Filename << " line " << (StartLine+1) << " column " << (Col) << ": ");
}

static bool
MultiplyByTenAndAdd(u64 *Number, u64 Addendand)
{
	u64 MaxU64 = 0xffffffffffffffff;
	if( (MaxU64 - Addendand) / 10  < *Number) return false;
	*Number = *Number * 10 + Addendand;
	return true;
}

static token *
ReadTokenInternal_(token_stream *Stream)
{
	//TODO: we should do heavy cleanup of this function, especially the floating point reading.
	
	Stream->Tokens.resize(Stream->Tokens.size()+1, {});
	token &Token = Stream->Tokens[Stream->Tokens.size()-1];
	
	const size_t TOKEN_BUFFER_SIZE = 1024;
	
	char TokenBuffer[TOKEN_BUFFER_SIZE];
	for(size_t I = 0; I < TOKEN_BUFFER_SIZE; ++I) TokenBuffer[I] = 0;
	size_t TokenBufferPos = 0;
	
	s32 NumericPos = 0;
	
	bool TokenHasStarted = false;
	
	bool SkipLine = false;
	
	while(true)
	{
		char c = (char)fgetc(Stream->File);
		if(c == '\n')
		{
			++Stream->Line;
			Stream->PreviousColumn = Stream->Column;
			Stream->Column = 0;
		}
		else ++Stream->Column;
		
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
				Stream->PrintErrorHeader(true);
				INCA_FATAL_ERROR("Found a token of unknown type" << std::endl);
			}
			TokenHasStarted = true;
			Stream->StartLine = Stream->Line;
			Stream->StartColumn = Stream->Column;
		}
		
		if(Token.Type == TokenType_Colon || Token.Type == TokenType_OpenBrace || Token.Type == TokenType_CloseBrace)
		{
			return &Token;
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
					Stream->PrintErrorHeader();
					INCA_FATAL_ERROR("Newline within quoted string." << std::endl);
				}
				TokenBuffer[TokenBufferPos] = c;
				++TokenBufferPos;
			}
		}
		else if(Token.Type == TokenType_UnquotedString)
		{
			if(!IsAlpha(c))
			{
				char ug = ungetc((int)c, Stream->File);
				if(ug == '\n')
				{
					Stream->Line--;
					Stream->Column = Stream->PreviousColumn;
				}
				else
					Stream->Column--;
				
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
					Stream->PrintErrorHeader();
					INCA_FATAL_ERROR("Misplaced minus in numeric literal." << std::endl);
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
					Stream->PrintErrorHeader();
					INCA_FATAL_ERROR("Misplaced plus in numeric literal." << std::endl);
				}
				//ignore the plus.
			}
			else if(c == '.')
			{
				if(Token.HasExponent)
				{
					Stream->PrintErrorHeader();
					INCA_FATAL_ERROR("Comma in exponent in numeric literal." << std::endl);
				}
				if(Token.HasComma)
				{
					Stream->PrintErrorHeader();
					INCA_FATAL_ERROR("More than one comma in a numeric literal." << std::endl);
				}
				NumericPos = 0;
				Token.HasComma = true;
			}
			else if(c == 'e' || c == 'E')
			{
				if(Token.HasExponent)
				{
					Stream->PrintErrorHeader();
					INCA_FATAL_ERROR("More than one exponent sign ('e' or 'E') in a numeric literal." << std::endl);
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
						Stream->PrintErrorHeader();
						INCA_FATAL_ERROR("Too large exponent in numeric literal" << std::endl);
					}
				}
				else if(Token.HasComma)
				{
					if(!MultiplyByTenAndAdd(&Token.AfterComma, (u64)(c - '0')))
					{
						Stream->PrintErrorHeader();
						INCA_FATAL_ERROR("Numeric overflow after comma in numeric literal (too many digits)." << std::endl);
					}
					Token.DigitsAfterComma++;
				}
				else
				{
					if(!MultiplyByTenAndAdd(&Token.BeforeComma, (u64)(c - '0')))
					{
						Stream->PrintErrorHeader();
						INCA_FATAL_ERROR("Numeric overflow in numeric literal (too many digits). If this is a double, try to use scientific notation instead." << std::endl);
					}
				}
				++NumericPos;
			}
			else
			{
				char ug = ungetc((int)c, Stream->File);
				if(ug == '\n')
				{
					Stream->Line--;
					Stream->Column = Stream->PreviousColumn;
				}
				else
					Stream->Column--;
				
				break;
			}
		}
		
		if(TokenBufferPos >= TOKEN_BUFFER_SIZE)
		{
			Stream->PrintErrorHeader();
			INCA_FATAL_ERROR("ERROR: Encountered a token longer than " << TOKEN_BUFFER_SIZE << " characters." << std::endl);
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
	}
	
	return &Token;
}

static void
AssertInt(token_stream &Stream, token& Token)
{
	if(Token.HasComma || Token.HasExponent || Token.IsNAN)
	{
		Stream.PrintErrorHeader();
		INCA_FATAL_ERROR("Got a value of type double when expecting an integer." << std::endl);
	}
}

static void
AssertUInt(token_stream &Stream, token& Token)
{
	AssertInt(Stream, Token);
	if(Token.IsNegative)
	{
		Stream.PrintErrorHeader();
		INCA_FATAL_ERROR("Got a signed value when expecting an unsigned integer." << std::endl);
	}
}

token *
token_stream::ReadToken()
{
	AtToken++;
	if(AtToken >= (s64)Tokens.size())
	{
		ReadTokenInternal_(this);
	}

	return &Tokens[AtToken];
}

token *
token_stream::PeekToken(size_t PeekAhead)
{
	while(AtToken + PeekAhead >= Tokens.size())
	{
		ReadTokenInternal_(this);
	}
	return &Tokens[AtToken + PeekAhead];
}

token *
token_stream::ExpectToken(token_type Type)
{
	token *Token = ReadToken();
	if(Token->Type != Type)
	{
		PrintErrorHeader();
		INCA_PARTIAL_ERROR("Expected a token of type " << TokenNames[Type] << ", got a " << TokenNames[Token->Type]);
		if(Token->Type == TokenType_QuotedString || Token->Type == TokenType_UnquotedString)
		{
			INCA_PARTIAL_ERROR(" (" << Token->StringValue << ")");
		}
		INCA_FATAL_ERROR(std::endl);
	}
	return Token;
}

double token_stream::ExpectDouble()
{
	token *Token = ExpectToken(TokenType_Numeric);
	return Token->GetDoubleValue();
}

u64 token_stream::ExpectUInt()
{
	token *Token = ExpectToken(TokenType_Numeric);
	AssertUInt(*this, *Token);
	return Token->GetUIntValue();
}

bool token_stream::ExpectBool()
{
	token *Token = ExpectToken(TokenType_Bool);
	return Token->BoolValue;
}

s64 token_stream::ExpectDate()
{
	s64 Date;
	const char *DateStr = ExpectQuotedString();
	bool ParseSuccess = ParseSecondsSinceEpoch(DateStr, &Date);
	if(!ParseSuccess)
	{
		PrintErrorHeader();
		INCA_FATAL_ERROR("Unrecognized date format \"" << DateStr << "\". Supported format: Y-m-d" << std::endl);
	}
	return Date;
}

const char * token_stream::ExpectQuotedString()
{
	token *Token = ExpectToken(TokenType_QuotedString);
	return Token->StringValue;
}

const char * token_stream::ExpectUnquotedString()
{
	token *Token = ExpectToken(TokenType_UnquotedString);
	return Token->StringValue;
}

static void
ReadQuotedStringList(token_stream &Stream, std::vector<const char *> &ListOut, bool CopyStrings = false)
{
	Stream.ExpectToken(TokenType_OpenBrace);
	while(true)
	{
		token *Token = Stream.ReadToken();
		
		if(Token->Type == TokenType_QuotedString)
		{
			const char *Str;
			if(CopyStrings)
			{
				Str = CopyString(Token->StringValue);
			}
			else
			{
				Str = Token->StringValue;
			}
			ListOut.push_back(Str);
		}
		else if(Token->Type == TokenType_CloseBrace)
		{
			break;
		}
		else if(Token->Type == TokenType_EOF)
		{
			Stream.PrintErrorHeader();
			INCA_FATAL_ERROR("End of file before list was ended." << std::endl);
		}
		else
		{
			Stream.PrintErrorHeader();
			INCA_FATAL_ERROR("Unexpected token." << std::endl);
		}
	}
}

static void
ReadDoubleSeries(token_stream &Stream, std::vector<double> &ListOut)
{
	while(true)
	{
		token *Token = Stream.PeekToken();
		if(Token->Type != TokenType_Numeric) break;
		Token = Stream.ReadToken(); //NOTE: Consume the token we peeked.
		ListOut.push_back(Token->GetDoubleValue());
	}
}

static void
ReadUIntSeries(token_stream &Stream, std::vector<u64> &ListOut)
{
	while(true)
	{
		token *Token = Stream.PeekToken();
		if(Token->Type != TokenType_Numeric) break;
		Token = Stream.ReadToken(); //NOTE: Consume the token we peeked.
		ListOut.push_back(Token->GetUIntValue());
	}
}

static void
ReadBoolSeries(token_stream &Stream, std::vector<bool> &ListOut)      //NOOOO! We run into the std::vector<bool> template specialisation!! It is probably not too big of a deal though.
{
	while(true)
	{
		token *Token = Stream.PeekToken();
		if(Token->Type != TokenType_Bool) break;
		Token = Stream.ReadToken(); //NOTE: Consume the token we peeked.
		ListOut.push_back(Token->BoolValue);
	}
}

static void
ReadParameterSeries(token_stream &Stream, std::vector<parameter_value> &ListOut, parameter_type Type)
{
	parameter_value Value;
	
	if(Type == ParameterType_Double)
	{
		while(true)
		{
			token *Token = Stream.PeekToken();
			if(Token->Type != TokenType_Numeric) break;
			Value.ValDouble = Stream.ExpectDouble();
			ListOut.push_back(Value);
		}
	}
	else if(Type == ParameterType_UInt)
	{
		while(true)
		{
			token *Token = Stream.PeekToken();
			if(Token->Type != TokenType_Numeric) break;
			Value.ValUInt = Stream.ExpectUInt();
			ListOut.push_back(Value);
		}
	}
	else if(Type == ParameterType_Bool)
	{
		while(true)
		{
			token *Token = Stream.PeekToken();
			if(Token->Type != TokenType_Bool) break;
			Value.ValBool = Stream.ExpectBool();
			ListOut.push_back(Value);
		}
	}
	else assert(0);  //NOTE: This should be caught by the library implementer. Signifies that a new parameter type was added without being handled here
	
	//NOTE: Date values have to be handled separately since we can't distinguish them from quoted strings...
	// TODO: Make separate format for dates?
}
