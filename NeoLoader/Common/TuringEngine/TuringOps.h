#pragma once

#include "TuringEngine.h"

class CTuringOps
{
public:
	static int				DoOperation(wstring* Accumulator, const wstring& Operator, wstring* Operand);
	static int				DoFunction(const wstring& Function, const vector<wstring>& Arguments, wstring* Return);

	static void				Register(CTuringEngine* pEngine);

	static bool				IsOperator(wchar_t Char);
	static bool				HasOperator(const wstring& String);
	#define OP_LEVELS 7
	static int				GetOperatorLevel(wstring String);

private:
	static int				FxFind		(VarPtrMap* pArgMap, SContext* pContext);
	static int				FxMatch		(VarPtrMap* pArgMap, SContext* pContext);
	static int				FxSplit		(VarPtrMap* pArgMap, SContext* pContext);
	static int				FxReplace	(VarPtrMap* pArgMap, SContext* pContext);
	static int				FxSubStr	(VarPtrMap* pArgMap, SContext* pContext);
	static int				FxInsert	(VarPtrMap* pArgMap, SContext* pContext);
	static int				FxErase		(VarPtrMap* pArgMap, SContext* pContext);
	static int				FxCompare	(VarPtrMap* pArgMap, SContext* pContext);
	static int				FxLength	(VarPtrMap* pArgMap, SContext* pContext);
};

#define XOR(a, b) ((!a) != (!b))

bool IsFalse(const wstring& String);
bool IsArgTrue(wstring* String, bool bDefault = false);
int wstring2int(wstring* String, int Default = 0);
double wstring2double(wstring* String, double Default = 0.0);