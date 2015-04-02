#pragma once

#include "TuringEngine.h"

class CTuringParser
{
public:
	CTuringParser(CTuringEngine* pEngine);

	virtual bool			ParseScript(const wstring& Script);

	static	CExpressions*	GetExpressions(const wstring &Line, wstring* Comment = NULL);
	static	wstring			PrintExpressions(CExpressions* Expressions);
#ifdef USE_JSON
	static	bool			SplitBlocks(const wstring& String, vector<wstring>& StrList, bool bDot = false);
#endif
	virtual SOp*			CompileOp(CExpressions* &Expressions, wstring& Error); 

	virtual wstring			GetScriptLine(int iLine);

	static	wstring			PrintFunction(FxVector* Fx);
	static	wstring			PrintLine(SOp* Op);
	static	wstring			PrintState(SScope& Scope);

protected:
	virtual bool			PreprocessScript(stringlines &ScriptLines, scriptblocks &ScriptBlocks);

	virtual bool			CompileScript(scriptblocks &ScriptBlocks);

	static  void			LineUpScript(const wstring& Script, stringlines &ScriptLines, bool bRectify = true);
	static	wstring			GetWord(const wstring &Line, wstring::size_type &Offset);
#ifdef USE_JSON
	static	int				ExtractBlock(CExpressions* Expressions, map<wstring,wstring> &Blocks, int &Counter);
#endif

	static	bool			OrderEquation(CExpressions* Expressions, bool bSub);


	CTuringEngine*			m_pEngine;

	stringlines				m_ScriptLines;
};