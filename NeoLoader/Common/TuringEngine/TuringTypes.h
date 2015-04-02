#pragma once

class CTuringEngine;
class CExpressions;

typedef map<wstring,wstring> VarMap;
typedef map<wstring,wstring*> VarPtrMap;
typedef vector<wstring> stringlines;
typedef map<wstring,stringlines*> ResMap;
typedef multimap<wstring,wstring> TmpMap;

///////////////////////////////////////////////////////////////////////////////////////////////////////
// SExpression

struct SExpression
{
	SExpression(const wstring &Word);
	SExpression(CExpressions* Expressions);
	~SExpression();

	__inline bool IsWord()		{return Type == eWord;}
	__inline bool IsString()	{return Type == eString;}
	__inline bool IsMulti()		{return Type == eMulti;}
	__inline bool IsOperator()	{return Type == eOperator;}

	enum
	{
		eWord = 0,
		eOperator,
		eString,
		eMulti
	}
	Type;
	union
	{
		wstring* Word;
		CExpressions* Multi;
	}
	Exp;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
// CExpressions

class CExpressions
{
public:
	~CExpressions();

	SExpression**	GetExp(UINT Index);
	wstring			GetExpStr(UINT Index);
	bool			IsExpMulti(UINT Index);
	bool			IsExpOp(UINT Index);

	bool			SubordinateExp(int Index, int ToGo);

	void			Add(SExpression* Expression)	{m_Expressions.push_back(Expression);}
	UINT			Count()							{return m_Expressions.size();}
	void			Del(UINT Index);

protected:
	vector<SExpression*> m_Expressions;
};

struct SBlock
{
	SBlock(CExpressions* Exp, int Line = -1, bool Break = false)
		: Expressions(Exp), bBreak(Break), iLine(Line) {}

	CExpressions* Expressions;
	int	iLine;
	bool bBreak;
};
typedef vector<SBlock> scriptblocks;

///////////////////////////////////////////////////////////////////////////////////////////////////////
// STuringBlock

struct STuringBlock
{
	enum EBlockType
	{
		eNone = 0,
		eData,
		eFunction,
		eIf,
		eElse,
		eLoop,
	};

	STuringBlock();
	~STuringBlock();
	static void Push(EBlockType Type, STuringBlock* &pBlock);
	static void Pop(STuringBlock* &pBlock);

	EBlockType		eType;
	STuringBlock*	pSupBlock;
	wstring			Name;
	stringlines		Ends;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
// SOp

struct SOp
{
	SOp()
	{
		eOp = EOp_Invalid;
		iLine = -1;
	}

	~SOp()
	{
		switch(eOp)
		{
			case SOp::EOp_Label:	delete uOp.sLabel;		break;
			case SOp::EOp_Goto:		delete uOp.sGoto;		break;
			case SOp::EOp_Equation:	delete uOp.sEquation;	break;
			case SOp::EOp_Function:	delete uOp.sFunction;	break; 
		}
		eOp = EOp_Invalid;
	}

	enum EOp
	{
		EOp_Invalid		= 0,
		EOp_Label		= 1,
		EOp_Goto		= 2,
		EOp_Equation	= 3,
		EOp_Function	= 4
	}
	eOp;

	union UOp
	{
		struct SLabel
		{
			wstring			Label;
		} 
		* sLabel;

		struct SGoto
		{
			SGoto()
			{
				Conditions = NULL;
			}
			~SGoto()
			{
				delete Conditions;
			}
			wstring			Label;
			bool			Not;
			CExpressions*	Conditions;
		} 
		* sGoto;

		struct SEquation
		{
			SEquation()
			{
				Equation = NULL;
			}
			~SEquation()
			{
				delete Equation;
			}
			CExpressions*	Equation;
		} 
		* sEquation;

		struct SFunction
		{
			~SFunction()
			{
				for(vector<SArg*>::iterator I = Arguments.begin(); I != Arguments.end(); I++)
					delete *I;
			}
			wstring			Function;
			struct SArg
			{
				SArg(wstring argument, wstring assignment, SExpression* value)
				{
					Argument = argument;
					Assignment = assignment;
					Value = value;
				}
				~SArg()
				{
					delete Value;
				}
				wstring			Argument;
				wstring			Assignment;
				SExpression*	Value;
			};
			vector<SArg*>	Arguments;
		} 
		* sFunction;
	}
	uOp;

	int iLine;
};
typedef vector<SOp*> FxVector;
typedef map<wstring,FxVector*> FxMap;

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Context

// Note: if we use an empty class instead of a void* we can cast it ptoperly on sub classes to
class CContext
{
};

struct SContext
{
	SContext(CTuringEngine* Engine, CContext* Context, VarMap* GVarMap)
		: pContext(Context), pEngine(Engine), pGVarMap(GVarMap) {}

	template <class TYPE>
	TYPE*			GetContext(){if(!pContext) throw -8 /*FX_ERR_NATIVE*/; return (TYPE*)pContext;}
	CContext*		pContext;
	CTuringEngine*	pEngine;
	VarMap*			pGVarMap;
};
typedef int (*FX)(VarPtrMap* pArgMap, SContext* pContext);
typedef map<wstring,FX> ExtFxMap;

///////////////////////////////////////////////////////////////////////////////////////////////////////
// SExecutionState

struct SExecutionState
{
	SExecutionState(FxVector* fx, FxVector::iterator begin, bool preemption, uint16 depthLimit, uint64 timeLimit)
		: Fx(fx), Pointer(begin), DepthLimit(depthLimit), TimeLimit(timeLimit)
		, CallState(NULL), SubState(NULL), bDebug(false) 
	{
		NextPause = preemption ? -1 : 0;
	}
	~SExecutionState() {delete SubState; delete CallState;}

	VarMap				Variables;
	FxVector*			Fx;
	FxVector::iterator	Pointer;
	uint64				NextPause;
	uint16				DepthLimit;
	uint64				TimeLimit;
	bool				bDebug;

	struct SCallState
	{
		wstring			Function;
		VarPtrMap		Arguments;
		TmpMap			Temp;
	}*					CallState;
	SExecutionState*	SubState;
};

struct SScope
{
	SScope(SExecutionState* ExecutionState = NULL, VarPtrMap* ArgMap = NULL, VarMap* GVarMap = NULL, CContext* Context = NULL)
		: pExecutionState(ExecutionState), pArgMap(ArgMap), pGVarMap(GVarMap), pContext(Context) {}

	SExecutionState* pExecutionState;
	VarPtrMap*	pArgMap;
	VarMap*		pGVarMap;
	CContext*	pContext;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
// SDebugState

struct SDebugState
{
	enum EDebugOp
	{
		eNone = 0,
		eInterleaveOp,
		eSingleStep,
		eSkipStep,
		eStepIn,
		eStepOut,
	};

	SDebugState(EDebugOp Op)
		: DebugOp(Op), pOp(NULL), pSupState(NULL) {}
	~SDebugState() {delete pOp; delete pSupState;}

	EDebugOp			DebugOp;
	SOp*				pOp;
	SExecutionState*	pCallerState;

	SScope				Scope;
	SDebugState*		pSupState;
};

struct SBreak
{
	SBreak(int Mode = 1) 
		: iMode(Mode), Conditions(NULL) {}
	~SBreak() {delete Conditions;}

	int				iMode;
	CExpressions*	Conditions;
};
typedef map<SOp*,SBreak*> BreakMap;


//#define null wstring(L"\0",1)