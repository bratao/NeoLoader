#pragma once

#define USE_JSON

#include "TuringTypes.h"

class CTuringParser;

#define DEPTH_LIMIT				500		// actually above 560 it causes a stack overflow
#ifdef _DEBUG
 #define TIME_LIMIT				20000	// 10 seconds
#else
 #define TIME_LIMIT				10000	// 5 seconds
#endif
#define PREEMPTION_DELAY		100

#define FX_PAUSE				4	// Script Preamption Pause
#define FX_TERMINATE			3	// Script Termination
#define FX_BREAK				2	// Script Breakpoint
#define FX_INTERRUPT			1	// Function Interrupt
#define FX_OK					0
#define FX_ERR_NOTFOUND			-1	// Function Not Found
#define FX_ERR_ARGUMENT			-2	// Invalid Arguments
#define FX_ERR_SYNTAX			-3	// invalid equasion syntax
#define FX_ERR_INTERRUPT		-4	// Interrupt not enabled or interrupt state is invalid
#define FX_ERR_FLOW				-5	// invalid goto label
#define FX_ERR_STACK			-6	// stack limit reached
#define FX_ERR_TIME				-7	// execution time limit reached
#define FX_ERR_NATIVE			-8	// native function failed
#define FX_ERR_CALL				-9	// Called function failed

//#define LEGACY_SUPPORT

class CTuringEngine
{
public:
	CTuringEngine();
	virtual ~CTuringEngine();

	virtual int				EnterFunction(const wstring& FxName, VarPtrMap* pArgMap, CContext* pContext, SExecutionState** pResumeState, VarMap* pGVarMap, bool Preemption = false, uint16 DepthLimit = DEPTH_LIMIT, uint64 TimeLimit = TIME_LIMIT);
	virtual bool			RegisterFunction(const wstring& FxName, FX Fx);
	virtual int				CallFunction(wstring FxName, VarPtrMap* pArgMap, CContext* pContext, VarMap* pGVarMap);

	static	wstring*		GetArgument(VarPtrMap* pArgMap, const wstring& Name);

	virtual void			Log(wstring Line, CContext* pContext = NULL) {}

protected:
	friend class CTuringParser;

	virtual void			Cleanup();

	virtual wstring*		GetVariable(wstring Name, SScope& Scope, TmpMap& Temp);
#ifdef USE_JSON
	virtual void			SetVariables(SScope& Scope, TmpMap& Temp);
#endif
	virtual void			SetVariable(wstring Name, wstring Value, SScope& Scope);
	virtual int				DoEquation(wstring* Accumulator, CExpressions* Expressions, SScope& Scope);
	//virtual bool			DoEquation(wstring* Result, wstring Equation, SScope& Scope);
	virtual FxVector*		GetFunction(const wstring& FxName);
	virtual wstring			GetFxName(FxVector* Fx);
	virtual bool			IsValidFunction(FxVector* Fx);
	virtual bool			IsValidPointer(FxVector* Fx, FxVector::iterator Pointer);
	virtual int				ExecuteFunction(SScope& Scope);
	virtual int				PrepareCall(SOp* Op, SScope& Scope);
	virtual void			FinishCall(SScope& Scope);
	virtual int				ExecuteOp(SOp* Op, SScope& Scope);

	virtual wstring			GetScriptLine(int iLine);

	virtual int				PrepareDebug(const wstring& FxName, SExecutionState* pExecutionState, VarPtrMap* pArgMap, VarMap* pGVarMap, CContext* pContext, SDebugState* &pDebugState, SDebugState::EDebugOp DebugOp, SOp* pOp = NULL);
	virtual int				ExecuteDebug(SDebugState* &pDebugState);

	virtual bool			BreaksEnabled() {return false;}
	virtual void			ClearBreak(SOp* Op);
	virtual void			SetBreak(SOp* Op, int Mode, CExpressions* Conditions = NULL);
	virtual SBreak*			GetBreak(SOp* Op);
	virtual bool			BreakOp(SBreak* pBreak, SOp* Op, SScope& Scope);

	ExtFxMap				m_ExtFxMap;
	FxMap					m_FxMap;
	ResMap					m_ResMap;
	BreakMap				m_BreakPoints;

private:
	static int				FxData	(VarPtrMap* pArgMap, SContext* pContext);
	static int				FxQuit	(VarPtrMap* pArgMap, SContext* pContext);
};

wstring EscapeString(wstring str);

wstring* GetArgument(VarPtrMap* pArgMap, const wstring& Name);
