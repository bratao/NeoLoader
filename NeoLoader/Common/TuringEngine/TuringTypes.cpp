#include "GlobalHeader.h"
#include "TuringTypes.h"
#include "TuringOps.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
// SExpression

SExpression::SExpression(const wstring &Word)
{
	ASSERT(!Word.empty());
	if(CTuringOps::IsOperator(Word.at(0)))
		Type = eOperator;
	else if(Word.at(0) == L'"')
		Type = eString;
	else
		Type = eWord;
	Exp.Word = new wstring(Word);
}
SExpression::SExpression(CExpressions* Expressions)
{
	Type = eMulti;
	Exp.Multi = Expressions;
}
SExpression::~SExpression()
{
	if(Type != eMulti)
		delete Exp.Word;
	else if(Exp.Multi)
		delete Exp.Multi;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// CExpressions

CExpressions::~CExpressions()
{
	for (vector<SExpression*>::iterator K = m_Expressions.begin(); K != m_Expressions.end(); K++)
		delete (*K);
}

SExpression** CExpressions::GetExp(UINT Index)
{
	if(m_Expressions.size() <= Index)
		return NULL;
	SExpression** Ret = &m_Expressions.at(Index);
	ASSERT(*Ret != NULL);
	return Ret;
}

wstring CExpressions::GetExpStr(UINT Index)
{
	SExpression** Expression = GetExp(Index);
	if(!Expression || (*Expression)->IsMulti())
		return L"";
	return *(*Expression)->Exp.Word;
}

bool CExpressions::IsExpMulti(UINT Index)
{
	SExpression** Expression = GetExp(Index);
	if(!Expression)
		return false;
	return (*Expression)->IsMulti();
}

bool CExpressions::IsExpOp(UINT Index)
{
	SExpression** Expression = GetExp(Index);
	if(!Expression)
		return false;
	return (*Expression)->IsOperator();
}

bool CExpressions::SubordinateExp(int Index, int ToGo)
{
	if(ToGo == 0)
		return true;
	if(Index < 0 || Index + ToGo > m_Expressions.size())
		return false;

	CExpressions* SubExpression = new CExpressions;
	for (; ToGo > 0; ToGo--)
    {
		vector<SExpression*>::iterator I = m_Expressions.begin() + Index;
		SubExpression->m_Expressions.push_back(*I);
		m_Expressions.erase(I);
	}
	m_Expressions.insert(m_Expressions.begin() + Index, new SExpression(SubExpression));
	return true;
}

void CExpressions::Del(UINT Index)
{
	vector<SExpression*>::iterator I = m_Expressions.begin() + Index;
	if(I != m_Expressions.end())
	{
		delete *I;
		m_Expressions.erase(I);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// STuringBlock

STuringBlock::STuringBlock()
{
	eType = eNone;
	pSupBlock = NULL;
}

STuringBlock::~STuringBlock()
{
	delete pSupBlock;
}

void STuringBlock::Push(EBlockType Type, STuringBlock* &pBlock)
{
	STuringBlock* pNewBlock = new STuringBlock();
	pNewBlock->eType = Type;
	pNewBlock->pSupBlock = pBlock;
	pBlock = pNewBlock;
}

void STuringBlock::Pop(STuringBlock* &pBlock)
{
	STuringBlock* pOldBlock = pBlock;
	pBlock = pOldBlock->pSupBlock;
	pOldBlock->pSupBlock = NULL;
	delete pOldBlock;
}
