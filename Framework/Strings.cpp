#include "GlobalHeader.h"
#include "Strings.h"
#ifdef WIN32
#include <wchar.h>
#endif

QString UnEscape(QString Text)
{
	QString Value;
	bool bEsc = false;
	for(int i = 0; i < Text.size(); i++)
	{
		QChar Char = Text.at(i);
		if(bEsc)
		{
			switch(Char.unicode())
			{
				case L'\\':	Value += L'\\';	break;
				case L'\'':	Value += L'\'';	break;
				case L'\"':	Value += L'\"';	break;
				case L'a':	Value += L'\a';	break;
				case L'b':	Value += L'\b';	break;
				case L'f':	Value += L'\f';	break;
				case L'n':	Value += L'\n';	break;
				case L'r':	Value += L'\r';	break;
				case L't':	Value += L'\t';	break;
				case L'v':	Value += L'\v';	break;
				default:	Value += Char.unicode();break;
			}
			bEsc = false;
		}
		else if(Char == L'\\')
			bEsc = true;
		else
			Value += Char;
	}	
	return Value;
}

//////////////////////////////////////////////////////////////
// Number string Conversion
//

wstring int2wstring(int Int)
{
	wstringstream wVal;
	wVal << Int;
	return wVal.str();
}

int wstring2int(const wstring& Str)
{
	wstringstream wVal(Str);
	int Int = 0;
	wVal >> Int;
	return Int;
}

string int2string(int Int)
{
	stringstream Val;
	Val << Int;
	return Val.str();
}

int string2int(const string& Str)
{
	stringstream Val(Str);
	int Int = 0;
	Val >> Int;
	return Int;
}

wstring double2wstring(double Double)
{
	wstringstream wVal;
	wVal.precision(16);
	wVal << Double;
	return wVal.str();
}

double wstring2double(const wstring& Str)
{
	wstringstream wVal(Str);
	double Double = 0.0;
	wVal >> Double;
	return Double;
}

//////////////////////////////////////////////////////////////
// String Comparations
//

/*bool CompareStr(const string &StrL,const string &StrR)
{
#ifndef WIN32
	if(strcasecmp(StrL.c_str(), StrR.c_str()) == 0)
#else
	if(stricmp(StrL.c_str(), StrR.c_str()) == 0)
#endif
		return true;
	return false;
}*/

bool CompareStr(const wstring &StrL,const wstring &StrR)
{
#ifndef WIN32
	if(wcscasecmp(StrL.c_str(), StrR.c_str()) == 0)
#else
	if(_wcsicmp(StrL.c_str(), StrR.c_str()) == 0)
#endif
		return true;
	return false;
}

bool CompareStrs(const wstring &StrL, const wstring &StrR, wchar_t SepC, bool bTrim)
{
	for(string::size_type Pos = 0;Pos < StrR.size();)
	{
		string::size_type Sep = StrR.find(SepC,Pos);
		if(Sep == string::npos)
			Sep = StrR.size();

		size_t CountL = StrL.size();
		size_t CountR = Sep-Pos;
		if(bTrim && CountL > CountR)
			CountL = CountR;
		if(compareex(StrL.c_str(),0,CountL,StrR.c_str(),Pos,CountR) == 0)
			return true;
		Pos = Sep+1;
	}
	return false;
}

/*int compareex(const string &StrL, size_t OffL, size_t CountL, const string &StrR, size_t OffR, size_t CountR)
{
	ASSERT(OffL+CountL <= StrL.size());
	ASSERT(OffR+CountR <= StrR.size());
	if(CountL == CountR)
#ifndef WIN32
		return strncasecmp(StrL.c_str()+OffL, StrR.c_str()+OffR, Min(CountL,CountR));
#else
		return _strnicmp(StrL.c_str()+OffL, StrR.c_str()+OffR, CountL);
		//return _memicmp(StrL.c_str()+OffL, StrR.c_str()+OffR, CountL);
#endif
	return CountR - CountL;
}*/

int compareex(const wchar_t *StrL, size_t OffL, size_t CountL, const wchar_t* StrR, size_t OffR, size_t CountR)
{
	if(CountL == CountR)
#ifndef WIN32
		return wcsncasecmp(StrL+OffL, StrR+OffR, CountL);
#else
		return _wcsnicmp(StrL+OffL, StrR+OffR, CountL);
#endif
	if(CountR > CountL)
		return 1;
	return -1;
}

//////////////////////////////////////////////////////////////
// String Operations
//

wstring MkLower(wstring Str)
{
	for(wstring::size_type i = 0; i < Str.size(); i++)
	{
		wstring::value_type &Char = Str.at(i);
		if((Char >= L'A') && (Char <= L'Z'))
			Char += 32;
	}
	return Str;
}

wstring MkUpper(wstring Str)
{
	for(wstring::size_type i = 0; i < Str.size(); i++)
	{
		wstring::value_type &Char = Str.at(i);
		if((Char >= L'a') && (Char <= L'z'))
			Char -= 32;
	}
	return Str;
}

wstring SubStrAt(const wstring& String, const wstring& Separator, int Index)
{
	wstring::size_type Pos = 0;
	for(;;)
	{
		wstring::size_type Sep = String.find(Separator,Pos);
		if(Sep != wstring::npos)
		{
			if(Index == 0)
				return String.substr(Pos,Sep-Pos);
			Pos = Sep+1;
		}
		else
		{
			if(Index == 0)
				return String.substr(Pos);
			break;
		}
		Index--;
	}
	return L"";
}

wstring::size_type FindNth(const wstring& String, const wstring& Separator, int Index)
{
	if(Index == 0)
		return 0;
	wstring::size_type Pos = 0;
	for(;;)
	{
		Index--;
		wstring::size_type Sep = String.find(Separator,Pos);
		if(Sep != wstring::npos)
		{
			if(Index == 0)
				return Sep;
			Pos = Sep+1;
		}
		else
			break;
	}
	return String.size();
}

wstring::size_type FindNthR(const wstring& String, const wstring& Separator, int Index)
{
	if(Index == 0)
		return 0;
	wstring::size_type Pos = String.size();
	for(;;)
	{
		Index--;
		wstring::size_type Sep = String.rfind(Separator,Pos);
		if(Sep != wstring::npos && Sep != 0)
		{
			if(Index == 0)
				return String.size()-(Sep+1);
			Pos = Sep-1;
		}
		else
			break;	
	}
	return String.size();
}

int CountSep(const wstring& String, const wstring& Separator)
{
	wstring::size_type Pos = 0;
	int Index = 0;
	for(;;)
	{
		Index++;
		wstring::size_type Sep = String.find(Separator,Pos);
		if(Sep != wstring::npos)
			Pos = Sep+1;
		else
			break;
	}
	return Index;
}

//bool RegExpMatch(const wstring& Value, const wstring& RegExp, bool bWildcard, bool bCaseSensitive)
//{
//	return QRegExp(QString::fromStdWString(RegExp), bCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive, bWildcard ? QRegExp::Wildcard : QRegExp::RegExp).exactMatch(QString::fromStdWString(Value));
//}

////////////////////////////////////////////////////////
//

__inline bool wmemtstex(const wchar_t Chr1, const wchar_t Chr2)
{
	wchar_t L = ((Chr1 >= L'A') && (Chr1 <= L'Z')) ? Chr1 + 32 : Chr1;	// L'a' - L'A' == 32
	wchar_t R = ((Chr2 >= L'A') && (Chr2 <= L'Z')) ? Chr2 + 32 : Chr2;
	return L == R;
}

const wchar_t* wmemchrex(const wchar_t *Str, wchar_t Find, size_t size)
{
	for (; 0 < size; ++Str, --size)
	{
		//if (*Str == Find)
		if (wmemtstex(*Str,Find))
			return Str;
	}
	return NULL; 
}

int wmemcmpex(const wchar_t *Str1, const wchar_t *Str2, size_t size)
{
	for (; 0 < size; ++Str1, ++Str2, --size)
	{
		//if (*Str1 != *Str2)
		//	return (*Str1 < *Str2 ? -1 : +1);
		if(!wmemtstex(*Str1,*Str2))
			return (*Str1 < *Str2 ? -1 : +1);
	}
	return 0; 
}

size_t findnex(const wchar_t* str, const wchar_t* find, size_t count, size_t off = 0)
{
	size_t size = wcslen(str);
	if (count == 0 && off <= size)
		return (off);	// null string always matches (if inside string)

	size_t togo;
	if (off < size && count <= (togo = size - off))
	{	// room for match, look for it
		const wchar_t *test, *next;
		for (togo -= count - 1, next = str + off;
			(test = wmemchrex(next, *find, togo)) != 0;
			togo -= test - next + 1, next = test + 1)
		{
			if (wmemcmpex(test, find, count) == 0)
				return (test - str);	// found a match
		}
	}

	return (wstring::npos);	// no match
}

wstring::size_type FindStr(const wstring& str, const wstring& find, wstring::size_type Off)
{
	return findnex(str.c_str(),find.c_str(),find.size(),Off);
}

size_t rfindnex(const wchar_t* str, const wchar_t* find, size_t count, size_t off = 0)
{
	size_t size = wcslen(str);
	if (count == 0)
		return (off < size ? off : size);	// null always matches

	if (count <= size)
	{	// room for match, look for it
		const wchar_t *test = str + (off < size - count ? off : size - count);
		for (; ; --test)
		{
			if (wmemtstex(*test, *find) && wmemcmpex(test, find, count) == 0)
				return (test - str);	// found a match
			else if (test == str)
				break;	// at beginning, no more chance for match
		}
	}

	return (wstring::npos);	// no match
}

wstring::size_type RFindStr(const wstring& str, const wstring& find, wstring::size_type Off)
{
	return rfindnex(str.c_str(),find.c_str(),find.size(),Off);
}


////////////////////////////////////////////////////////
//

wchar_t* UTF8toWCHAR(const char *str)
{
	// Skip BOM
	if(str[0] == 0xEF && str[1] == 0xBB && str[2] == 0xBF)
		str+=3;

	size_t pos = 0;
//		len=0;
//	for (const char *tmp=str;*tmp;tmp++)
//		if (((uchar)*tmp)<128 || (*tmp&192)==192)
//			len++;
	bool tst = false;
	wchar_t* res = new wchar_t[strlen(str) + 1];
	for (char* tmp = (char*)str; *tmp; tmp++)
	{
		if (!(*tmp&128))
		{
			//Byte represents an ASCII character. Direct copy will do.
			res[pos] = *tmp;
			tst = true;
		}
		else if ((*tmp&192)==128)
		{
			if(tst)
				res[pos] = *tmp;
			else //Byte is the middle of an encoded character. Ignore.
				continue;
		}
		else if ((*tmp&224) == 192)
		{
			//Byte represents the start of an encoded character in the range
			//U+0080 to U+07FF
			res[pos] = ((*tmp&31) << 6) | tmp[1]&63;
			tst = false;
		}
		else if ((*tmp&240) == 224)
		{
			//Byte represents the start of an encoded character in the range
			//U+07FF to U+FFFF
			res[pos]=((*tmp&15) << 12)|((tmp[1]&63) << 6) | tmp[2]&63;
			tst = false;
		}
		else if ((*tmp&248) == 240)
		{
			//Byte represents the start of an encoded character beyond the
			//U+FFFF limit of 16-bit integers
			res[pos] = '?';
			tst = false;
		}
		pos++;
	}
	res[pos] = 0;
	return res;
}

// Function set taken form libtorrent
bool valid_path_character(char c)
{
#ifdef WIN32
	static const char invalid_chars[] = "?<>\"|\b*:";
#else
	static const char invalid_chars[] = "";
#endif
	if (c >= 0 && c < 32) return false;
    return strchr(invalid_chars, c) == 0;
}

void convert_to_utf8(std::string& str, unsigned char chr)
{
	str += 0xc0 | ((chr & 0xff) >> 6);
	str += 0x80 | (chr & 0x3f);
}

bool verify_encoding(std::string& target, bool fix_paths)
{
	std::string tmp_path;
	bool valid_encoding = true;
	for (std::string::iterator i = target.begin()
		, end(target.end()); i != end; ++i)
	{
		// valid ascii-character
		if ((*i & 0x80) == 0)
		{
			// replace invalid characters with '.'
			if (!fix_paths || valid_path_character(*i))
			{
				tmp_path += *i;
			}
			else
			{
				tmp_path += '_';
				valid_encoding = false;
			}
			continue;
		}
			
		if (end - i < 2)
		{
			convert_to_utf8(tmp_path, *i);
			valid_encoding = false;
			continue;
		}
			
		// valid 2-byte utf-8 character
		if ((i[0] & 0xe0) == 0xc0
			&& (i[1] & 0xc0) == 0x80)
		{
			tmp_path += i[0];
			tmp_path += i[1];
			i += 1;
			continue;
		}

		if (end - i < 3)
		{
			convert_to_utf8(tmp_path, *i);
			valid_encoding = false;
			continue;
		}

		// valid 3-byte utf-8 character
		if ((i[0] & 0xf0) == 0xe0
			&& (i[1] & 0xc0) == 0x80
			&& (i[2] & 0xc0) == 0x80)
		{
			tmp_path += i[0];
			tmp_path += i[1];
			tmp_path += i[2];
			i += 2;
			continue;
		}

		if (end - i < 4)
		{
			convert_to_utf8(tmp_path, *i);
			valid_encoding = false;
			continue;
		}

		// valid 4-byte utf-8 character
		if ((i[0] & 0xf0) == 0xe0
			&& (i[1] & 0xc0) == 0x80
			&& (i[2] & 0xc0) == 0x80
			&& (i[3] & 0xc0) == 0x80)
		{
			tmp_path += i[0];
			tmp_path += i[1];
			tmp_path += i[2];
			tmp_path += i[3];
			i += 3;
			continue;
		}

		convert_to_utf8(tmp_path, *i);
		valid_encoding = false;
	}
	// the encoding was not valid utf-8
	// save the original encoding and replace the
	// commonly used path with the correctly
	// encoded string
	if (!valid_encoding) target = tmp_path;
	return valid_encoding;
}
