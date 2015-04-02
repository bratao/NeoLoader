#include "GlobalHeader.h"
#include "TuringEngine.h"
#include "TuringOps.h"
#include "TuringParser.h"
#include "../Common/Strings.h"

#ifdef USE_JSON
#include "Json.h"
#endif

CTuringEngine::CTuringEngine()
{
	RegisterFunction(L"Data",FxData);
	RegisterFunction(L"Quit",FxQuit);

	CTuringOps::Register(this);
}

CTuringEngine::~CTuringEngine()
{
	Cleanup();

	m_ExtFxMap.clear();
}

void CTuringEngine::Cleanup()
{
	for(FxMap::iterator I = m_FxMap.begin(); I != m_FxMap.end(); I++)
	{
		for(FxVector::iterator J = I->second->begin(); J != I->second->end(); J++)
			delete (*J);
		delete I->second;
	}
	m_FxMap.clear();

	for(ResMap::iterator I = m_ResMap.begin(); I != m_ResMap.end(); I++)
		delete I->second;
	m_ResMap.clear();

	for(BreakMap::iterator I = m_BreakPoints.begin(); I != m_BreakPoints.end(); I++)
		delete I->second;
	m_BreakPoints.clear();
}

wstring* CTuringEngine::GetVariable(wstring Name, SScope& Scope, TmpMap& Temp)
{
	if(Name.empty())
		return NULL;

	bool bHash = false;
	if(Name.at(0) == L'#')
	{
		Name.erase(0,1);
		bHash = true;
	}
	bool bGlobal = Name.size() > 1 && Name.at(0) == L'.';

	if(Name.at(bGlobal ? 1 : 0) == L'[' && Scope.pArgMap->find(Name) == Scope.pArgMap->end()) // is a reference
	{
		wstring* Reference = GetVariable(Name.substr(bGlobal ? 2 : 1,Name.size()-(bGlobal ? 3 : 2)), Scope, Temp);
		if(!Reference || Reference->empty())
			return NULL;
		Name = *Reference;
	}

	if(Name.at(0) == L'\"') // is a constant string
		return &Temp.insert(TmpMap::value_type(L"", Name.substr(1,Name.size()-2)))->second;
	else if (iswdigit(Name.at(0)) || Name.compare(L"true") == 0 || Name.compare(L"false") == 0) // is a constant number
		return &Temp.insert(TmpMap::value_type(L"", Name))->second;
	else if(Name.compare(L"npos") == 0)
		return &Temp.insert(TmpMap::value_type(L"", L"-1"))->second;
	//else if(Name.compare(L"null") == 0)
	//	return &Temp.insert(TmpMap::value_type(L"", null))->second;

#ifdef USE_JSON
	wstring SubName;
	wstring::size_type Pos = Name.find(L'.',1);
	if(Pos != wstring::npos)
	{
		SubName = Name.substr(Pos);
		Name.erase(Pos);
	}
#endif

	wstring* Variable = NULL;
	VarMap::iterator found = (bGlobal ? Scope.pGVarMap : &Scope.pExecutionState->Variables)->find(Name);
	if(found != (bGlobal ? Scope.pGVarMap : &Scope.pExecutionState->Variables)->end())
		Variable = &found->second;
	else
	{
		VarPtrMap::iterator pfound = Scope.pArgMap->find(Name);
		if(pfound != Scope.pArgMap->end())
			Variable = pfound->second;
	}

	if(!Variable)
	{
		if(bHash)
			return &Temp.insert(TmpMap::value_type(L"", L"false"))->second;
		Variable = &(bGlobal ? Scope.pGVarMap : &Scope.pExecutionState->Variables)->insert(VarMap::value_type(Name, L"")).first->second;
	}

#ifdef USE_JSON
	if(!SubName.empty())
	{
		wstring FullName = Name;
		vector<wstring> StrList;
		CTuringParser::SplitBlocks(SubName, StrList, true);
		//vector<wstring> StrList = SplitBlocks(SubName, true);
		wstring Return;

		JSONValue* Root = JSON::Parse(Variable->c_str());
		if(!Root) Root = new JSONValue();
		JSONValue* Item = Root;
		for(vector<wstring>::iterator I = StrList.begin(); I != StrList.end(); I++)
		{
			if(I->empty())
				return NULL;
			wstring Member = I->substr(1);
			if(Member.at(0) == L'[')
			{
				if(Member == L"[]")
					continue;
				else if(wstring* Reference = GetVariable(Member.substr(1,Member.size()-2), Scope, Temp))
				{
					Member = *Reference;
					if(Member.empty())
						Member = L"0";
				}
			}
			
			if(!Item)
			{
				FullName += L"." + Member;
				continue;
			}

			if(iswdigit(Member.at(0))) // array or object
			{
				int Index = wstring2int(Member);
				JSONObject* Obj = Item->AsObject();
				JSONArray* Arr = Item->AsArray();
				if(Obj && Index >= 0 && Index < Obj->size())
				{
					JSONObject::iterator I = Obj->begin();
					while(Index-->0)
						I++;
					//Item = I->second;
					//FullName += L"." + I->first;
					Return = I->first;
					break;
				}
				else 
				{
					FullName += L"." + Member;
					if(Arr && Index >= 0 && Index < Arr->size())
						Item = (*Arr)[Index];
					else
						Item = NULL;
				}
			}
			else // object
			{
				FullName += L"." + Member;
				JSONObject* Obj = Item->AsObject();
				if(Obj)
					Item = (*Obj)[Member];
				else
					Item = NULL;
			}

			if(!Item && bHash)
			{
				Return = L"false";
				break;
			}
		}

		if(bHash)
		{
			if(!Item)
				Return = L"0";
			else if(Item->AsArray())
				Return = int2wstring(Item->AsArray()->size());
			else if(Item->AsObject())
				Return = int2wstring(Item->AsObject()->size());
		}

		if(!Return.empty())
		{
			delete Root;
			return &Temp.insert(TmpMap::value_type(L"", Return))->second;
		}

		if(!Item)
			Variable = &Temp.insert(TmpMap::value_type(FullName, L""))->second;
		else if(Item->AsArray() || Item->AsObject())
			Variable = &Temp.insert(TmpMap::value_type(FullName, Item->Stringify()))->second;
		else
			Variable = &Temp.insert(TmpMap::value_type(FullName, Item->ToString()))->second;
		delete Root;
	}
#endif
	if(bHash)
		return &Temp.insert(TmpMap::value_type(L"", L"true"))->second;
	return Variable;
}

#ifdef USE_JSON
void CTuringEngine::SetVariables(SScope& Scope, TmpMap& Temp)
{
	for(TmpMap::iterator I = Temp.begin(); I != Temp.end(); I++)
	{
		if(I->first.empty())
			continue;
		SetVariable(I->first, I->second, Scope);
	}
}
#endif

void CTuringEngine::SetVariable(wstring Name, wstring Value, SScope& Scope)
{
	ASSERT(!Name.empty());

	bool bGlobal = Name.size() > 1 && Name.at(0) == L'.';

#ifdef USE_JSON
	wstring SubName;
	wstring::size_type Pos = Name.find(L'.',1);
	if(Pos != wstring::npos)
	{
		SubName = Name.substr(Pos);
		Name.erase(Pos);
	}
#endif

	wstring* Variable = NULL;
	VarMap::iterator found = (bGlobal ? Scope.pGVarMap : &Scope.pExecutionState->Variables)->find(Name);
	if(found != (bGlobal ? Scope.pGVarMap : &Scope.pExecutionState->Variables)->end())
		Variable = &found->second;
	else
	{
		VarPtrMap::iterator pfound = Scope.pArgMap->find(Name);
		if(pfound != Scope.pArgMap->end())
			Variable = pfound->second;
	}

	if(!Variable)
		Variable = &(bGlobal ? Scope.pGVarMap : &Scope.pExecutionState->Variables)->insert(VarMap::value_type(Name, L"")).first->second;

#ifdef USE_JSON
	if(!SubName.empty())
	{
		vector<wstring> StrList;
		CTuringParser::SplitBlocks(SubName, StrList, true);
		//vector<wstring> StrList = SplitBlocks(SubName, true);

		JSONValue* Root = JSON::Parse(Variable->c_str());
		JSONValue** Item = &Root;
		for(vector<wstring>::iterator I = StrList.begin(); I != StrList.end(); I++)
		{
			ASSERT(!I->empty());
			wstring Member = I->substr(1);

			if(iswdigit(Member.at(0))) // array
			{
				int Index = wstring2int(Member);
				JSONArray* Arr = *Item ? (*Item)->AsArray() : NULL;
				if(!Arr)
				{
					delete *Item;
					*Item = new JSONValue(JSONType_Array);
					Arr = (*Item)->AsArray();
				}
			
				ASSERT(Index >= 0);
				while(Index >= Arr->size())
					Arr->push_back(new JSONValue());

				Item = &(*Arr)[Index];
			}
			else // object
			{
				JSONObject* Obj = *Item ? (*Item)->AsObject() : NULL;
				if(!Obj)
				{
					delete *Item;
					*Item = new JSONValue(JSONType_Object);
					Obj = (*Item)->AsObject();
				}

				Item = &(*Obj)[Member];
			}
		}

		delete *Item;
		*Item = JSON::Parse(Value.c_str());
		if(!*Item) *Item = new JSONValue(Value.c_str());

		Value = Root->Stringify();

		delete Root;
	}
#endif

	*Variable = Value;
}

int CTuringEngine::DoEquation(wstring* Accumulator, CExpressions* Expressions, SScope& Scope)
{
#ifdef _DEBUG_EQ
	wstring AccumulatorName = L"temp";
	wstring OperandName = L"none";
#endif
	TmpMap Temp;
	UINT Counter = 0;
	wstring Operator;
	if(Accumulator)
		Operator = L"=";
	else
	{
#ifdef _DEBUG_EQ
		AccumulatorName = Expressions->GetExpStr(0);
#endif
		if(Expressions->IsExpOp(0))
			return FX_ERR_SYNTAX;
		Accumulator = GetVariable(Expressions->GetExpStr(0),Scope,Temp);
		if(!Accumulator)
			return FX_ERR_ARGUMENT;
		Operator = Expressions->GetExpStr(1);
		Counter = 2;
	}
	SExpression** Expression;
	SExpression** SubExpression;
	wstring* Operand;

	for(;;)
    {
		if(Operator == L"?")
		{
			Operator = L"=";
			if(IsFalse(*Accumulator))
			{
				if(Expressions->GetExpStr((++Counter)++) != L":")
					return FX_ERR_SYNTAX;
				continue;
			}
		}
		else if(Operator == L":")
		{
			Operator = Expressions->GetExpStr((++Counter)++);
			if(!Operator.empty())
				return FX_ERR_SYNTAX;
			break;
		}

		Expression = Expressions->GetExp(Counter++);
		if(!Expression)
			return FX_ERR_ARGUMENT;

		if((*Expression)->IsMulti())
		{
			if((*Expression)->Exp.Multi->GetExpStr(1) == L"=")
			{
				SubExpression = (*Expression)->Exp.Multi->GetExp(0);
				if((*SubExpression)->IsMulti() || (*SubExpression)->IsOperator())
					return FX_ERR_SYNTAX;
#ifdef _DEBUG_EQ
				OperandName = *(*SubExpression)->Exp.Word;
#endif
				Operand = GetVariable(*(*SubExpression)->Exp.Word,Scope,Temp);
				if(!Operand)
					return FX_ERR_ARGUMENT;

			}
			else
			{
#ifdef _DEBUG_EQ
				OperandName = L"temp";
#endif
				Operand = &Temp.insert(TmpMap::value_type(L"", L""))->second;
			}

			if(int Ret = DoEquation(Operand,(*Expression)->Exp.Multi,Scope))
				return Ret;
		}
		else
		{
			if((*Expression)->IsOperator())
			{
				if(Counter == 1) // only a = (!b) is valid, not a = !b
				{
					Operator = *(*Expression)->Exp.Word;
					continue;
				}
				return FX_ERR_SYNTAX;
			}

#ifdef _DEBUG_EQ
			OperandName = *(*Expression)->Exp.Word;
#endif
			if(Expressions->IsExpMulti(Counter)) // this is a simple function
			{
				vector<wstring> Arguments;
				CExpressions* SubExpressions = (*Expressions->GetExp(Counter++))->Exp.Multi;
				for(int i=0; i<SubExpressions->Count(); i++)
				{
					if(!SubExpressions->IsExpMulti(i))
						return FX_ERR_ARGUMENT;

					Arguments.push_back(L"");
					if(int Ret = DoEquation(&Arguments.back(),(*SubExpressions->GetExp(i))->Exp.Multi, Scope))
						return Ret;
						
				}

				Operand = &Temp.insert(TmpMap::value_type(L"", L""))->second;
				if(int Ret = CTuringOps::DoFunction(*(*Expression)->Exp.Word, Arguments, Operand))
					return Ret;
			}
			else
			{
				Operand = GetVariable(*(*Expression)->Exp.Word,Scope,Temp);
				if(!Operand)
					return FX_ERR_ARGUMENT;
			}
		}

#ifdef _DEBUG_EQ
		TRACE(_T("%s: %s %s %s: %s\n"),AccumulatorName.c_str(),Accumulator->c_str(),Operator.c_str(),OperandName.c_str(),Operand->c_str());
#endif
		if(int Ret = CTuringOps::DoOperation(Accumulator,Operator,Operand))
			return Ret;

		Expression = Expressions->GetExp(Counter++);
		if(!Expression)
			break; // we are done
		if(!(*Expression)->IsOperator())
			return FX_ERR_SYNTAX;
		Operator = *(*Expression)->Exp.Word;
	}

#ifdef USE_JSON
	SetVariables(Scope,Temp);
#endif
	return FX_OK;
}

/*bool CTuringEngine::DoEquation(wstring* Result, wstring Equation, SScope& Scope)
{
	if(Equation.empty())
		return false;

	if(Equation.at(0) == L'$')
	{
		TmpMap Temp;
		wstring* Reference = GetVariable(Equation.substr(1), Scope, Temp);
		Equation = Reference ? *Reference : L"";
	}

	int Ret = FX_ERR_SYNTAX;
	if(expressions* Expressions = GetExpressions(Equation))
	{
		if(OrderEquation(Expressions,true))
			Ret = DoEquation(Result, Expressions, Scope);
		delete Expressions;
	}

	if(Ret != FX_OK)
	{
		Log(_T("Invalid Equation: \"") + Equation + _T("\" with ") +
			((Ret == FX_ERR_ARGUMENT) ? _T("Invalid Arguments") : _T("Invalid Syntax")), Scope.pContext);
		return false;
	}
	return true;
}*/

FxVector* CTuringEngine::GetFunction(const wstring& FxName)
{
	FxMap::const_iterator I = m_FxMap.find(FxName);
	if(I == m_FxMap.end())
		return NULL;
	return I->second;
}

wstring CTuringEngine::GetFxName(FxVector* Fx)
{
	for(FxMap::iterator I = m_FxMap.begin(); I != m_FxMap.end(); I++)
	{
		if(I->second == Fx)
			return I->first;
	}
	return L"Unknown";
}

int CTuringEngine::EnterFunction(const wstring& FxName, VarPtrMap* pArgMap, CContext* pContext, SExecutionState** pResumeState, VarMap* pGVarMap, bool Preemption, uint16 DepthLimit, uint64 TimeLimit)
{
	FxVector* Fx = GetFunction(FxName);
	if(!Fx)
		return FX_ERR_NOTFOUND;

	if(DepthLimit == 0)
	{
		Log(_T("Function: \"") + FxName + _T("\" can not be called; stack limit reached."), pContext);
		return FX_ERR_STACK;
	}

	SExecutionState* pExecutionState = pResumeState ? *pResumeState : NULL;
	if(pExecutionState == NULL)
		pExecutionState = new SExecutionState(Fx, Fx->begin(),Preemption,--DepthLimit,TimeLimit);

	if(pGVarMap == NULL) 
		pGVarMap = &pExecutionState->Variables;

	SScope Scope(pExecutionState,pArgMap,pGVarMap,pContext);
	int Ret = ExecuteFunction(Scope);
	if(Ret > FX_OK && Ret != FX_TERMINATE)
	{
		if(!pResumeState)
		{
			delete pExecutionState;
			Log(_T("Function: \"") + FxName + _T("\" can not handle interrupt requests"), pContext);
			return FX_ERR_INTERRUPT;
		}	
		*pResumeState = pExecutionState;
	}
	else
	{
		if(pResumeState)
			*pResumeState = NULL;
		delete pExecutionState;
	}
	return Ret;
}

bool CTuringEngine::IsValidFunction(FxVector* Fx)
{
	for(FxMap::iterator I = m_FxMap.begin(); I != m_FxMap.end(); I++)
	{
		if(I->second == Fx)
			return true;
	}
	return false;
}

bool CTuringEngine::IsValidPointer(FxVector* Fx, FxVector::iterator Pointer)
{
#if _ITERATOR_DEBUG_LEVEL == 2
	if (Pointer._Getcont() == Fx) 
#endif
	for(FxVector::iterator I = Fx->begin(); I != Fx->end(); I++)
	{
		if(I == Pointer)
			return true;
	}
	return false; // invalid pointer
}

int CTuringEngine::ExecuteFunction(SScope& Scope)
{
	// Note: if we are using libraries thay may get changed we need to verify the pointer on each entrance
	if(!IsValidPointer(Scope.pExecutionState->Fx,Scope.pExecutionState->Pointer))
	{
		Log(_T("Resume failed, script state is not longer valid"), Scope.pContext);
		return FX_ERR_INTERRUPT;
	}

	if(Scope.pExecutionState->NextPause)
		Scope.pExecutionState->NextPause = GetCurTick() + PREEMPTION_DELAY;

	int Ret = FX_OK;
	Scope.pExecutionState->TimeLimit += GetCurTick();
	for(; Scope.pExecutionState->Pointer != Scope.pExecutionState->Fx->end(); Scope.pExecutionState->Pointer++)
	{
		if(Scope.pExecutionState->TimeLimit < GetCurTick())
		{
			Log(_T("Function: \"") + GetFxName(Scope.pExecutionState->Fx) + _T("\" hast to be terminated; time limit reached."), Scope.pContext);
			Ret = FX_ERR_TIME; 
			break;
		}

		SOp* Op = *Scope.pExecutionState->Pointer;

		if(SBreak* pBreak = GetBreak(Op))
		{
			if(Scope.pExecutionState->bDebug) // if this is true it means that we are resuming the breakpoint
				Scope.pExecutionState->bDebug = false;
			else if(BreakOp(pBreak, Op, Scope))
			{
				Scope.pExecutionState->bDebug = true;
				Ret = FX_BREAK;
				break;
			}	
		}

		if(Scope.pExecutionState->NextPause && Scope.pExecutionState->NextPause < GetCurTick())
		{
			Ret =  FX_PAUSE;
			break;
		}
		
		Ret = ExecuteOp(Op, Scope);
		if(Ret != FX_OK)
			break;
	}
	Scope.pExecutionState->TimeLimit -= GetCurTick();
	return Ret;
}

int CTuringEngine::PrepareCall(SOp* Op, SScope& Scope)
{
	Scope.pExecutionState->CallState = new SExecutionState::SCallState;

	Scope.pExecutionState->CallState->Function = Op->uOp.sFunction->Function;
	ASSERT(!Scope.pExecutionState->CallState->Function.empty());
	if(Scope.pExecutionState->CallState->Function.at(0) == L'[') // is this a function pointer
	{
		TmpMap Temp;
		wstring Function = Scope.pExecutionState->CallState->Function;
		wstring* Reference = GetVariable(Function.substr(1,Function.size()-2), Scope, Temp);
		if(Reference && !Reference->empty())
			Scope.pExecutionState->CallState->Function = *Reference;
	}

	for(vector<SOp::UOp::SFunction::SArg*>::iterator I = Op->uOp.sFunction->Arguments.begin(); I != Op->uOp.sFunction->Arguments.end(); I++)
	{
		SExpression* Val = (*I)->Value;

		wstring* pValue;
		if(Val->IsMulti())
		{
			pValue = &Scope.pExecutionState->CallState->Temp.insert(TmpMap::value_type(L"", L""))->second;
			if(int Ret = DoEquation(pValue, Val->Exp.Multi, Scope))
			{
				Log(_T("Function: ") + GetFxName(Scope.pExecutionState->Fx) + _T(" encountered an error, Line: ") + GetScriptLine(Op->iLine) + 
					_T("; invalid argument equasion: ") + (*I)->Argument + _T("; with ") + 
					((Ret == FX_ERR_ARGUMENT) ? _T("Invalid Arguments") : _T("Invalid Syntax")), Scope.pContext);
				return Ret;
			}
		}
		else
		{
			pValue = GetVariable(*(Val->Exp.Word), Scope, Scope.pExecutionState->CallState->Temp);
			if(pValue == NULL)
			{
				Log(_T("Function: ") + GetFxName(Scope.pExecutionState->Fx) + _T(" encountered an error, Line: ") + GetScriptLine(Op->iLine) + 
					_T("; invalid argument: ") + (*I)->Argument, Scope.pContext);
				return FX_ERR_ARGUMENT;
			}
			if((*I)->Assignment == L":=")
				pValue->clear();
		}
		Scope.pExecutionState->CallState->Arguments.insert(VarPtrMap::value_type((*I)->Argument,pValue));
	}

	return FX_OK;
}

void CTuringEngine::FinishCall(SScope& Scope)
{
#ifdef USE_JSON
	SetVariables(Scope, Scope.pExecutionState->CallState->Temp);
#endif
	delete Scope.pExecutionState->CallState;
	Scope.pExecutionState->CallState = NULL;
}

int CTuringEngine::ExecuteOp(SOp* Op, SScope& Scope)
{
	switch(Op->eOp)
	{
		case SOp::EOp_Label: 
			break; // Labels dont do anything
		case SOp::EOp_Goto: 
		{
			if(Op->uOp.sGoto->Conditions)
			{
				wstring Result;
				if(int Ret = DoEquation(&Result, Op->uOp.sGoto->Conditions, Scope))
				{
					Log(_T("Function: ") + GetFxName(Scope.pExecutionState->Fx) + _T(" encountered an error, Line: ") + GetScriptLine(Op->iLine) + 
						_T("; goto condition has ") + ((Ret == FX_ERR_ARGUMENT) ? _T("Invalid Arguments") : _T("Invalid Syntax")), Scope.pContext);
					return Ret;
				}

				if(IsFalse(Result) != Op->uOp.sGoto->Not)
					break;
			}

			FxVector::iterator Walker = Scope.pExecutionState->Fx->begin();
			while(Walker != Scope.pExecutionState->Fx->end())	
			{
				if((*Walker)->eOp == SOp::EOp_Label)
				{
					if((*Walker)->uOp.sLabel->Label == Op->uOp.sGoto->Label)
					{
						Scope.pExecutionState->Pointer = Walker;
						break;
					}
				}
				Walker++;
			}
			if(Walker == Scope.pExecutionState->Fx->end())
			{
				Log(_T("Function: ") + GetFxName(Scope.pExecutionState->Fx) + _T(" encountered an error, Line: ") + GetScriptLine(Op->iLine) + 
					_T("; invalid goto label: ") + Op->uOp.sGoto->Label, Scope.pContext);
				return FX_ERR_FLOW; 
			}
			break;
		}

		case SOp::EOp_Equation:
		{
			if(int Ret = DoEquation(NULL, Op->uOp.sEquation->Equation, Scope))
			{
				Log(_T("Function: ") + GetFxName(Scope.pExecutionState->Fx) + _T(" encountered an error, Line: ") + GetScriptLine(Op->iLine) + 
					_T("; equation has ") + ((Ret == FX_ERR_ARGUMENT) ? _T("Invalid Arguments") : _T("Invalid Syntax")), Scope.pContext);
				return Ret;
			}
			break;
		}

		case SOp::EOp_Function: 
		{
			if(Scope.pExecutionState->CallState == NULL)
			{
				if(int Ret = PrepareCall(Op, Scope))
					return Ret;
			}

			Scope.pExecutionState->TimeLimit -= GetCurTick();
			int Ret = CallFunction(Scope.pExecutionState->CallState->Function, &Scope.pExecutionState->CallState->Arguments, Scope.pContext, Scope.pGVarMap);
			if(Ret == FX_ERR_NOTFOUND)
				Ret = EnterFunction(Scope.pExecutionState->CallState->Function, &Scope.pExecutionState->CallState->Arguments, Scope.pContext, &Scope.pExecutionState->SubState, Scope.pGVarMap, Scope.pExecutionState->NextPause != 0, Scope.pExecutionState->DepthLimit, Scope.pExecutionState->TimeLimit);
			Scope.pExecutionState->TimeLimit += GetCurTick();
			if(Ret > FX_OK)	// function interrupted
				return Ret;

			FinishCall(Scope);

			if(Ret < FX_OK)
			{
				Log(_T("Function: ") + GetFxName(Scope.pExecutionState->Fx) + _T(" encountered an error, Line: ") + GetScriptLine(Op->iLine) + 
					(Ret == FX_ERR_NOTFOUND ? _T("; Function: ") + Op->uOp.sFunction->Function + _T(" Not Found") : _T("")), Scope.pContext);
				return Ret == FX_ERR_NOTFOUND ? FX_ERR_CALL : Ret; 
			}
			break;
		}
	}

	return FX_OK;
}

bool CTuringEngine::RegisterFunction(const wstring& FxName, FX Fx)
{
	if(m_ExtFxMap.find(FxName) != m_ExtFxMap.end())
		return false;
	m_ExtFxMap.insert(ExtFxMap::value_type(FxName, Fx));
	return true;
}

int CTuringEngine::CallFunction(wstring FxName, VarPtrMap* pArgMap, CContext* pContext, VarMap* pGVarMap)
{
	ExtFxMap::const_iterator I = m_ExtFxMap.find(FxName);
	if(I == m_ExtFxMap.end())
		return FX_ERR_NOTFOUND;
	SContext Context(this, pContext, pGVarMap);
  try
  {
	return I->second(pArgMap, &Context);
  }
  catch(int Err)
  {
	return Err;
  }
}

wstring* CTuringEngine::GetArgument(VarPtrMap* pArgMap, const wstring& Name)
{
	VarPtrMap::iterator pfound = pArgMap->find(Name);
	if(pfound != pArgMap->end())
		return pfound->second;
	return NULL;
}

wstring CTuringEngine::GetScriptLine(int iLine)
{
	return int2wstring(iLine);
}

int CTuringEngine::PrepareDebug(const wstring& FxName, SExecutionState* pExecutionState, VarPtrMap* pArgMap, VarMap* pGVarMap, CContext* pContext, SDebugState* &pDebugState, SDebugState::EDebugOp DebugOp, SOp* pOp)
{
	ASSERT(pExecutionState);
	SDebugState* pState = new SDebugState(DebugOp);
	pState->pSupState = pDebugState;
	pDebugState = pState;

	pDebugState->Scope.pContext = pContext;
	pDebugState->Scope.pGVarMap = pGVarMap;
	pDebugState->Scope.pExecutionState = pExecutionState;
	pDebugState->Scope.pArgMap = pArgMap;
	pDebugState->pCallerState = NULL;
	while(pDebugState->Scope.pExecutionState->SubState)
	{
		pDebugState->Scope.pArgMap = &pDebugState->Scope.pExecutionState->CallState->Arguments;
		pDebugState->pCallerState = pDebugState->Scope.pExecutionState;
		pDebugState->Scope.pExecutionState = pDebugState->Scope.pExecutionState->SubState;
	}

	if(DebugOp == SDebugState::eInterleaveOp)
	{
		ASSERT(pOp->eOp != SOp::EOp_Invalid);
		pDebugState->pOp = pOp;
	}
	return FX_OK;
}

int CTuringEngine::ExecuteDebug(SDebugState* &pDebugState)
{
	if(!IsValidFunction(pDebugState->Scope.pExecutionState->Fx) && !IsValidPointer(pDebugState->Scope.pExecutionState->Fx,pDebugState->Scope.pExecutionState->Pointer))
	{
		Log(_T("Debug operation failed! Script state is out of the debug scope or invalid."), pDebugState->Scope.pContext);
		return FX_ERR_INTERRUPT;
	}

	int Ret = FX_OK;
	bool bStepOut = false;
	pDebugState->Scope.pExecutionState->TimeLimit += GetCurTick();
	switch(pDebugState->DebugOp)
	{
		case SDebugState::eInterleaveOp:
			Ret = ExecuteOp(pDebugState->pOp, pDebugState->Scope);
			break;
		case SDebugState::eSingleStep:
			Ret = ExecuteOp(*pDebugState->Scope.pExecutionState->Pointer, pDebugState->Scope);
			if(Ret != FX_OK)
				break;
		case SDebugState::eSkipStep:
			pDebugState->Scope.pExecutionState->Pointer++;
			if(pDebugState->Scope.pExecutionState->Pointer == pDebugState->Scope.pExecutionState->Fx->end())
				bStepOut = true;
			break;
		
		case SDebugState::eStepIn:
		{
			if((*pDebugState->Scope.pExecutionState->Pointer)->eOp == SOp::EOp_Function)
			{
				Ret = PrepareCall(*pDebugState->Scope.pExecutionState->Pointer, pDebugState->Scope);
				if(Ret == FX_OK)
				{
					FxMap::const_iterator I = m_FxMap.find(pDebugState->Scope.pExecutionState->CallState->Function);
					if(I != m_FxMap.end())
						pDebugState->Scope.pExecutionState->SubState = new SExecutionState(I->second, I->second->begin(), pDebugState->Scope.pExecutionState->NextPause != 0, pDebugState->Scope.pExecutionState->DepthLimit,pDebugState->Scope.pExecutionState->TimeLimit);
					else
					{
						FinishCall(pDebugState->Scope);
						Log(_T("Function: ") + (*pDebugState->Scope.pExecutionState->Pointer)->uOp.sFunction->Function + _T(" Not Found in the debug scope"), pDebugState->Scope.pContext);
						Ret = FX_ERR_NOTFOUND;
					}
				}
			}
			break;
		}
		case SDebugState::eStepOut:
		{
			for(; pDebugState->Scope.pExecutionState->Pointer != pDebugState->Scope.pExecutionState->Fx->end(); pDebugState->Scope.pExecutionState->Pointer++)
			{
				if(pDebugState->Scope.pExecutionState->TimeLimit < GetCurTick())
				{
					Log(_T("Function: \"") + GetFxName(pDebugState->Scope.pExecutionState->Fx) + _T("\" hast to be terminated; time limit reached."), pDebugState->Scope.pContext);
					Ret = FX_ERR_TIME;
				}
				else
					Ret = ExecuteOp(*pDebugState->Scope.pExecutionState->Pointer, pDebugState->Scope);
				if(Ret != FX_OK)
					break;
			}

			if(Ret <= FX_OK) // function hasnt been interrupted
				bStepOut = true;
			break;
		}
	}
	pDebugState->Scope.pExecutionState->TimeLimit -= GetCurTick();

	if(bStepOut && pDebugState->pCallerState)
	{
		pDebugState->Scope.pExecutionState = pDebugState->pCallerState; // go up

		delete pDebugState->Scope.pExecutionState->SubState;
		pDebugState->Scope.pExecutionState->SubState = NULL;
		FinishCall(pDebugState->Scope);

		pDebugState->Scope.pExecutionState->Pointer++;
	}

	if(Ret <= FX_OK)
	{
		SDebugState* pState = pDebugState;
		pDebugState = pState->pSupState;
		pState->pSupState = NULL;
		delete pState;
	}

	return Ret;
}

void CTuringEngine::ClearBreak(SOp* Op)
{
	BreakMap::iterator I = m_BreakPoints.find(Op);
	if(I != m_BreakPoints.end())
	{
		delete I->second;
		m_BreakPoints.erase(I);
	}
}

void CTuringEngine::SetBreak(SOp* Op, int Mode, CExpressions* Conditions)
{
	SBreak* &pBreak = m_BreakPoints[Op];
	if(pBreak)
		delete pBreak;
	pBreak = new SBreak(Mode);
	pBreak->Conditions = Conditions;
}

SBreak* CTuringEngine::GetBreak(SOp* Op)
{
	BreakMap::iterator I = m_BreakPoints.find(Op);
	if(I == m_BreakPoints.end())
		return NULL;
	return I->second;
}

bool CTuringEngine::BreakOp(SBreak* pBreak, SOp* Op, SScope& Scope)
{
	if(pBreak->Conditions)
	{
		wstring Result;
		if(int Ret = DoEquation(&Result, pBreak->Conditions, Scope))
		{
			Log(_T("Function: ") + GetFxName(Scope.pExecutionState->Fx) + _T(" encountered an error, Line: ") + GetScriptLine(Op->iLine) + 
				_T("; breakpoint condition has ") + ((Ret == FX_ERR_ARGUMENT) ? _T("Invalid Arguments") : _T("Invalid Syntax")), Scope.pContext);
			return false;
		}
		return !IsFalse(Result);
	}

	if(pBreak->iMode == 0)
	{
		Log(_T("reached breakpoint at: \"") + CTuringParser::PrintLine(Op) + _T("\"; Line:") + GetScriptLine(Op->iLine), Scope.pContext);
		Log(CTuringParser::PrintState(Scope), Scope.pContext);
		Log(_T("+++"), Scope.pContext);
		return false;
	}
	return true;
}

wstring* GetArgument(VarPtrMap* pArgMap, const wstring& Name)
{
	return CTuringEngine::GetArgument(pArgMap, Name);
}

////////////////////////////////////////////////////////////////////////////
// build in Functions

/** Data retrive data from a data segment
* @param: Name:		segment name
* @param: Data:		returned data
* @param: Line:		line to return (in not specifyed all lines are returned at once), incremented by 1, if index out of boud set to -1
* @param: NL:		optional line separator string ("\r\n" is default)
*/

int CTuringEngine::FxData(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wName	= GetArgument(pArgMap, L"Name");
	wstring* wData	= GetArgument(pArgMap, L"Data");
	wstring* wLine	= GetArgument(pArgMap, L"Line");
	wstring* wNL	= GetArgument(pArgMap, L"NL");
	if(wName == NULL || wData == NULL)
		return FX_ERR_ARGUMENT;

	ResMap::const_iterator I = pContext->pEngine->m_ResMap.find(*wName);
	if(I == pContext->pEngine->m_ResMap.end())
		return FX_ERR_ARGUMENT;

	wData->erase();
	if(wLine)
	{
		size_t Line = wLine->empty() ? 0 : wstring2int(wLine,-1);
		if(Line != -1 && Line < I->second->size())
		{
			*wData = *(I->second->begin() + Line);
			wData->erase(0,wData->find_first_not_of(L" \t\r\n"));
			wData->erase(wData->find_last_not_of(L" \t\r\n")+1);
			Line++;
			*wLine = int2wstring(Line);
		}
		else
			*wLine = L"";
	}
	else
	{
		for (UINT i = 0; i < I->second->size(); i++)
		{
			wstring Line = I->second->at(i);
			Line.erase(Line.find_last_not_of(L"\r\n")+1); // remove old new line if present
			if(i+1 < I->second->size()) // dont append new line to the last line
				Line.append(wNL ? *wNL : L"\r\n"); // add new/custom new line
			wData->append(Line);
		}
	}

	return FX_OK;
}

/** Quit end script execution
* @param: Error:		Optional error reason
*/
int CTuringEngine::FxQuit(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wError = GetArgument(pArgMap, L"Error");
	if(wError != NULL)
	{
		pContext->pEngine->Log(_T("Script Quited with ERROR: ") + *wError, pContext->pContext);
		return FX_ERR_NATIVE;
	}
	return FX_TERMINATE;
}
