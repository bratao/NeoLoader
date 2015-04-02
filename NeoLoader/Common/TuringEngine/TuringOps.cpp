#include "GlobalHeader.h"
#include "TuringOps.h"
#include "../Common/Strings.h"
#include <math.h>

#ifndef WIN32
#ifdef __APPLE__
#undef map
#undef multimap
#include <boost/spirit/home/classic/utility/regex.hpp>
#define map _map_fix_
#define multimap _multimap_fix_
#else
#include <boost/regex.hpp>
#endif
#define rxl boost
#else
#include <regex>
#define rxl std::tr1
#endif


bool IsFalse(const wstring& String)
{
	if(String.empty())
		return true;
	if(CompareStr(String, L"false"))
		return true;
	//if(String.compare(L"0") == 0)
	if(String.at(0) == L'0' && wstring2double(String) == 0.0)
		return true;
	return false;
}

bool IsArgTrue(wstring* String, bool bDefault)
{
	if(!String)
		return bDefault;
	return !IsFalse(*String);
}

int wstring2int(wstring* String, int Default)
{
	if(!String || String->empty())
		return Default;
	return wstring2int(*String);
}

double wstring2double(wstring* String, double Default)
{
	if(!String || String->empty())
		return Default;
	return wstring2double(*String);
}

////////////////////////////////////////////////////////////////////////////////////////////
//

/*
*	Operators:	"*+-/%&^|!=<>,:?~"
*	Brackets:	"()[]"
*	Modifyers:	".#"
*	Numbers:	"0123456789"
*	Unused:		"{};$@"
*/

wstring g_Operators = L"*+-/%&^|!=<>,:?~";

bool CTuringOps::IsOperator(wchar_t Char)
{
	return g_Operators.find_first_of(Char) != wstring::npos;
}

bool CTuringOps::HasOperator(const wstring& String)
{
	return String.find_first_of(g_Operators) != wstring::npos;
}

int CTuringOps::GetOperatorLevel(wstring String)
{
	if(String.empty())
		return 0; // this is a multi expression
	// Hierarchy
	// * / !
	// + - = 
	// == != < > <= >= ~~
	// && ||
	// : ?
	if(String == L"*" || String == L"/" || String == L"!")		// high prio math op
		return 6;
	if(String == L"+" || String == L"-" || String == L"=")		// low prio math op
		return 5;
	if(String == L"&" || String == L"|")						// string op
		return 4;
	if(String == L"==" || String == L"!=" || String == L"~=" || String == L"~~"	// comparation
	|| String == L"<" || String == L">" || String == L"<=" || String == L">=")
		return 3;
	if(String == L"&&" || String == L"||")						// logic op
		return 2;
	if(String == L":" || String == L"?")						// other op
		return 1;
	return 0; // not a operator
}

/*		Turing script operator overview:
*
*	Generic:
*	L	=	R	Asignment
*	L	==	R	Comparation
*	L	!=	R	Negated Comparation
*
*	L	~=	R	Comparation Case insensitive
*	L	~~	R	WildCard Comparation Case insensitive
*
*	L	&	R	Concatinate 2 strings
*	L	|	R	Take first not empty string
*
*	Mathematical Operatos
*	L	*	R	double multiplication
*	L	+	R	double addition
*	L	-	R	double sibtraction
*	L	/	R	double division
*	L	%	R	integer Modulo
*
*	Mathematical Comparations
*	L	<	R	lower 
*	L	<=	R	lower or equal
*	L	>	R	greater
*	L	>=	R	greater or equal
*
*	Logical Operations & other operations
*		!	R	Logic NOT
*	L	&&	R	Logic AND
*	L	||	R	Logic OR
*
*	A ?	B :	C	seelction operation: if A than take B else Take C , note only the taken equasion is being evaluated
*/

int CTuringOps::DoOperation(wstring* Accumulator, const wstring& Operator, wstring* Operand)
{
  try
  {
	// Generic
	if(Operator.compare(L"=") == 0)
	{
		*Accumulator = *Operand;
	}
	else if(Operator.compare(L"==") == 0)
	{
		if(*Accumulator == *Operand)			*Accumulator = L"true";
		else									*Accumulator = L"false";
	}
	else if(Operator.compare(L"!=") == 0)
	{
		if(*Accumulator != *Operand)			*Accumulator = L"true";
		else									*Accumulator = L"false";
	}

	else if(Operator.compare(L"~=") == 0)		// compare case insensitive
	{
		if(CompareStr(*Accumulator,*Operand))	*Accumulator = L"true";
		else									*Accumulator = L"false";
	}
	else if(Operator.compare(L"~~") == 0)		// compare using wild card
	{
		if(wildcmpex(Operand->c_str(), Accumulator->c_str()))	*Accumulator = L"true";
		else													*Accumulator = L"false";
	}

	else if(Operator.compare(L"&") == 0)
	{
		Accumulator->append(*Operand);
	}
	else if(Operator.compare(L"|") == 0)		//(a | b) == (a ? a : b)
	{
		if(Accumulator->empty())				*Accumulator = *Operand;
	}

	// Mathemaic operation
	else if(Operator.compare(L"*") == 0)
	{
		*Accumulator = double2wstring(wstring2double(*Accumulator) * wstring2double(*Operand));
	}
	else if(Operator.compare(L"+") == 0)
	{
		*Accumulator = double2wstring(wstring2double(*Accumulator) + wstring2double(*Operand));
	}
	else if(Operator.compare(L"-") == 0)
	{
		*Accumulator = double2wstring(wstring2double(*Accumulator) - wstring2double(*Operand));
	}
	else if(Operator.compare(L"/") == 0)
	{
		*Accumulator = double2wstring(wstring2double(*Accumulator) / wstring2double(*Operand));
	}
	else if(Operator.compare(L"%") == 0) // in c++ this is defined only for integer
	{
		if(int By = wstring2int(*Operand))			*Accumulator = int2wstring(wstring2int(*Accumulator) % By);
		else										*Accumulator = L"NaN";
	}
	
	// mathematic comparation
	else if(Operator.compare(L"<") == 0)
	{
		if(wstring2double(*Accumulator) < wstring2double(*Operand))	*Accumulator = L"true";
		else														*Accumulator = L"false";
	}
	else if(Operator.compare(L"<=") == 0)
 	{
		if(wstring2double(*Accumulator) <= wstring2double(*Operand))*Accumulator = L"true";
		else														*Accumulator = L"false";
	}
	else if(Operator.compare(L">") == 0)
	{
		if(wstring2double(*Accumulator) > wstring2double(*Operand))	*Accumulator = L"true";
		else														*Accumulator = L"false";
	}
	else if(Operator.compare(L">=") == 0)
	{
		if(wstring2double(*Accumulator) >= wstring2double(*Operand))*Accumulator = L"true";
		else														*Accumulator = L"false";
	}

	// logic
	else if(Operator.compare(L"!") == 0)
	{
		if(IsFalse(*Operand))										*Accumulator = L"true";
		else														*Accumulator = L"false";
	}
	else if(Operator.compare(L"&&") == 0)
	{
		if(!IsFalse(*Accumulator) && !IsFalse(*Operand))			*Accumulator = L"true";
		else														*Accumulator = L"false";
	}
	else if(Operator.compare(L"||") == 0)
	{
		if(!IsFalse(*Accumulator) || !IsFalse(*Operand))			*Accumulator = L"true";
		else														*Accumulator = L"false";
	}

	// unknown
	else 
		return FX_ERR_SYNTAX;
	return FX_OK;
  }
  catch(...)
  {
	return FX_ERR_ARGUMENT;
  }
}

/*		Turing script equation function overview:
Clock()			return current clock cycle in micro seconds

Trim(a)			returns a trimmed string
Upper(a)		returns a string in uper case
Lower(a)		returns a string in lower case

Find(a, b)		returns the position of string b inside string a, or -1 (npos) is not found
Len(a)			returns the length of a
SubStr(a,b,c)	returns a sub string of a starting at the offset b with the length c, c is optional

Left(a, b)		returns b bytes from left of a
Left(a, b, c)	returns b from left untill the c'th occurence (from left) of b
Right(a, b)		returns b bytes from right of a
Right(a, b, c)	returns b from right untill the c'th occurence (also from left) of b
Token(a, b, c)	returns the c'th sub string separated by tockens b in the string a
Token(a, b)		returns the count of string separated by tockens b in the string a

FindEx(a, b)	returns the position of Regular expression b inside string a, or -1 (npos) is not found
MatchEx(a, b)	matches a with a regular expression b
GetEx(a, b)		returns the exact sub streing that matched the Regular expression b inside string a


const(a)		returns a matematical constant it knows "pi" ane "e"

cos(a)		    Compute cosine of a
sin(a)		    Compute sine of a
tan(a)		    Compute tangent of a
acos(a)		    Compute arc cosine of a
asin(a)		    Compute arc sine of a
atan(a)		    Compute arc tangent of a

cosh(a)		    Compute hyperbolic cosine of a
sinh(a)		    Compute hyperbolic sine of a
tanh(a)		    Compute hyperbolic tangent of a

exp(a)		    Compute exponential function of a
log(a)		    Compute natural logarithm of a
log10(a)		Compute common logarithm of a

pow(a, b)		Raise a to power of b
sqrt(a)		    Compute square root of a

ceil(a)		    Round up value of a
floor(a)		Round down value of a
abs(a)		    Compute absolute value of a

int(a)			Convert a to integer, drop after coma
fract(a)		Extract the after coma
mod(a,b)		Compute remainder of division a / b
*/

int CTuringOps::DoFunction(const wstring& Function, const vector<wstring>& Arguments, wstring* Return)
{
  try
  {
	if(Function.compare(L"Clock") == 0)
		*Return = double2wstring((double)GetCurTick()/100.0); // return in seconds	

	// String Functions
	else if(Function.compare(L"Trim") == 0)
		*Return = Trimm(Arguments.at(0));
	else if(Function.compare(L"Upper") == 0)
		*Return = MkUpper(Arguments.at(0));
	else if(Function.compare(L"Lower") == 0)
		*Return = MkLower(Arguments.at(0));

	else if(Function.compare(L"Find") == 0)
		*Return = int2wstring(FindStr(Arguments.at(0), Arguments.at(1)));
	else if(Function.compare(L"Len") == 0)
		*Return = int2wstring(Arguments.at(0).size());
	else if(Function.compare(L"SubStr") == 0)
		*Return = Arguments.at(0).substr(wstring2int(Arguments.at(1)), (Arguments.size() == 2) ? wstring::npos : wstring2int(Arguments.at(2)));

	else if(Function.compare(L"Left") == 0)
	{
		wstring::size_type OpOff = wstring2int(Arguments.at(1));
		if(Arguments.size() == 3)
			OpOff = FindNth(Arguments.at(0), Arguments.at(2), OpOff);
		*Return = Arguments.at(0).substr(0, OpOff);
	}
	else if(Function.compare(L"Right") == 0)
	{
		wstring::size_type OpOff = wstring2int(Arguments.at(1));
		if(Arguments.size() == 3)
			OpOff = FindNthR(Arguments.at(0), Arguments.at(2), OpOff);
		*Return = Arguments.at(0).substr(Arguments.at(0).size() - OpOff, OpOff);
	}
	else if(Function.compare(L"Token") == 0)
	{
		if(Arguments.size() == 3)
			*Return = SubStrAt(Arguments.at(0), Arguments.at(1), wstring2int(Arguments.at(2)));
		else
			*Return = int2wstring(CountSep(Arguments.at(0), Arguments.at(1)));
	}

#ifdef rxl
	else if(Function.compare(L"FindEx") == 0)
	{
		rxl::wregex Pattern;
		try{Pattern = rxl::wregex(Arguments.at(1), rxl::regex::ECMAScript | rxl::regex::icase);}
		catch(...){return FX_ERR_ARGUMENT;}
		rxl::wcmatch Result;
		*Return = rxl::regex_search(Arguments.at(0).c_str(), Result, Pattern) ? int2wstring(Result.position()) : L"-1";
	}
	else if(Function.compare(L"MatchEx") == 0)
	{
		rxl::wregex Pattern;
		try{Pattern = rxl::wregex(Arguments.at(1), rxl::regex::ECMAScript | rxl::regex::icase);}
		catch(...){return FX_ERR_ARGUMENT;}
		rxl::wcmatch Result;
		*Return = rxl::regex_match(Arguments.at(0).c_str(), Pattern) ? L"true" : L"false";
	}
	else if(Function.compare(L"GetEx") == 0)
	{
		rxl::wregex Pattern;
		try{Pattern = rxl::wregex(Arguments.at(1), rxl::regex::ECMAScript | rxl::regex::icase);}
		catch(...){return FX_ERR_ARGUMENT;}
		rxl::wcmatch Result;
		*Return = rxl::regex_search(Arguments.at(0).c_str(), Result, Pattern) ? Result.str() : L"";
	}
#endif


	// Math Functions
	else if(Function.compare(L"const") == 0)
	{
		if(Arguments.at(0).compare(L"pi") == 0)		*Return = L"3.14159265358979323846";
		else if(Arguments.at(0).compare(L"e") == 0)	*Return = L"2.71828182845904523536";
		else										*Return = L"";
	}

	else if(Function.compare(L"cos") == 0)
		*Return = double2wstring(cos(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"sin") == 0)
		*Return = double2wstring(sin(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"tan") == 0)
		*Return = double2wstring(tan(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"acos") == 0)
		*Return = double2wstring(acos(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"asin") == 0)
		*Return = double2wstring(asin(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"atan") == 0)
		*Return = double2wstring(atan(wstring2double(Arguments.at(0))));

	else if(Function.compare(L"cosh") == 0)
		*Return = double2wstring(cosh(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"sinh") == 0)
		*Return = double2wstring(sinh(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"tanh") == 0)
		*Return = double2wstring(tanh(wstring2double(Arguments.at(0))));

	else if(Function.compare(L"exp") == 0)
		*Return = double2wstring(exp(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"log") == 0)
		*Return = double2wstring(log(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"log10") == 0)
		*Return = double2wstring(log10(wstring2double(Arguments.at(0))));

	else if(Function.compare(L"pow") == 0)
		*Return = double2wstring(pow(wstring2double(Arguments.at(0)), wstring2double(Arguments.at(1))));
	else if(Function.compare(L"sqrt") == 0)
		*Return = double2wstring(sqrt(wstring2double(Arguments.at(0))));

	else if(Function.compare(L"ceil") == 0)
		*Return = double2wstring(ceil(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"floor") == 0)
		*Return = double2wstring(floor(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"abs") == 0)
		*Return = double2wstring(fabs(wstring2double(Arguments.at(0))));
	else if(Function.compare(L"int") == 0)
	{
		double Int, Fract = modf (wstring2double(Arguments.at(0)) , &Int);
		*Return = double2wstring(Int);
	}
	else if(Function.compare(L"fract") == 0)
	{
		double Int, Fract = modf (wstring2double(Arguments.at(0)) , &Int);
		*Return = double2wstring(Fract);
	}
	else if(Function.compare(L"mod") == 0)
		*Return = double2wstring(fmod(wstring2double(Arguments.at(0)), wstring2double(Arguments.at(1))));

	// unknown
	else 
		return FX_ERR_SYNTAX;
	return FX_OK;
  }
  catch(...)
  {
	return FX_ERR_ARGUMENT;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//

void CTuringOps::Register(CTuringEngine* pEngine)
{
	pEngine->RegisterFunction(L"Find",FxFind);
	pEngine->RegisterFunction(L"Match",FxMatch);
	pEngine->RegisterFunction(L"Split",FxSplit);
	pEngine->RegisterFunction(L"Replace",FxReplace);
	pEngine->RegisterFunction(L"SubStr",FxSubStr);
	pEngine->RegisterFunction(L"Insert",FxInsert);
	pEngine->RegisterFunction(L"Erase",FxErase);
	pEngine->RegisterFunction(L"Compare",FxCompare);
	pEngine->RegisterFunction(L"Length",FxLength);
}

/** Find find a sub string
* @param: Str:			Strign to search within
* @param: ICase:		Search case sensitiv
* @param: Off:			Search form offset
* @param: Pos:			Position where the sub string was found or -1 (npos) if not found
*	From the folowing parameters only one might be specifyed
* @param: Find:			String to be found
* @param: RFind:		String to be found from back
* @param: RegEx:		Regular Expression to be found
* @param: OneOf:		Find first charakter chaarkters specifyed inthe set
* @param: ROneOf:		Find last charakter chaarkters specifyed inthe set
* @param: NoneOf:		Find first chaarkter not specifyed inthe set
* @param: RNoneOf		Find last chaarkter not specifyed inthe set
*	The folowing parameters are only valif for a regexp search
* @param: Sub:			Matched sub string
* @param: Len:			Length of the matched sub string
*
*/
int CTuringOps::FxFind(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wStr		= GetArgument(pArgMap, L"Str");
	wstring* wFind		= GetArgument(pArgMap, L"Find");
	wstring* wRFind		= GetArgument(pArgMap, L"RFind");
	wstring* wRegEx		= GetArgument(pArgMap, L"RegEx");
	wstring* wICase		= GetArgument(pArgMap, L"ICase");
	wstring* wOneOf		= GetArgument(pArgMap, L"OneOf");
	wstring* wROneOf	= GetArgument(pArgMap, L"ROneOf");
	wstring* wNoneOf	= GetArgument(pArgMap, L"NoneOf");
	wstring* wRNoneOf	= GetArgument(pArgMap, L"RNoneOf");
	wstring* wOff		= GetArgument(pArgMap, L"Off");
	wstring* wSub		= GetArgument(pArgMap, L"Sub");

	wstring* wPos		= GetArgument(pArgMap, L"Pos");
	wstring* wLen		= GetArgument(pArgMap, L"Len");
	if(wStr == NULL || (wRegEx ? wPos == NULL && wSub == NULL && wLen == NULL : wPos == NULL || wSub != NULL || wLen != NULL))
		return FX_ERR_ARGUMENT;

	if(XOR(wFind != NULL, wRFind != NULL))
	{
		if(wRegEx != NULL || wOneOf != NULL || wROneOf != NULL || wNoneOf != NULL || wRNoneOf != NULL)
			return FX_ERR_ARGUMENT;

		if(wRFind)
		{
			if(IsArgTrue(wICase, true))
				*wPos = int2wstring(RFindStr(*wStr, *wFind, wstring2int(wOff,wstring::npos)));
			else
				*wPos = int2wstring(wStr->rfind(*wRFind,wstring2int(wOff,wstring::npos)));
		}
		else //if(wFind)
		{
			if(IsArgTrue(wICase, true))
				*wPos = int2wstring(FindStr(*wStr, *wFind, wstring2int(wOff,0)));
			else
				*wPos = int2wstring(wStr->find(*wFind,wstring2int(wOff,0)));
		}
	}
#ifdef rxl
	else if(wRegEx)
	{
		if(wFind != NULL || wRFind != NULL || wOneOf != NULL || wROneOf != NULL || wNoneOf != NULL || wRNoneOf != NULL)
			return FX_ERR_ARGUMENT;

		rxl::wregex::flag_type Type = rxl::regex::ECMAScript;
		if(IsArgTrue(wICase, true))
			Type |= rxl::regex::icase;
		rxl::wregex Pattern;
		try{Pattern = rxl::wregex(*wRegEx, Type);}
		catch(...){return FX_ERR_ARGUMENT;}

		wstring::size_type Offset = wstring2int(wOff,0);
		if(Offset > wStr->size())
			Offset = wStr->size();

		rxl::wcmatch Result;
		bool bFound = rxl::regex_search(wStr->c_str() + Offset, Result, Pattern);

		if(wSub)
			*wSub = bFound ? Result.str() : L"";
		if(wPos)
			*wPos = bFound ? int2wstring(Result.position() + Offset) : L"-1";
		if(wLen)
			*wLen = bFound ? int2wstring(Result.length()) : L"0";
	}
#endif
	else if(XOR(wOneOf != NULL, wROneOf != NULL))
	{
		if(wRegEx != NULL || wFind != NULL || wRFind != NULL || wNoneOf != NULL || wRNoneOf != NULL)
			return FX_ERR_ARGUMENT;

		if(wROneOf)
			*wPos = int2wstring(wStr->find_last_of(*wROneOf,wstring2int(wOff,wstring::npos)));
		else //if(wOneOf)
			*wPos = int2wstring(wStr->find_first_of(*wOneOf,wstring2int(wOff,0)));
	}
	else if(XOR(wNoneOf != NULL, wRNoneOf != NULL))
	{
		if(wRegEx != NULL || wFind != NULL || wRFind != NULL || wOneOf != NULL || wROneOf != NULL)
			return FX_ERR_ARGUMENT;

		if(wRNoneOf)
			*wPos = int2wstring(wStr->find_last_not_of(*wRNoneOf,wstring2int(wOff,wstring::npos)));
		else //if(wNoneOf)
			*wPos = int2wstring(wStr->find_first_not_of(*wNoneOf,wstring2int(wOff,0)));
	}
	else
		return FX_ERR_ARGUMENT;
	return FX_OK;
}

/** Match matches a string
* @param: Str:			Strign to be matched
* @param: ICase:		Search case sensitiv
* @param: Off:			Search form offset
* @param: Res:			was matching successful
* @param: Count:		Return Count how often the expression matched
* @param: Len:			the length from left of the Match string to match with Str
*	From the folowing parameters only one might be specifyed
* @param: Match:		String to be compared
* @param: RegEx:		Regular Expression to match
*
*/
int CTuringOps::FxMatch(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wStr		= GetArgument(pArgMap, L"Str");
	wstring* wOff		= GetArgument(pArgMap, L"Off");
	wstring* wMatch		= GetArgument(pArgMap, L"Match");
	wstring* wRegEx		= GetArgument(pArgMap, L"RegEx");
	wstring* wICase		= GetArgument(pArgMap, L"ICase");
	wstring* wLen		= GetArgument(pArgMap, L"Len");
	wstring* wRes		= GetArgument(pArgMap, L"Res");
	wstring* wCount		= GetArgument(pArgMap, L"Count");
	if(wStr == NULL || !XOR(wMatch != NULL, wRegEx != NULL) || (wRes == NULL && wCount == NULL ))
		return FX_ERR_ARGUMENT;

	wstring::size_type Offset = wstring2int(wOff,0);
	if(Offset > wStr->size())
		Offset = wStr->size();
	wstring::size_type Len = wstring2int(wLen,(int)wstring::npos);

	if(wMatch)
	{
		if(wRes)
		{
			if(IsArgTrue(wICase, true))
				*wRes = wStr->compare(Offset,Len,*wMatch,0,Len) == 0 ? L"true" : L"false";
			else
				*wRes = compareex(*wStr, Offset, Len,*wMatch,0,Len) == 0 ? L"true" : L"false";
		}
		else //if(wCount)
		{
			int Count = 0;
			for(;;)
			{
				wstring::size_type Pos;
				if(IsArgTrue(wICase, true))
					Pos = FindStr(*wStr, *wMatch);
				else
					Pos = wStr->find(*wMatch);
				if(Pos == wstring::npos)
					break;

				Count ++;
			}
			*wCount = int2wstring(Count);
		}
	}
#ifdef rxl
	else if(wRegEx)
	{
		rxl::wregex::flag_type Type = rxl::regex::ECMAScript;
		if(IsArgTrue(wICase, true))
			Type |= rxl::regex::icase;
		rxl::wregex Pattern;
		try{Pattern = rxl::wregex(*wRegEx, Type);}
		catch(...){return FX_ERR_ARGUMENT;}
		if(wRes)
			*wRes = rxl::regex_match(wStr->substr(Offset, Len).c_str(), Pattern) ? L"true" : L"false";
		else //if(wCount)
		{
			int Count = 0;
			rxl::wcmatch Result;
			rxl::wsregex_token_iterator end;
			for (rxl::wsregex_token_iterator I(wStr->begin(), wStr->end(), Pattern); I != end; I++)
			{
				Count ++;
			}
			*wCount = int2wstring(Count);
		}
	}
#endif
	else
		return FX_ERR_ARGUMENT;
	return FX_OK;
}

/** Split splits a string
* @param: Str:			Strign to be splited
* @param: ICase:		Search case sensitiv
* @param: KeepEmpty:	keep empty ub strings
* @param: Index:		Index of sub string, automaticaly incremented by 1, set to -1 if index out of bound
* @param: Sub:			current sub string
*	From the folowing parameters only one might be specifyed
* @param: Find:			separator string
* @param: RegEx:		separator Regular Expression
*
*/
int CTuringOps::FxSplit(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wStr		= GetArgument(pArgMap, L"Str");
	wstring* wFind		= GetArgument(pArgMap, L"Find");
	wstring* wRegEx		= GetArgument(pArgMap, L"RegEx");
	wstring* wICase		= GetArgument(pArgMap, L"ICase");
	wstring* wKeepEmpty = GetArgument(pArgMap, L"KeepEmpty");
	wstring* wSub		= GetArgument(pArgMap, L"Sub");
	wstring* wIndex		= GetArgument(pArgMap, L"Index");
	if(wStr == NULL || !XOR(wFind != NULL, wRegEx != NULL) || wSub == NULL || wIndex == NULL)
		return FX_ERR_ARGUMENT;

	int Index = wstring2int(wIndex);
	int Count = 0;
	wstring::size_type Pos = 0;
	wstring::size_type Sep = 0;
	wstring::size_type Len = 0;
	if(wFind)
	{
		Len = wFind->length();
		for(;Sep < wStr->size();)
		{
			if(IsArgTrue(wICase, true))
				Sep = FindStr(*wStr, *wFind, Pos);
			else
				Sep= wStr->find(*wFind, Pos);

			if(Sep == wstring::npos)
				Sep = wStr->size();

			if(Sep > Pos || IsArgTrue(wKeepEmpty))
			{
				if(Count == Index)
					break;
				Count++;
			}
			Pos = Sep + Len;
		}
	}
#ifdef rxl
	else if(wRegEx)
	{
		rxl::wregex::flag_type Type = rxl::regex::ECMAScript;
		if(IsArgTrue(wICase, true))
			Type |= rxl::regex::icase;
		rxl::wregex Pattern;
		try{Pattern = rxl::wregex(*wRegEx, Type);}
		catch(...){return FX_ERR_ARGUMENT;}

		for(;Sep < wStr->size();)
		{
			rxl::wcmatch Result;
			if(regex_search(wStr->c_str()+Pos, Result, Pattern))
			{
				Sep = Result.position();
				Len = Result.length();
			}
			else
			{
				Sep = wStr->size();
				Len = 1;
			}

			if(Sep > Pos || IsArgTrue(wKeepEmpty))
			{
				if(Count == Index)
					break;
				Count++;
			}
			Pos = Sep + Len;
		}
	}
#endif
	else
		return FX_ERR_ARGUMENT;

	*wSub = (Count == Index && Sep > Pos) ? wStr->substr(Pos,Sep-Pos) : L"";
	*wIndex = (Count == Index && Sep > Pos) ? int2wstring(Index + 1) : L"-1";
	return FX_OK;
}

/** Replace replaces a found substrings with a new string
* @param: Str:			Strign to be replaced within
* @param: ICase:		Search case sensitiv
* @param: Sub:			new string to be inserted
*	From the folowing parameters only one might be specifyed
* @param: Find:			string to be replaced
* @param: FindOne:		string to be replaced only once
* @param: RegEx:		separator Regular Expression
*
*/
int CTuringOps::FxReplace(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wStr		= GetArgument(pArgMap, L"Str");
	wstring* wFind		= GetArgument(pArgMap, L"Find");
	wstring* wFindOne	= GetArgument(pArgMap, L"FindOne");
	wstring* wRegEx		= GetArgument(pArgMap, L"RegEx");
	wstring* wICase		= GetArgument(pArgMap, L"ICase");
	wstring* wSub		= GetArgument(pArgMap, L"Sub");
	if(wStr == NULL || wSub == NULL)
		return FX_ERR_ARGUMENT;

	if(wFind || wFindOne)
	{
		if(wRegEx != NULL)
			return FX_ERR_ARGUMENT;

		if(wFindOne)
			wFind = wFindOne;

		for(;;)
		{
			wstring::size_type Pos;
			if(IsArgTrue(wICase, true))
				Pos = FindStr(*wStr, *wFind);
			else
				Pos = wStr->find(*wFind);
			if(Pos == wstring::npos)
				break;

			wStr->replace(Pos,wFind->size(),*wSub);

			if(wFindOne)
				break;
		}

		return FX_OK;
	}
#ifdef rxl
	else if(wRegEx)
	{
		if(wFind || wFindOne)
			return FX_ERR_ARGUMENT;

		rxl::wregex::flag_type Type = rxl::regex::ECMAScript;
		if(IsArgTrue(wICase, true))
			Type |= rxl::regex::icase;
		rxl::wregex Pattern;
		try{Pattern = rxl::wregex(*wRegEx, Type);}
		catch(...){return FX_ERR_ARGUMENT;}
		*wStr = rxl::regex_replace(*wStr, Pattern, *wRegEx);
	}
#endif
	else
		return FX_ERR_ARGUMENT;
	return FX_OK;
}

/** SubStr return a sub string out of a string
* @param: Str:			Strign extract teh sub string from
* @param: Sub:			returned extracted substring
* @param: Off:			start offset
* @param: Len:			length to extract (optional)
*
*/
int CTuringOps::FxSubStr(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wStr		= GetArgument(pArgMap, L"Str");
	wstring* wSub		= GetArgument(pArgMap, L"Sub");
	wstring* wOff		= GetArgument(pArgMap, L"Off");
	wstring* wLen		= GetArgument(pArgMap, L"Len");
	if(wStr == NULL || wSub == NULL)
		return FX_ERR_ARGUMENT;

	wstring::size_type Off = wstring2int(wOff,0);
	if(Off > wStr->length())
		return FX_ERR_ARGUMENT;
	wstring::size_type Len = wstring2int(wLen,wstring::npos);

	*wSub = wStr->substr(Off,Len);
	return FX_OK;
}

/** Insert insert a string into an other string
* @param: Str:			Strign to inssert into
* @param: Sub:			String to insert
* @param: Pos:			Position where to insert
* @param: Off:			starting ofset in teh subs tring from which to insert (optional)
* @param: Len:			length to insert(optional)
*
*/
int CTuringOps::FxInsert(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wStr		= GetArgument(pArgMap, L"Str");
	wstring* wPos		= GetArgument(pArgMap, L"Pos");
	wstring* wLen		= GetArgument(pArgMap, L"Len");
	wstring* wSub		= GetArgument(pArgMap, L"Sub");
	wstring* wOff		= GetArgument(pArgMap, L"Off");
	if(wStr == NULL || wPos == NULL || wSub == NULL)
		return FX_ERR_ARGUMENT;

	wStr->insert(wstring2int(wPos,0),*wSub,wstring2int(wOff,0),wstring2int(wLen,wstring::npos));
	return FX_OK;
}

/** Erase erase a part of a string
* @param: Str:			Strign to erase from
* @param: Pos:			starting ofset where to start erasing form
* @param: Len:			length to erase
*
*/
int CTuringOps::FxErase(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wStr		= GetArgument(pArgMap, L"Str");
	wstring* wPos		= GetArgument(pArgMap, L"Pos");
	wstring* wLen		= GetArgument(pArgMap, L"Len");
	if(wStr == NULL)
		return FX_ERR_ARGUMENT;

	wStr->erase(wstring2int(wPos,0),wstring2int(wLen,wstring::npos));
	return FX_OK;
}

/** Compare compare two strings
* @param: StrL:			left string to compare
* @param: OffL:			ofset to start comapring from
* @param: StrR:			right string to compare
* @param: OffR:			ofset to start comapring from
* @param: StrR:			wildcard string to compare with StrL
* @param: ICase:		Search case sensitiv
* @param: Len:			length to compare
* @param: Res:			Result
*
*/
int CTuringOps::FxCompare(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wStrL		= GetArgument(pArgMap, L"StrL");
	wstring* wOffL		= GetArgument(pArgMap, L"OffL");
	wstring* wStrR		= GetArgument(pArgMap, L"StrR");
	wstring* wOffR		= GetArgument(pArgMap, L"OffR");
	wstring* wStrW		= GetArgument(pArgMap, L"StrW");
	wstring* wICase		= GetArgument(pArgMap, L"ICase");
	wstring* wLen		= GetArgument(pArgMap, L"Len");
	wstring* wRes		= GetArgument(pArgMap, L"Res");
	if(wStrL == NULL || (wStrR == NULL && wStrW == NULL) || wRes == NULL)
		return FX_ERR_ARGUMENT;

	wstring::size_type Len = wstring2int(wLen,(int)wstring::npos);
	if(wStrW) // wildcard
		*wRes = wildcmpex(wStrW->c_str(), wStrL->c_str()) ? L"0" : L"1";
	else if(IsArgTrue(wICase, true))
		*wRes = int2wstring(wStrL->compare(wstring2int(wOffL,0),Len,*wStrR,wstring2int(wOffR,0),Len));
	else
		*wRes = int2wstring(compareex(*wStrL, wstring2int(wOffL,0), Len,*wStrR,wstring2int(wOffR,0),Len));
	return FX_OK;
}

/** Length retrun string length
* @param: Str:			Strign to measure
* @param: Len:			returned length
*
*/
int CTuringOps::FxLength(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wStr		= GetArgument(pArgMap, L"Str");
	wstring* wLen		= GetArgument(pArgMap, L"Len");
	if(wStr == NULL || wLen == NULL)
		return FX_ERR_ARGUMENT;

	*wLen = int2wstring(wStr->length());
	return FX_OK;
}

