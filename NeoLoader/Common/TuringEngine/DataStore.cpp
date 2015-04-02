#include "GlobalHeader.h"
#include "DataStore.h"
#include "../Common/Strings.h"
#include "TuringEngine.h"

bool CDataStore::SplitPath(const wstring *Name, wstring* Prefix, wstring* Sufix)
{
	wstring::size_type Pos = Name->find(L"#",Name->rfind(L"/")+1);
	if(Pos == wstring::npos)
		return false;
	*Prefix = Name->substr(0,Pos);
	*Sufix = Name->substr(Pos+1);
	return true;
}

template <class TYPE> 
bool CDataStore::SelectName(TYPE* Store, wstring *Path, bool New)
{
	wstring Prefix;
	wstring Sufix;
	if(!SplitPath(Path, &Prefix, &Sufix))
		return true; // the name is not indexed

	int PrevIndex = 0;
	if(!Sufix.empty())
	{
		if(Sufix.at(Sufix.size()-1) == L'+') // we want the next entry to this one
			PrevIndex = wstring2int(Sufix);
		else 
			return true; // we want a specific name
	}
	int NextIndex = INT_MAX;

	wstring CurPrefix;
	wstring CurSufix;
	int	CurIndex;
	for (typename TYPE::iterator I = Store->begin(); I != Store->end(); I++)
	{
		if(!SplitPath(&I->first, &CurPrefix, &CurSufix) || CurPrefix != Prefix)
			continue;

		CurIndex = wstring2int(CurSufix);
		if(New)
		{
			if(PrevIndex < CurIndex) // find largest free index
				PrevIndex = CurIndex;
		}
		else
		{
			if(PrevIndex < CurIndex && CurIndex < NextIndex) // find smales index after the preview pos
				NextIndex = CurIndex;
		}
	}

	if(NextIndex == INT_MAX)
	{
		if(!New)
		{
			Path->clear();
			return false;
		}
		else
			NextIndex = PrevIndex + 1;
	}
	*Path = Prefix + L"#" + int2wstring(NextIndex);
	return true;
}


CDataStore::TData* CDataStore::SelectData(wstring* Path, bool New)
{
	wstring Root;
	if(Path == NULL)
		Path = &Root;

	if(!SelectName(&m_DataStore,Path,New))
		return NULL;

	TData* Data;
	TStore::iterator I = m_DataStore.find(*Path);
	if(I == m_DataStore.end())
		Data = &m_DataStore[*Path];
	else
		Data = &I->second;

	return Data;
}

wstring* CDataStore::SelectEntry(wstring* Path, wstring* Name, bool New)
{
	TData* Data = SelectData(Path, New);
	if(!Data)
		return NULL;

	if(!SelectName(Data,Name,New))
		return NULL;

	wstring* Entry;
	TData::iterator J = Data->find(*Name);
	if(J == Data->end())
		Entry = &(*Data)[*Name];
	else
		Entry = &J->second;

	return Entry;
}

void CDataStore::StoreData (wstring* Path, wstring* Name, wstring* Value)
{
	if(wstring* Entry = SelectEntry(Path,Name,true))
		*Entry = *Value;
}

void CDataStore::RetrieveData (wstring* Path, wstring* Name, wstring* Value)
{
	if(wstring* Entry = SelectEntry(Path,Name,false))
		*Value = *Entry;
	else
		Value->clear();
}

void CDataStore::EnumStore(wstring* Path, wstring* Name, multimap<wstring,wstring> &Enum)
{
	for(TStore::iterator I = m_DataStore.begin(); I != m_DataStore.end(); I++)
	{
		if(Path && !wildcmpex(Path->c_str(),I->first.c_str()))
			continue;

		if(Name == NULL)
			Enum.insert(pair<wstring,wstring>(I->first,L""));
		else
		{
			for(TData::iterator J = I->second.begin(); J != I->second.end(); J++)
			{
				if(wildcmpex(Name->c_str(),J->first.c_str()))
					Enum.insert(pair<wstring,wstring>(I->first,J->first));
			}
		}
	}
}

void CDataStore::RemoveData (wstring* Path, wstring* Name)
{
	multimap<wstring,wstring> Enum;
	EnumStore(Path,Name,Enum);

	for(multimap<wstring,wstring>::iterator E = Enum.begin(); E != Enum.end(); E++)
	{
		TStore::iterator I = m_DataStore.find(E->first);
		ASSERT(I != m_DataStore.end());
		if(E->second.empty())
			m_DataStore.erase(I);
		else
		{
			TData::iterator J = I->second.find(E->second);
			ASSERT(J != I->second.end());
			I->second.erase(J);
		}
	}
}

void CDataStore::CopyData (wstring* Path, wstring* Name, wstring* NewPath)
{
	multimap<wstring,wstring> Enum;
	EnumStore(Path,Name,Enum);

	wstring CurPath;
	TData* CurData = SelectData(NewPath, true); // Note: this is done to convert an Path# to a fixed Path#n

	for(multimap<wstring,wstring>::iterator E = Enum.begin(); E != Enum.end(); E++)
	{
		wstring TmpPath = *NewPath;
		const wchar_t* rest = wildcmpex(Path->c_str(),E->first.c_str());
		ASSERT(rest);
		if(*rest)
		{
			//if(*rest != L'/')
			//	TmpPath.append(L"/");
			TmpPath.append(rest);
		}
		if(TmpPath != CurPath || !CurData)
		{
			CurPath = TmpPath;
			CurData = SelectData(&CurPath, true);
			ASSERT(CurData);
		}

		TStore::iterator I = m_DataStore.find(E->first);
		ASSERT(I != m_DataStore.end());
		if(E->second.empty())
		{
			for(TData::iterator J = I->second.begin(); J != I->second.end(); J++)
				CurData->insert(pair<wstring,wstring>(J->first,J->second));
		}
		else
		{
			TData::iterator J = I->second.find(E->second);
			ASSERT(J != I->second.end());
			CurData->insert(pair<wstring,wstring>(J->first,J->second));
		}
	}
}

wstring	CDataStore::PrintStore()
{
	wstring Printer;
	for(TStore::iterator I = m_DataStore.begin(); I != m_DataStore.end(); I++)
	{
		for(TData::iterator J = I->second.begin(); J != I->second.end(); J++)
		{
			Printer.append(I->first);
			Printer.append(L"\\");
			Printer.append(J->first);
			Printer.append(L"=");
			Printer.append(J->second);
			Printer.append(L"\n");
		}
	}

	return Printer;
}


//////////////////////////////////////////////////////////////////////////
// Data Functions
//
// The Data store provides a task common data storage.
//
// The values are organized in the store like in a file system, Each value has a name and a path
// The Data Store is implemented as a map of maps of values, meaning that the path and the name can be considered independant
//	Path and name each on its own can be formated like a filesystem path.
//	Notations:  PathA/PathB/PathC \ Name1/Name1 = Value
//
//	For some operations (delete, copy) DOS style (* and ?) wildcard can be used
//
//	The Path as well as the name can be used in a itterative way:
//
//		When Storing data to a paths/names ending with "*#" an index number will be appended resulting in "*#1", than "*#2" ... "*#n", etc.
//			the path/name variable will be updated with the final value
//
//		Sample:
//			Name = "Name#"
//			StoreData(Name=Name, Value=Value)
//			' Now Name="Name#n"
//
//		When Retriving data paths/names ending with "*#n+" will return the first element after "*#n", and the variable will be updated accordingly
//			this way the script can itterate through the data store, 
//			when the end is reached the Name/Path will be emptyed.
//
//		Sample:
//			Name = "Name#"
//			loop
//				Name & "+"
//				RetrieveData(Name=Name, Value=Value)
//				if(!Name) then
//					break
//				' Do Something with Name="Name#n" and Value="..."
//			end
//
//		When the name/path contains already an Index and not "+" on the end, no itteration/incrementation will be done
//
// The Store can be used to exchange informations between sessions. 
// Its main function is to allow the script to return colelcted informations to the task
//

void CDataStore::Register(CTuringEngine* pEngine)
{
	pEngine->RegisterFunction(L"StoreData",FxStoreData);
	pEngine->RegisterFunction(L"RetrieveData",FxRetrieveData);
	pEngine->RegisterFunction(L"RemoveData",FxRemoveData);
	pEngine->RegisterFunction(L"CopyData",FxCopyData);
	pEngine->RegisterFunction(L"ParentPath",FxParentPath);
}

/** StoreData save a value to the data store
* @param: Path: The path of the value  (If path is is not specifyed the root path is assumed)
* @param: Name: The name of the value
* @param: Value: The value
*/
int CDataStore::FxStoreData(VarPtrMap* pArgMap, SContext* pContext)
{
	CDataStore* pStore = pContext->GetContext<CStoreContext>()->GetStore();
	wstring* wPath = GetArgument(pArgMap, L"Path");
	wstring* wName = GetArgument(pArgMap, L"Name");
	wstring* wValue = GetArgument(pArgMap, L"Value");
	if(wName == NULL || wValue == NULL)	// path is optional
		return FX_ERR_ARGUMENT;

	pStore->StoreData(wPath,wName,wValue);
	return FX_OK;
}

/** RetrieveData get a value from the data store
* @param: Path: The path of the value  (If path is is not specifyed the root path is assumed)
* @param: Name: The name of the value
* @param: Value: The value
*/
int CDataStore::FxRetrieveData(VarPtrMap* pArgMap, SContext* pContext)
{
	CDataStore* pStore = pContext->GetContext<CStoreContext>()->GetStore();
	wstring* wPath = GetArgument(pArgMap, L"Path");
	wstring* wName = GetArgument(pArgMap, L"Name");
	wstring* wValue = GetArgument(pArgMap, L"Value");
	if(wName == NULL || wValue == NULL)	// path is optional
		return FX_ERR_ARGUMENT;

	pStore->RetrieveData(wPath,wName,wValue);
	return FX_OK;
}

/** RemoveData erase a value from the data store
* @param: Path: The path of the value (If path is is not specifyed the root path is assumed)
* @param: Name: The name of the value
*/
int CDataStore::FxRemoveData(VarPtrMap* pArgMap, SContext* pContext)
{
	CDataStore* pStore = pContext->GetContext<CStoreContext>()->GetStore();
	wstring* wPath = GetArgument(pArgMap, L"Path");
	wstring* wName = GetArgument(pArgMap, L"Name");
	if(wPath == NULL && wName == NULL)
		return FX_ERR_ARGUMENT;

	pStore->RemoveData(wPath,wName);
	return FX_OK;
}

/** CopyData duplicate data in the store
* @param: Path: Source Path
* @param: Name: Value Name (optional, if empty all data n the source path will be copyed)
* @param: NewPath: Target Path
*/
int CDataStore::FxCopyData(VarPtrMap* pArgMap, SContext* pContext)
{
	CDataStore* pStore = pContext->GetContext<CStoreContext>()->GetStore();
	wstring* wPath = GetArgument(pArgMap, L"Path");
	wstring* wName = GetArgument(pArgMap, L"Name");
	wstring* wNewPath = GetArgument(pArgMap, L"NewPath");
	if(wPath == NULL || wNewPath == NULL)
		return FX_ERR_ARGUMENT;

	pStore->CopyData(wPath,wName,wNewPath);
	return FX_OK;
}

/** ParentPath Get a path on level igher
* @param: Path: Path
* @param: Parent: Parent path
*/
int CDataStore::FxParentPath(VarPtrMap* pArgMap, SContext* pContext)
{
	wstring* wPath = GetArgument(pArgMap, L"Path");
	wstring* wParent = GetArgument(pArgMap, L"Parent");
	if(wPath == NULL || wParent == NULL)
		return FX_ERR_ARGUMENT;

	wstring::size_type Pos = wPath->rfind(L"/");
	if(Pos == wstring::npos)
		*wParent = L"";
	else
		*wParent = wPath->substr(0,Pos);
	return FX_OK;
}