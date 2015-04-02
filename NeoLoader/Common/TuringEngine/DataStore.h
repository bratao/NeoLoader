#pragma once

#include "TuringTypes.h"

class CDataStore
{
public:
	void		StoreData (wstring* Path, wstring* Name, wstring* Value);
	void		RetrieveData (wstring* Path, wstring* Name, wstring* Value);
	void		RemoveData (wstring* Path, wstring* Name);
	void		CopyData (wstring* Path, wstring* Name, wstring* NewPath);

	typedef map<wstring,wstring>	TData;
	typedef map<wstring,TData>		TStore;

	TStore*		GetStore() {return &m_DataStore;}

	TData*		SelectData(wstring* Path, bool New);
	wstring*	SelectEntry(wstring* Path, wstring* Name, bool New);

	static void	Register(CTuringEngine* pEngine);

	wstring		PrintStore();

protected:
	bool		SplitPath (const wstring *Path, wstring* Prefix, wstring* Sufix);
	template <class TYPE> 
	bool		SelectName(TYPE* Store, wstring *Path, bool	New);
	void		EnumStore(wstring* Path, wstring* Name, multimap<wstring,wstring>& Enum);

	TStore	m_DataStore;

private:
	// Data Functions
	static int	FxStoreData			(VarPtrMap* pArgMap, SContext* pContext);
	static int	FxRetrieveData		(VarPtrMap* pArgMap, SContext* pContext);
	static int	FxRemoveData		(VarPtrMap* pArgMap, SContext* pContext);
	static int	FxCopyData			(VarPtrMap* pArgMap, SContext* pContext);
	static int	FxParentPath		(VarPtrMap* pArgMap, SContext* pContext);
};

class CStoreContext: public CContext
{
public:
	virtual CDataStore*	GetStore() = 0;
};