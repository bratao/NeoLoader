/*
 * File JSONValue.h part of the SimpleJSON Library - http://mjpa.in/json
 * 
 * Copyright (C) 2010 Mike Anchor
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <vector>
#include <string>

// Win32 incompatibilities
#if defined(WIN32) && !defined(__GNUC__)
	#define wcsncasecmp _wcsnicmp 
	static inline bool isnan(double x) { return x != x; }
	static inline bool isinf(double x) { return !isnan(x) && isnan(x - x); }
#endif

#include <vector>
#include <string>
#include <map>

// Linux compile fix - from quaker66
#ifdef __GNUC__
	#include <cstring> 
	#include <cstdlib>

#define isnan(x) (x != x)
#define isinf(x) (!isnan(x) && isnan(x - x))
#endif

// Mac compile fixes - from quaker66
//#if defined(__APPLE__) || (defined(WIN32) && defined(__GNUC__))
#if 0
	#include <wctype.h>
	#include <wchar.h>
	
	static inline int wcsncasecmp(const wchar_t *s1, const wchar_t *s2, size_t n)
	{
		int lc1  = 0;
		int lc2  = 0;

		while (n--)
		{
			lc1 = towlower (*s1);
			lc2 = towlower (*s2);

			if (lc1 != lc2)
				return (lc1 - lc2);

			if (!lc1)
				return 0;

			++s1;
			++s2;
		}

		return 0;
	}
#endif

// Simple function to check a string 's' has at least 'n' characters
static inline bool simplejson_wcsnlen(const wchar_t *s, size_t n) {
	if (s == 0)
		return false;

	const wchar_t *save = s;
	while (n-- > 0)
	{
		if (*(save++) == 0) return false;
	}

	return true;
}

// Custom types
class JSONValue;
typedef vector<JSONValue*> JSONArray;
typedef map<wstring, JSONValue*> JSONObject;

class JSON
{
	friend class JSONValue;
	
	public:
		static JSONValue* Parse(const char *data);
		static JSONValue* Parse(const wchar_t *data);
		static wstring Stringify(const JSONValue *value);
	protected:
		static bool SkipWhitespace(const wchar_t **data);
		static bool ExtractString(const wchar_t **data, wstring &str);
		static double ParseInt(const wchar_t **data);
		static double ParseDecimal(const wchar_t **data);
	private:
		JSON();
};


enum JSONType { JSONType_Null, JSONType_String, JSONType_Bool, JSONType_Number, JSONType_Array, JSONType_Object };

class JSONValue
{
	friend class JSON;
	
	public:
		JSONValue(JSONType value_type = JSONType_Null);
		JSONValue(const wchar_t *char_value);
		JSONValue(const wstring &string_value);
		JSONValue(bool bool_value);
		JSONValue(double number_value);
		JSONValue(const JSONArray &array_value);
		JSONValue(const JSONObject &object_value);
		~JSONValue();
		
		bool IsNull() const;

		const wstring ToString() const;

		wstring* AsString();
		bool* AsBool();
		double* AsNumber();
		JSONArray* AsArray();
		JSONObject* AsObject();
		
		wstring Stringify(int Tab = 0) const;
		
	protected:
		static JSONValue *Parse(const wchar_t **data);
	
	private:
		static wstring StringifyString(const wstring &str);
	
		JSONType type;
		union JSONData
		{
			void*		void_value;
			wstring*	string_value;
			bool*		bool_value;
			double*		number_value;
			JSONArray*	array_value;
			JSONObject*	object_value;
		} data;
};
