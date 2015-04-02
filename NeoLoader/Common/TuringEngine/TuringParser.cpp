#include "GlobalHeader.h"
#include "TuringParser.h"
#include "TuringOps.h"
#include "../Common/Strings.h"

CTuringParser::CTuringParser(CTuringEngine* pEngine)
{
	m_pEngine = pEngine;
}

bool CTuringParser::ParseScript(const wstring& Script)
{
	m_pEngine->Cleanup();

	bool bRet = false;

	stringlines ScriptLines;
	LineUpScript(Script,ScriptLines);

	m_ScriptLines = ScriptLines; // note: PreprocessScript modifyes the scriptlines

	scriptblocks ScriptBlocks;
	if(PreprocessScript(ScriptLines, ScriptBlocks))
		bRet = CompileScript(ScriptBlocks);

    for (scriptblocks::iterator I = ScriptBlocks.begin(); I != ScriptBlocks.end(); I++)
		delete (*I).Expressions;

	return bRet;
}

void CTuringParser::LineUpScript(const wstring& Script, stringlines &ScriptLines, bool bRectify)
{
	wstring::size_type Prev = 0;
	wstring::size_type Next = 0;
	wstring Temp;
	int Merged=0;
	for(;Next != wstring::npos;)
	{
		Next = Script.find(L'\n',Prev);
		if(Next == wstring::npos)
			Temp.append(Script.substr(Prev));
		else
		{
			Temp.append(Script.substr(Prev,Next-Prev));
			Prev = Next+1;
		}

		wstring::size_type Tmp = Temp.find_last_not_of(L"\r\n");
		if(bRectify && Tmp != wstring::npos && Temp.at(Tmp) == L'\\')
		{
			Temp.erase(Tmp);
			Merged++;
			Tmp = Script.find_first_not_of(L" \t",Prev);
			if(Tmp != wstring::npos)
				Prev = Tmp;
		}
		else
		{
			ScriptLines.push_back(Temp);
			for(;Merged > 0;Merged--)
				ScriptLines.push_back(L"");
			Temp.clear();
		}
	}
}

wstring CTuringParser::GetScriptLine(int iLine)
{
	if(m_ScriptLines.size() >= iLine)
	{
		wstring Line = m_ScriptLines.at(iLine-1);
		if(Line.c_str()[0] == L'?')
			Line.at(0) = L' ';

		return int2wstring(iLine) + L" (\"" + Trimm(Line) + L"\")";
	}
	return L"*unknown*";
}

wstring CTuringParser::GetWord(const wstring &Line, wstring::size_type &Offset)
{
	wstring::size_type Start = Line.find_first_not_of(L" \t\r\n", Offset);
	if(Start == wstring::npos)
		return L"";
	wstring::size_type Stop = Line.find_first_of(L"( \t\r\n",Start);
	if(Stop == wstring::npos)
		Stop = Line.size();
	Offset = Stop;
	return Line.substr(Start,Stop-Start);
}

CExpressions* CTuringParser::GetExpressions(const wstring &Line, wstring* Comment)
{
	CExpressions* Expressions = new CExpressions;
	wstring Expression;

	bool bEsc = false;
	bool bString = false;
	int iBlocks = 0;
	int iBrackets = 0;
	bool bOperator = false;

	for(wstring::size_type i = 0; i < Line.size(); i++)
	{
		wchar_t Char = Line.at(i);

		int iEnd = 0;
		if(bString) // inside a string
		{
			if(bEsc)
			{
				bEsc = false;

				if(iBrackets == 0 && iBlocks == 0) // we interprete the ESC sequence only in the last dissection stage
				{
					switch(Char)
					{
						case L'\\':	Expression += L'\\';	break;
						case L'\'':	Expression += L'\'';	break;
						case L'\"':	Expression += L'\"';	break;
						case L'a':	Expression += L'\a';	break;
						case L'b':	Expression += L'\b';	break;
						case L'f':	Expression += L'\f';	break;
						case L'n':	Expression += L'\n';	break;
						case L'r':	Expression += L'\r';	break;
						case L't':	Expression += L'\t';	break;
						case L'v':	Expression += L'\v';	break;
						default:	Expression += L'?';		break;
					}
				}
				else
				{
					Expression += L'\\' ;
					Expression += Char;
				}
				continue;
			}
			else if(Char == L'\\') // ESC sequence handling
			{
				bEsc = true;
				continue;
			}

			if(Char == L'"') // end of a string is always end of expression
			{
				bString = false;
				if(iBrackets == 0 && iBlocks == 0)
					iEnd = 1;
			}
		}
		else if(Char == L'"') // begin of a string
		{
			bString = true;
			if(iBrackets == 0 && iBlocks == 0)
				iEnd = -1;
		}
		else if(Char == L'\'')
		{
			if(Comment)
				*Comment = Line.substr(i);
			break; // from here a comment starts
		}

		else if(iBlocks || Char == L'[') // a block
		{
			if(Char == L'[')
				iBlocks++;
			else if(Char == L']')
				iBlocks--;
		}

		else if(Char == L'(')
		{
			bOperator = false;
			if(iBrackets == 0)
				iEnd = 2; //-1;
			iBrackets++;
		}
		else if(Char == L')')
		{
			iBrackets--;
			if(iBrackets == 0)
				iEnd = -2; //1;
		}

		else if (iBrackets == 0)
		{
			if(Char == L' ' || Char == L'\t' || Char == L'\r' || Char == L'\n')
				iEnd = 2;
			else if(CTuringOps::IsOperator(Char) != bOperator)
			//else if(bOperator 
			// ? (!IsOperator(Char) && Expression[0] != L'%') // Note: the % can be extended with a text, it must be terminated with a white space
			// : (IsOperator(Char)))
			{
				bOperator = !bOperator;
				iEnd = -1;
			}
		}

		if(iEnd == 0 || iEnd == 1)
			Expression += Char;
		if(iEnd != 0)
		{
			if(iEnd == -2)
			{
				CExpressions* SubExpressions = GetExpressions(Expression);
				if(SubExpressions)
					Expressions->Add(new SExpression(SubExpressions));
				else
				{
					delete Expressions;
					return NULL;
				}
			}
			else if(!Expression.empty())
				Expressions->Add(new SExpression(Expression));
			Expression.clear();
		}
		if(iEnd == -1)
			Expression += Char;
	}

	if(!Expression.empty())
		Expressions->Add(new SExpression(Expression));

	if(bString || iBrackets)
	{
		delete Expressions;
		return NULL;
	}
	return Expressions;
}

wstring EscapeString(wstring str)
{
	for(string::size_type i=0; i<str.size(); i++)
	{
		switch(str[i])
		{
			case L'\\': str[i] = L'\\';	break;
			case L'\'': str[i] = L'\'';	break;
			case L'\"': str[i] = L'\"';	break;
			case L'\a': str[i] = L'a';	break;
			case L'\b': str[i] = L'b';	break;
			case L'\f': str[i] = L'f';	break;
			case L'\n': str[i] = L'n';	break;
			case L'\r': str[i] = L'r';	break;
			case L'\t': str[i] = L't';	break;
			case L'\v': str[i] = L'v';	break;
			default:
				continue;
		}
		str.insert(i,L"\\"); 
		i++;
	}
	return str;
}

wstring CTuringParser::PrintExpressions(CExpressions* Expressions)
{
	wstring Printer;
	for(UINT i=0; i < Expressions->Count(); i++)
	{
		SExpression* Expression = *Expressions->GetExp(i);
		if(!Printer.empty() && Printer.at(Printer.size()-1) != L' ')
			Printer.append(L" ");

		if(Expression->IsMulti())
		{
			Printer.append(L"(");
			if(Expression->Exp.Multi) // in some cases the item might have been detached
				Printer.append(PrintExpressions(Expression->Exp.Multi));
			Printer.append(L")");
		}
		else if(Expression->IsString())
			Printer.append(L"\"" + EscapeString(Expression->Exp.Word->substr(1, Expression->Exp.Word->size()-2)) + L"\"");
		else
			Printer.append(*Expression->Exp.Word);
	}
	return Printer;
}

#ifdef USE_JSON
bool CTuringParser::SplitBlocks(const wstring& String, vector<wstring>& StrList, bool bDot)
{
	wstring::size_type End = 0;
	wstring::size_type Begin = 0;
	int Count = 0;
	for(wstring::size_type Pos = 0;Pos < String.size();)
	{
		wstring::size_type Tmp = String.find_first_of(bDot ? L".[]" : L"[]", Pos);
		if(Tmp == wstring::npos)
			End = Tmp = String.size();
		else if(String.at(Tmp) == L'[')
		{
			if(!Count++ && !bDot)
				End = Tmp;
		}
		else if(String.at(Tmp) == L']')
		{
			if(!--Count)
				End = Tmp+1;
		}
		else if(Count == 0)
		{
			ASSERT(bDot);
			End = Tmp;
		}
		
		if(End > Begin)
		{
			StrList.push_back(String.substr(Begin,End-Begin));
			Begin = End;
		}
		Pos = Tmp+1;
	}
	return Count == 0 && !StrList.empty();
}

int CTuringParser::ExtractBlock(CExpressions* Expressions, map<wstring,wstring> &Blocks, int &Counter)
{
	int Changes = 0;
	for(UINT i=0; i < Expressions->Count(); i++)
	{
		SExpression* Expression = *Expressions->GetExp(i);
		if(Expression->IsMulti())
		{
			int SubChanges = ExtractBlock(Expression->Exp.Multi, Blocks, Counter);
			if(SubChanges < 0)
				return -1;
			Changes += SubChanges;
		}
		else if(Expression->IsWord())
		{
			vector<wstring> StrList;
			if(!SplitBlocks(*Expression->Exp.Word, StrList))
				return -1;
			if(StrList.size() == 1 && StrList.at(0).at(0) != L'[')
				continue;

			Expression->Exp.Word->clear();
			for(vector<wstring>::iterator J = StrList.begin(); J != StrList.end(); J++)
			{
				ASSERT(!J->empty());
				if(J->at(0) == L'[')
				{
					if(CTuringOps::HasOperator(*J))
					{
						wstring Temp = L"tmp_var_" + int2wstring(Counter++);
						Blocks[Temp] = J->substr(1,J->size()-2);
						*Expression->Exp.Word += L"[" + Temp + L"]";
					}
					else if(iswdigit(J->at(1)))
						*Expression->Exp.Word += L"." + J->substr(1,J->size()-2);
					else if (J->at(1) == L'\"')
						*Expression->Exp.Word += L"." + J->substr(2,J->size()-4);
					else if(!Expression->Exp.Word->empty() && (*Expression->Exp.Word->rbegin()) != L'.')
						*Expression->Exp.Word += L"." + *J;
					else
					{
						*Expression->Exp.Word += *J;
						continue;
					}
					Changes++;
				}
				else
					*Expression->Exp.Word += *J;
			}
		}
	}
	return Changes;
}
#endif

bool CTuringParser::PreprocessScript(stringlines &ScriptLines, scriptblocks &ScriptBlocks)
{
	int Counter = 1; // unique names
	CExpressions* Expressions = NULL;
	wstringstream Writer;
	STuringBlock* pCurBlock = new STuringBlock;
	stringlines* Res = NULL;
	int iLine = 0;
	int Added = 0;
	int ThenMod = 0;

    for (UINT i = 0; i < ScriptLines.size(); i++)
    {
		if(Added > 0)
			Added--;
		else
			iLine++;
		if(ThenMod)
		{
			if(ThenMod == 4)
				Added++;
			if(ThenMod == 2)
				iLine -= 1;
			if(ThenMod == 1)
				iLine += 1;
			ThenMod--;
		}

		ASSERT(pCurBlock); // we can never get down to 0;

		wstring Line = ScriptLines.at(i);
		wstring::size_type Offset = 0;
		wstring word = GetWord(Line,Offset);
		if(word.empty() || word.at(0) == L'\'')
			continue;

		switch(pCurBlock->eType)
		{
			case STuringBlock::eNone:
			{
				pCurBlock->Name = GetWord(Line,Offset);
				if(pCurBlock->Name.empty())
				{
					m_pEngine->Log(_T("Failed to parse line: ") + GetScriptLine(iLine) + _T("; ") + word + _T(" begin does not have a valid name"));
					delete pCurBlock;
					return false;
				}
				if(GetWord(Line,Offset) != L"Begin")
				{
					m_pEngine->Log(_T("Failed to parse line: ") + GetScriptLine(iLine) + _T("; is not a valid ") + word + _T(" begin"));
					delete pCurBlock;
					return false;
				}

				if(word == L"Function")
					pCurBlock->eType = STuringBlock::eFunction;
				else if(word == L"Data")
				{
					ResMap::const_iterator I = m_pEngine->m_ResMap.find(pCurBlock->Name);
					if(I != m_pEngine->m_ResMap.end())
					{
						m_pEngine->Log(_T("Data: ") + pCurBlock->Name + _T(" already exists"));
						delete pCurBlock;
						return false;
						//delete I->second;
						//m_pEngine->m_ResMap.erase(I);
					}

					Res = new stringlines;
					m_pEngine->m_ResMap.insert(ResMap::value_type(pCurBlock->Name, Res));

					pCurBlock->eType = STuringBlock::eData;
					ScriptLines.erase(ScriptLines.begin() + i--);
					continue;
				}
				break;
			}
			case STuringBlock::eFunction:
			{
				if(word == L"Function" && GetWord(Line,Offset) == L"End")
				{
					ScriptLines.insert(ScriptLines.begin() + i++, L"eof:");
					ScriptBlocks.push_back(SBlock(GetExpressions(L"eof:"), iLine));
					pCurBlock->eType = STuringBlock::eNone;
				}
				break;
			}
			case STuringBlock::eData:
			{
				if(word == L"Data" && GetWord(Line,Offset) == L"End")
				{
					Res = NULL;
					pCurBlock->eType = STuringBlock::eNone;
				}
				else if(Res)
					Res->push_back(Line);
				ScriptLines.erase(ScriptLines.begin() + i--);
				continue;
			}
		}

		// Note: if we are here we are inside a function

		bool bBreak = false;
		if(word == L"?")
		{
			bBreak = true;
			Line.erase(0,1);
		}

		wstring Tab = Line.substr(0, Line.find_first_not_of(L" \t\r\n"));

		wstring Comment;
		Expressions = GetExpressions(Line, &Comment);
		if(Expressions == NULL)
		{
			m_pEngine->Log(_T("Failed to parse line: ") + GetScriptLine(iLine));
			delete pCurBlock;
			return false;
		}
		word = Expressions->GetExpStr(0);

#ifdef USE_JSON
		if(word.find(L'.[') == wstring::npos || word.at(0) == L'[' || !Expressions->IsExpMulti(1)) // is not a object function
#endif
		if(word != L"end" && word != L"if" && word != L"else" && word != L"loop" && word != L"break" && word != L"continue" && word != L"exit") // not a relevant word
		{
#ifdef USE_JSON
			map<wstring,wstring> Blocks;
			int Changes = ExtractBlock(Expressions, Blocks, Counter);
			if(Changes < 0)
			{
				m_pEngine->Log(_T("Failed to pre process line: ") + GetScriptLine(iLine) + _T("; invalid blocks"));
				break;
			}
			else if(Changes > 0) // extract blocks and 
			{
				ScriptLines.erase(ScriptLines.begin() + i--);
			
				Writer.str(L""); if(bBreak) Writer << L"? "; Writer << Tab;
				Writer << PrintExpressions(Expressions);
				if(!Comment.empty()) Writer << " " << Comment;
				ScriptLines.insert(ScriptLines.begin() + i+1, Writer.str());
				Added++;

				int j=1;
				for (map<wstring,wstring>::iterator J = Blocks.begin(); J != Blocks.end(); J++)
				{
					Writer.str(L""); Writer << Tab;
					Writer << J->first + L" = " + J->second;
					ScriptLines.insert(ScriptLines.begin() + i+j++, Writer.str());
					Added++;
				}
				delete Expressions;
				Expressions = NULL;
			}
			else
#endif
				ScriptBlocks.push_back(SBlock(Expressions, iLine, bBreak));
			Expressions = NULL;
			continue;
		}
		
		if(Added && (word == L"if" || word == L"else" || word == L"loop" || word == L"end"))
		{
			m_pEngine->Log(_T("Failed to pre process line: ") + GetScriptLine(iLine) + _T("; \"if (...) then\" can only be followed by a simple statement"));
			break;
		}

		ScriptLines.erase(ScriptLines.begin() + i--);
		int j=1;
		if(word == L"end")
		{
			if(pCurBlock->eType == STuringBlock::eFunction)
			{
				m_pEngine->Log(_T("Failed to pre process line: ") + GetScriptLine(iLine) + _T("; out of place \"end\" statement"));
				break;
			}

			while (!pCurBlock->Ends.empty())
			{
				wstring End = pCurBlock->Ends.back();
				if(!Comment.empty()) End += L" " + Comment;
				ScriptLines.insert(ScriptLines.begin() + i+j++, End);
				Added++;
				pCurBlock->Ends.pop_back();
			}
			STuringBlock::Pop(pCurBlock);
		}
		else if(word == L"if")
		{
			STuringBlock::Push(STuringBlock::eIf, pCurBlock);
			pCurBlock->Name = int2wstring(Counter++);

			SExpression** Conditions = Expressions->GetExp(1);
			if(!Conditions || !(*Conditions)->IsMulti())
			{
				m_pEngine->Log(_T("Failed to pre process line: ") + GetScriptLine(iLine) + _T("; invalid \"if\" statement"));
				break;
			}
			Writer.str(L""); if(bBreak) Writer << L"? "; Writer << Tab;
			Writer << L"goto !(" << PrintExpressions((*Conditions)->Exp.Multi).c_str() << L") end_if_" << pCurBlock->Name;
			if(!Comment.empty()) Writer << " " << Comment;
			ScriptLines.insert(ScriptLines.begin() + i+j++, Writer.str());
			Added++;

			Writer.str(L""); Writer << Tab;
			Writer << L"end_if_" << pCurBlock->Name << L":";
			pCurBlock->Ends.push_back(Writer.str());

			if(Expressions->GetExpStr(2) == L"then") // single operation if statement
			{
				j++;
				ScriptLines.insert(ScriptLines.begin() + i+j++, L"end");
				ThenMod = 5;
			}
		}
		else if(word == L"else")
		{
			if(pCurBlock->eType != STuringBlock::eIf)
			{
				m_pEngine->Log(_T("Failed to pre process line: ") + GetScriptLine(iLine) + _T("; \"else\", outside \"if\" block"));
				break;
			}

			Writer.str(L""); Writer << Tab;
			Writer << L"goto end_else_" << pCurBlock->Name;
			ScriptLines.insert(ScriptLines.begin() + i+j++, Writer.str());
			Added++;

			wstring End = pCurBlock->Ends.back();
			if(!Comment.empty()) End += L" " + Comment;
			ScriptLines.insert(ScriptLines.begin() + i+j++, End);
			Added++;
			pCurBlock->Ends.pop_back();

			Writer.str(L""); Writer << Tab;
			Writer << L"end_else_" << pCurBlock->Name << L":";
			pCurBlock->Ends.push_back(Writer.str());
			
			if(Expressions->GetExpStr(1) == L"if")
			{
				pCurBlock->eType = STuringBlock::eIf;
				pCurBlock->Name = int2wstring(Counter++);

				SExpression** Conditions = Expressions->GetExp(2);
				if(!Conditions || !(*Conditions)->IsMulti())
				{
					m_pEngine->Log(_T("Failed to pre process line: ") + GetScriptLine(iLine) + _T("; invalid \"else if\" statement"));
					break;
				}
				Writer.str(L""); if(bBreak) Writer << L"? "; Writer << Tab;
				Writer << L"goto !(" << PrintExpressions((*Conditions)->Exp.Multi).c_str() << L") end_if_" << pCurBlock->Name;
				ScriptLines.insert(ScriptLines.begin() + i+j++, Writer.str());
				Added++;

				Writer.str(L""); Writer << Tab;
				Writer << L"end_if_" << pCurBlock->Name << L":";
				pCurBlock->Ends.push_back(Writer.str());
			}
			else
				pCurBlock->eType = STuringBlock::eElse;
		}
		else if(word == L"exit")
		{
			Writer.str(L""); if(bBreak) Writer << L"? "; Writer << Tab;
			Writer << L"goto eof";
			ScriptLines.insert(ScriptLines.begin() + i+j++, Writer.str());
			Added++;
		}
		else if(word == L"loop")
		{
			STuringBlock::Push(STuringBlock::eLoop, pCurBlock);
			pCurBlock->Name = int2wstring(Counter++);

			Writer.str(L""); Writer << Tab;
			Writer << L"loop_" << pCurBlock->Name << L":";
			if(!Comment.empty()) Writer << " " << Comment;
			ScriptLines.insert(ScriptLines.begin() + i+j++, Writer.str());
			Added++;

			if(SExpression** Conditions = Expressions->GetExp(1)) // loops can be unconditional
			{
				if(!(*Conditions)->IsMulti()) 
				{
					m_pEngine->Log(_T("Failed to pre process line: ") + GetScriptLine(iLine) + _T("; invalid \"loop\" statement"));
					break;
				}
				Writer.str(L""); if(bBreak) Writer << L"? "; Writer << Tab;
				Writer << L"goto !(" << PrintExpressions((*Conditions)->Exp.Multi).c_str() << L") end_loop_" << pCurBlock->Name;
				ScriptLines.insert(ScriptLines.begin() + i+j++, Writer.str());
				Added++;
			}

			Writer.str(L""); Writer << Tab;
			Writer << L"end_loop_" << pCurBlock->Name << L":";
			pCurBlock->Ends.push_back(Writer.str());

			Writer.str(L""); Writer << Tab;
			Writer << L"goto loop_" << pCurBlock->Name;
			pCurBlock->Ends.push_back(Writer.str());
		}
		else if(word == L"break" || word == L"continue")
		{
			STuringBlock* pBlock = pCurBlock;
			int LoopCount = Expressions->Count() > 1 ? wstring2int(Expressions->GetExpStr(1)) : 1;
			for(;pBlock;pBlock = pBlock->pSupBlock)
			{
				if(pBlock->eType == STuringBlock::eLoop && --LoopCount == 0)
					break;
			}
			if(pBlock == NULL)
			{
				m_pEngine->Log(_T("Failed to pre process line: ") + GetScriptLine(iLine) + _T("; out of place \"") + word + _T("\" statement"));
				break;
			}

			Writer.str(L""); if(bBreak) Writer << L"? "; Writer << Tab;
			Writer << L"goto ";
			if(word == L"break")
				Writer << L"end_";
			Writer << L"loop_" << pBlock->Name;
			if(!Comment.empty()) Writer << " " << Comment;
			ScriptLines.insert(ScriptLines.begin() + i+j++, Writer.str());
			Added++;
		}
#ifdef USE_JSON
		else
		{
			ASSERT(Expressions->IsExpMulti(1));

			wstring Function = Expressions->GetExpStr(0);
			SExpression** ArgumentList = Expressions->GetExp(1);
			if(Function.empty() || Expressions->Count() != 2)
			{
				m_pEngine->Log(_T("Failed to pre process line: ") + GetScriptLine(iLine) + _T("; not a valid function call"));
				break;
			}

			wstring This;
			if((*ArgumentList)->Exp.Multi->Count() > 0)
				This = L", ";
			This += L"this = ";

			wstring::size_type Pos = Function.find_last_of(L".[");
			ASSERT(Pos != wstring::npos);
			This += Function.substr(0,Pos);

			Writer.str(L""); if(bBreak) Writer << L"? "; Writer << Tab;
			Writer << L"[" << Function << L"]";
			Writer << L" (" << PrintExpressions((*ArgumentList)->Exp.Multi) << This << L")";
			if(!Comment.empty()) Writer << " " << Comment;
			ScriptLines.insert(ScriptLines.begin() + i+1, Writer.str());
			Added++;
		}
#endif
		delete Expressions;
		Expressions = NULL;
	}

	bool bRet = true;
	if(Expressions)
	{
		// An log was already made
		delete Expressions;
		bRet = false;
	}
	else if(pCurBlock->pSupBlock)
	{
		m_pEngine->Log(_T("Failed to pre process script: Block stack is not complete"));
		bRet = false;
	}
	delete pCurBlock;
	return bRet;
}

bool CTuringParser::CompileScript(scriptblocks &ScriptBlocks)
{
	FxVector* Fx = NULL;
	wstring FxName;

    for (scriptblocks::iterator I = ScriptBlocks.begin(); I != ScriptBlocks.end(); I++)
    {
		ASSERT(I->Expressions);

		if(Fx == NULL) // we are outside of a function
		{
			if(I->Expressions->GetExpStr(0) != L"Function")
			{
				m_pEngine->Log(_T("Failed to parse line: ") + GetScriptLine(I->iLine) + _T("; statement is invalid outside a function"));
				return false;
			}

			FxName = I->Expressions->GetExpStr(1);
			if(FxName.empty())
			{
				m_pEngine->Log(_T("Failed to parse line: ") + GetScriptLine(I->iLine) + _T("; function begin does not have a valid name"));
				return false;
			}

			if(I->Expressions->GetExpStr(2) != L"Begin" || I->Expressions->Count() > 3)
			{
				m_pEngine->Log(_T("Failed to parse line: ") + GetScriptLine(I->iLine) + _T("; is not a valid function begin"));
				return false;
			}

			FxMap::const_iterator I = m_pEngine->m_FxMap.find(FxName);
			if(I != m_pEngine->m_FxMap.end())
			{
				m_pEngine->Log(_T("Function: ") + FxName + _T("already exists"));
				return false;
				//for(FxVector::iterator J = I->second->begin(); J != I->second->end(); J++)
				//	delete (*J);
				//delete I->second;
				//m_pEngine->m_FxMap.erase(I);
			}

			Fx = new FxVector;
			m_pEngine->m_FxMap.insert(FxMap::value_type(FxName, Fx));
		}
		else if(I->Expressions->GetExpStr(0) == L"Function") //we are inside a function and found a function statement
		{
			if(I->Expressions->GetExpStr(1) != L"End" || I->Expressions->Count() > 2)
			{
				m_pEngine->Log(_T("Failed to parse line: ") + GetScriptLine(I->iLine) + _T("; is not a valid function end"));
				return false;
			}

			Fx = NULL;
		}
		else // from here on we process a function intern line
		{
			wstring Error;
			if(SOp* Op = CompileOp(I->Expressions, Error))
			{
				Op->iLine = I->iLine;
				Fx->push_back(Op);
				if(I->bBreak && m_pEngine->BreaksEnabled())
					m_pEngine->m_BreakPoints.insert(BreakMap::value_type(Op, new SBreak()));
			}
			else
			{
				m_pEngine->Log(_T("Failed to compile line: ") + GetScriptLine(I->iLine) + _T("; ") + Error);
				return false;
			}
		}
    }

	if(Fx != NULL)
	{
		m_pEngine->Log(_T("Failed to parse script, last Function: ") + FxName + _T("is not complete"));
		return false;
	}
	return true;
}

SOp* CTuringParser::CompileOp(CExpressions* &Expressions, wstring& Error)
{
	SOp* Op = new SOp;

	// label
	if(Expressions->GetExpStr(1) == L":") 
	{
		SExpression** Label = Expressions->GetExp(0);
		if((*Label)->IsMulti() || Expressions->Count() > 2)
		{
			Error = _T("invalid label");
			delete Op;
			return NULL;
		}

		Op->eOp = SOp::EOp_Label;
		Op->uOp.sLabel = new SOp::UOp::SLabel;
		Op->uOp.sLabel->Label = *(*Label)->Exp.Word;
	}
	// goto
	else if(Expressions->GetExpStr(0) == L"goto") 
	{
		bool Not = false;
		SExpression** Conditions = NULL;
		SExpression** Label = NULL;
		if(Expressions->Count() == 2)
			Label = Expressions->GetExp(1);
		else if(Expressions->Count() == 3)
		{
			Conditions = Expressions->GetExp(1);
			Label = Expressions->GetExp(2);
		}
		else if(Expressions->Count() == 4 && Expressions->GetExpStr(1) == L"!")
		{
			Not = true;
			Conditions = Expressions->GetExp(2);
			Label = Expressions->GetExp(3);
		}

		if(!Label || (*Label)->IsMulti() || (Conditions && !(*Conditions)->IsMulti()))
		{
			Error = _T("invalid goto");
			delete Op;
			return NULL;
		}

		Op->eOp = SOp::EOp_Goto;
		Op->uOp.sGoto = new SOp::UOp::SGoto;
		Op->uOp.sGoto->Label = *(*Label)->Exp.Word;
		Op->uOp.sGoto->Not = Not;
		if(Conditions)
		{
			if(!OrderEquation((*Conditions)->Exp.Multi,true))
			{
				Error = _T("invalid equasion");
				delete Op;
				return NULL;
			}

			Op->uOp.sGoto->Conditions = (*Conditions)->Exp.Multi;
			(*Conditions)->Exp.Multi = NULL; // or else it will be deleted !
		}
		else
			Op->uOp.sGoto->Conditions = NULL;
	}
	// equasion
	else if(Expressions->IsExpOp(1)) 
	{
		if(!OrderEquation(Expressions,false))
		{
			Error = _T("invalid equasion");
			delete Op;
			return NULL;
		}

		Op->eOp = SOp::EOp_Equation;
		Op->uOp.sEquation = new SOp::UOp::SEquation;
		Op->uOp.sEquation->Equation = Expressions;
		Expressions = NULL; // or else it will be deleted !
	}
	// function
	else if (Expressions->IsExpMulti(1))
	{
		wstring Function = Expressions->GetExpStr(0);
		SExpression** ArgumentList = Expressions->GetExp(1);
		if(Function.empty() || Expressions->Count() != 2)
		{
			Error = _T("invalid function call");
			delete Op;
			return NULL;
		}

		Op->eOp = SOp::EOp_Function;
		Op->uOp.sFunction = new SOp::UOp::SFunction;
		Op->uOp.sFunction->Function = Function;
		
		for(UINT i = 0; i<(*ArgumentList)->Exp.Multi->Count();)
		{
			wstring Arg = (*ArgumentList)->Exp.Multi->GetExpStr(i++);
			if(!Arg.empty())
			{
				wstring Assign = (*ArgumentList)->Exp.Multi->GetExpStr(i++);
				if(Assign == L"=" || Assign == L":=")
				{

					int ToGo = 0;
					while(((*ArgumentList)->Exp.Multi->GetExpStr(i + ++ToGo) != L",") && (i + ToGo < (*ArgumentList)->Exp.Multi->Count()));

					if(ToGo > 1)
						(*ArgumentList)->Exp.Multi->SubordinateExp(i,ToGo);

					if(SExpression** Val = (*ArgumentList)->Exp.Multi->GetExp(i++))
					{
						if((*Val)->IsMulti())
						{
							if(!OrderEquation((*Val)->Exp.Multi,false))
							{
								Error = _T("invalid equasion");
								ArgumentList = NULL;
								break;
							}

							if(Assign != L"=")
							{
								Error = _T("Function Argument Equations must use direct assignment \"=\"");
								ArgumentList = NULL;
								break;
							}
						}
						if(Arg.at(0) == L'\"') // just in case, this way we can pas arbitrary variable names to native functions
							Arg = Arg.substr(1,Arg.size()-2);
						Op->uOp.sFunction->Arguments.push_back(new SOp::UOp::SFunction::SArg(Arg,Assign,*Val));
						*Val = NULL; // or else it will be deleted !
						i++; // skip the coma
						continue;
					}
				}
			}

			ArgumentList = NULL;
			break;
		}

		if(!ArgumentList)
		{
			Error = _T("invalid argument list");
			delete Op;
			return NULL;
		}
	}
	// unknown
	else
	{
		Error = _T("unknown expression");
		delete Op;
		return NULL;
	}

	return Op;
}

bool CTuringParser::OrderEquation(CExpressions* Expressions, bool bSub)
{
	// Make sure normal equations a = b & a & c work proeprly and dont overwrite inproperly
	if(!bSub && Expressions->Count() > 3 && Expressions->GetExpStr(1) == L"=")
		Expressions->SubordinateExp(2, Expressions->Count()-2);

	int Prev = bSub ? -1 : 1;
	vector<int> Order;
	Order.push_back(0);
	vector<int> Start;
	Start.resize(OP_LEVELS,0);
	for (UINT i = 0; i < Expressions->Count() + 1; i++)
    {
		wstring ExpStr;
		if(i < Expressions->Count())
		{
			SExpression* Expression = *Expressions->GetExp(i);

			if(Expression->IsMulti())
			{
				if(i > 0 && !Expressions->IsExpOp(i-1))
				{
					for(int j=0; j < Expression->Exp.Multi->Count(); j+=2)
					{
						int ToGo = 0;
						while((Expression->Exp.Multi->GetExpStr(j + ++ToGo) != L",") && (j + ToGo < Expression->Exp.Multi->Count()));
						if(!Expression->Exp.Multi->SubordinateExp(j, ToGo))
							return false;
						if(!OrderEquation((*Expression->Exp.Multi->GetExp(j))->Exp.Multi, true))
							return false;
						Expression->Exp.Multi->Del(j-- + 1);
					}
					// put a () around this in order not to cofuse the later sorting process
					if(!Expressions->SubordinateExp(--i, 2))
						return false;
					continue;
				}

				if(!OrderEquation(Expression->Exp.Multi, true))
					return false;
				continue;
			}

			if(!Expression->IsOperator())
				continue;
			ExpStr = *Expression->Exp.Word;
		}

		if(i < (bSub ? 0 : 2))
			continue;

		if(i == Prev+1) // two operators in a row
		{
			int ToGo = 1;
			int Index = i;
			while(Expressions->IsExpOp(i+ToGo++));
			if(ToGo == Expressions->Count())
			{
				ToGo--;
				Index++;
			}
			if(ToGo > 1)
			{
				i--;
				if(!Expressions->SubordinateExp(Index,ToGo))
					return false;
				continue;
			}
		}
	
		Prev = i;
		if(i == 0)
			continue;

		int Level = CTuringOps::GetOperatorLevel(ExpStr);
		if(Level < 0)
			return false;
		ASSERT(Level < OP_LEVELS);
		if(Level != Order.back())
		{
			if(Level < Order.back()) // step down, sobordinate expression
			{
				int Index = Start[Order.back()] - 1;
				Order.pop_back();
				int ToGo = i - Index;
				if(ToGo < Expressions->Count() - (bSub ? 0 : 2))
				{
					i = Index - 1;
					if(!Expressions->SubordinateExp(Index,ToGo))
						return false;
					continue;
				}
			}
			
			if(Level > Order.back()) // step up
			{
				Order.push_back(Level);
				Start[Level] = i;
			}
		}	
	}

	// make sure stuff like a = b = c + d results in -> a = (b = c + d), in order to set a and b to the value c + d
    for (UINT i = bSub ? 0 : 2; i < Expressions->Count(); i++)
    {
		SExpression* Expression = *Expressions->GetExp(i);
		if(Expression->IsMulti())
			continue;

		if(i > 0 && *Expression->Exp.Word == L"=")
			Expressions->SubordinateExp(i-1, Expressions->Count()-(i-1));
	}

	return true;
}

/////////////////////////////////////////////////////////////
//

wstring CTuringParser::PrintFunction(FxVector* Fx)
{
	wstring Printer;

	for(FxVector::iterator J = Fx->begin(); J != Fx->end(); J++)	
	{
		Printer.append(PrintLine(*J));
		Printer.append(L"\n");
	}

	return Printer;
}

wstring CTuringParser::PrintLine(SOp* Op)
{
	wstring Printer;

	switch(Op->eOp)
	{
		case SOp::EOp_Label: 
		{
			Printer.append(Op->uOp.sLabel->Label);
			Printer.append(L":");
			break;
		}
		case SOp::EOp_Goto: 
		{
			Printer.append(L"goto ");
			if(Op->uOp.sGoto->Conditions)
			{
				if(Op->uOp.sGoto->Not)
					Printer.append(L"!");
				Printer.append(L"(");
				Printer.append(PrintExpressions(Op->uOp.sGoto->Conditions));
				Printer.append(L") ");
			}
			Printer.append(Op->uOp.sGoto->Label);
			break;
		}

		case SOp::EOp_Equation:
		{
			Printer.append(PrintExpressions(Op->uOp.sEquation->Equation));
			break;
		}

		case SOp::EOp_Function: 
		{
			Printer.append(Op->uOp.sFunction->Function);

			Printer.append(L" ( ");
			for(vector<SOp::UOp::SFunction::SArg*>::iterator I = Op->uOp.sFunction->Arguments.begin(); I != Op->uOp.sFunction->Arguments.end(); I++)
			{
				if(I != Op->uOp.sFunction->Arguments.begin())
					Printer.append(L", ");

				SExpression* Val = (*I)->Value;
				Printer.append((*I)->Argument);
				Printer.append(L" ");
				Printer.append((*I)->Assignment);
				Printer.append(L" ");
				if(Val->IsMulti())
				{
					Printer.append(L"(");
					Printer.append(PrintExpressions(Val->Exp.Multi));
					Printer.append(L")");
				}
				else if(Val->IsString())
					Printer.append(L"\"" + EscapeString(Val->Exp.Word->substr(1, Val->Exp.Word->size()-2)) + L"\"");
				else
					Printer.append(*Val->Exp.Word);
			}
			Printer.append(L" )");

			break;
		}
	}

	return Printer;
}

wstring LimitedPrint(wstring& String)
{
	if(String.size() > 1024)
	{
		wstringstream wVal;
		wVal << String.substr(0,1024).c_str();
		wVal << L" ... \n(" << String.size() << L")";
		return wVal.str();
	}
	else
		return String;
}

wstring CTuringParser::PrintState(SScope& Scope)
{
	wstring Printer;

	Printer.append(L"\nArguments:");
	for(VarPtrMap::iterator I = Scope.pArgMap->begin(); I != Scope.pArgMap->end(); I++)
	{
		Printer.append(L"\n");
		Printer.append(I->first.c_str());
		Printer.append(L" = ");
		Printer.append(LimitedPrint(*I->second));
	}

	Printer.append(L"\nVariables:");
	for(VarMap::iterator I = Scope.pExecutionState->Variables.begin(); I != Scope.pExecutionState->Variables.end(); I++)
	{
		Printer.append(L"\n");
		Printer.append(I->first.c_str());
		Printer.append(L" = ");
		Printer.append(LimitedPrint(I->second));
	}

	if(Scope.pGVarMap == &Scope.pExecutionState->Variables)
		return Printer;

	for(VarMap::iterator I = Scope.pGVarMap->begin(); I != Scope.pGVarMap->end(); I++)
	{
		ASSERT(!I->first.empty());
		if(I->first.at(0) == L'.')
		{
			Printer.append(L"\n");
			Printer.append(I->first.c_str());
			Printer.append(L" = ");
			Printer.append(LimitedPrint(I->second));
		}
	}

	return Printer;
}