/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_DCPP_UTIL_H
#define DCPLUSPLUS_DCPP_UTIL_H

#include "compiler.h"

#ifdef _WIN32

# define PATH_SEPARATOR '\\'
# define PATH_SEPARATOR_STR "\\"
# define PATH_SEPARATOR_WSTR L"\\"

#define FLYLINKDC_REGISTRY_PATH _T("SOFTWARE\\FlylinkDC++")
#define FLYLINKDC_REGISTRY_MEDIAINFO_FREEZE_KEY _T("MediaFreezeInfo")
#define FLYLINKDC_REGISTRY_MEDIAINFO_CRASH_KEY  _T("MediaCrashInfo")
#define FLYLINKDC_REGISTRY_SQLITE_ERROR  _T("SQLiteError")
#define FLYLINKDC_REGISTRY_LEVELDB_ERROR  _T("LevelDBError")

#include <wininet.h>
#include <atlcomtime.h>

#else

# define PATH_SEPARATOR '/'
# define PATH_SEPARATOR_STR "/"

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#endif

#include <wininet.h>
#include "Text.h"
#include "Thread.h"
#include <atlcomtime.h>

namespace dcpp
{

class CInternetHandle
#ifdef _DEBUG
	:  boost::noncopyable // [+] IRainman fix.
#endif
{
    public:
		explicit CInternetHandle(HINTERNET p_hInternet): m_hInternet(p_hInternet)
        {
        }
		~CInternetHandle()
        {
			if (m_hInternet)
		{
                ::InternetCloseHandle(m_hInternet);
        }
		}
        operator const HINTERNET() const
        {
            return m_hInternet;
        }
    protected:
		const HINTERNET m_hInternet;
};

template <class T>
void AppendPathSeparator(T& p_path) //[+]PPA
{
	if (!p_path.empty())
        if (p_path[ p_path.length() - 1 ] != PATH_SEPARATOR)
            p_path += PATH_SEPARATOR;
}

#define URI_SEPARATOR '/'
#define URI_SEPARATOR_STR "/"
#define URI_SEPARATOR_WSTR L"/"
template <class T>
static void AppendUriSeparator(T& p_path) //[+]SSA
{
	if (!p_path.empty())
		if (p_path[ p_path.length() - 1 ] != URI_SEPARATOR)
			p_path += URI_SEPARATOR;
}


template<typename T, bool flag> struct ReferenceSelector
{
    typedef T ResultType;
};
template<typename T> struct ReferenceSelector<T, true>
{
    typedef const T& ResultType;
};

template<typename T> class IsOfClassType
{
    public:
        template<typename U> static char check(int U::*);
        template<typename U> static float check(...);
    public:
        enum { Result = sizeof(check<T>(0)) };
};

template<typename T> struct TypeTraits
{
    typedef IsOfClassType<T> ClassType;
    typedef ReferenceSelector < T, ((ClassType::Result == 1) || (sizeof(T) > sizeof(char*))) > Selector;
    typedef typename Selector::ResultType ParameterType;
};

#define GETSET(type, name, name2) \
    private: type name; \
    public: TypeTraits<type>::ParameterType get##name2() const { return name; } \
    void set##name2(TypeTraits<type>::ParameterType a##name2) { name = a##name2; }

#define GETM(type, name, name2) \
	private: type name; \
	public: TypeTraits<type>::ParameterType get##name2() const { return name; }

#define GETC(type, name, name2) \
	private: const type name; \
	public: TypeTraits<type>::ParameterType get##name2() const { return name; }

#define LIT(x) x, (sizeof(x)-1)

/** Evaluates op(pair<T1, T2>.first, compareTo) */
template < class T1, class T2, class op = std::equal_to<T1> >
class CompareFirst
{
    public:
        CompareFirst(const T1& compareTo) : a(compareTo) { }
        bool operator()(const pair<T1, T2>& p)
        {
            return op()(p.first, a);
        }
    private:
        CompareFirst& operator=(const CompareFirst&);
        const T1& a;
};

/** Evaluates op(pair<T1, T2>.second, compareTo) */
template<class T1, class T2, class op = equal_to<T2> >
class CompareSecond
{
    public:
        CompareSecond(const T2& compareTo) : a(compareTo) { }
        bool operator()(const pair<T1, T2>& p)
        {
            return op()(p.second, a);
        }
    private:
        CompareSecond& operator=(const CompareSecond&);
        const T2& a;
};

/** Evaluates op(pair<T1, T2>.second, compareTo) */
template<class T1, class T2, class T3, class op = equal_to<T2> >
class CompareSecondFirst
{
    public:
        CompareSecondFirst(const T2& compareTo) : a(compareTo) { }
        bool operator()(const pair<T1, pair<T2, T3>>& p)
        {
            return op()(p.second.first, a);
        }
    private:
        CompareSecondFirst& operator=(const CompareSecondFirst&);
        const T2& a;
};

/**
 * Compares two values
 * @return -1 if v1 < v2, 0 if v1 == v2 and 1 if v1 > v2
 */
template<typename T1>
inline int compare(const T1& v1, const T1& v2)
{
    return (v1 < v2) ? -1 : ((v1 == v2) ? 0 : 1);
}

template<class T, int N>
class LocalArray
{
    public:
        T m_data[N];
        LocalArray()
        {
            m_data[0]   = 0;
        }
        operator T*()
        {
            return m_data;
        }
        static int size()
        {
            return N;
        }
        int size_of() const
        {
            return N * sizeof(T);
        }
        T* data()
        {
            return m_data;
        }
        void init()
        {
            memzero(m_data, sizeof(m_data));
        }
};
class IDateReceiveReporter
{
    public:
        virtual bool ReportResultWait(DWORD totalDataWait) = 0;
        virtual bool ReportResultReceive(DWORD currentDataReceive) = 0;
        virtual bool SetCurrentStage(std::string& value) = 0;
};

template<typename T>
class AutoArray
{
        typedef T* TPtr;
    public:
        explicit AutoArray(TPtr t) : p(t) { }
        explicit AutoArray(size_t size) : p(new T[size]) { }
        ~AutoArray()
        {
            delete[] p;
        }
        operator TPtr()
        {
            return p;
        }
        TPtr data()
        {
            return p;
        }
        TPtr get()
        {
            return p;
        }
        AutoArray& operator=(TPtr t)
        {
            delete[] p;
            p = t;
            return *this;
        }
    private:
        AutoArray(const AutoArray&);
        AutoArray& operator=(const AutoArray&);
        
        TPtr p;
};

class Util
{
    public:
        static tstring emptyStringT;
        static string emptyString;
        static wstring emptyStringW;
        
        enum Paths
        {
            /** Global configuration */
            PATH_GLOBAL_CONFIG,
            /** Per-user configuration (queue, favorites, ...) */
            PATH_USER_CONFIG,
            /** Per-user local data (cache, temp files, ...) */
            PATH_USER_LOCAL,
            /** Various resources (help files etc) */
            PATH_RESOURCES,
            /** Translations */
            PATH_LOCALE,
            /** Default download location */
            PATH_DOWNLOADS,
            /** Default file list location */
            PATH_FILE_LISTS,
            /** Default hub list cache */
            PATH_HUB_LISTS,
            /** Where the notepad file is stored */
            PATH_NOTEPAD,
            /** Folder with emoticons packs*/
            PATH_EMOPACKS,
            PATH_LAST
        };
        
        static void initialize();
        static void load_customlocations(); //[+]FlylinkDC++
        static void load_compress_ext(); //[+]FlylinkDC++
        static tstring getCompileDate(const LPCTSTR& p_format = _T("%Y-%m-%d"))
        {
            COleDateTime tCompileDate;
            tCompileDate.ParseDateTime(_T(__DATE__), LOCALE_NOUSEROVERRIDE, 1033);
            return tCompileDate.Format(p_format).GetString();
        }
        
        static tstring getCompileTime(const LPCTSTR& p_format = _T("%H-%M-%S"))
        {
            COleDateTime tCompileDate;
            tCompileDate.ParseDateTime(_T(__TIME__), LOCALE_NOUSEROVERRIDE, 1033);
            return tCompileDate.Format(p_format).GetString();
        }
		static bool isValidIP(const string& p_ip)
		{
			uint32_t a[4] = {0};
			const int l_Items = sscanf_s(p_ip.c_str(), "%u.%u.%u.%u", &a[0], &a[1], &a[2], &a[3]);
			return  l_Items == 4 && a[0] < 256 && a[1] < 256 && a[2] < 256 && a[3] < 256; // TODO - boost
		}
        
        /** Path of temporary storage */
        static string getTempPath()
        {
#ifdef _WIN32
            TCHAR buf[MAX_PATH + 1];
            DWORD x = GetTempPath(MAX_PATH, buf);
            return Text::fromT(tstring(buf, x));
#else
            return "/tmp/";
#endif
        }
        
        /** Path of configuration files */
        static const string& getPath(Paths path)
        {
            return paths[path];
        }
        
        /** Migrate from pre-localmode config location */
        static void migrate(const string& file);
        
        /** Path of file lists */
        static string getListPath()
        {
            return getPath(PATH_FILE_LISTS);
        }
        /** Path of hub lists */
        static string getHubListsPath()
        {
            return getPath(PATH_HUB_LISTS);
        }
        /** Notepad filename */
        static string getNotepadFile()
        {
            return getPath(PATH_NOTEPAD);
        }
        
        static string translateError(int aError);
        static TCHAR* strstr(const TCHAR *str1, const TCHAR *str2, int *pnIdxFound); //[+]PPA
        
        static time_t getStartTime()
        {
            return startTime;
        }
        
        static string getFilePath(const string& path)
        {
            string::size_type i = path.rfind(PATH_SEPARATOR);
            return (i != string::npos) ? path.substr(0, i + 1) : path;
        }
        static string getFileName(const string& path)
        {
            string::size_type i = path.rfind(PATH_SEPARATOR);
            return (i != string::npos) ? path.substr(i + 1) : path;
        }
        static string getFileExt(const string& path)
        {
            string::size_type i = path.rfind('.');
            return (i != string::npos) ? path.substr(i) : Util::emptyString;
        }
		static string getFileExtWithoutDot(const string& path)
		{
			const auto i = path.rfind('.');
			return i != string::npos ? path.substr(i + 1) : Util::emptyString;
		}
		static wstring getFileExtWithoutDot(const wstring& path)
		{
			const auto i = path.rfind('.');
			return i != wstring::npos ? path.substr(i + 1) : Util::emptyStringW;
		}

        static string getLastDir(const string& path)
        {
            string::size_type i = path.rfind(PATH_SEPARATOR);
            if (i == string::npos)
                return Util::emptyString;
            string::size_type j = path.rfind(PATH_SEPARATOR, i - 1);
            return (j != string::npos) ? path.substr(j + 1, i - j - 1) : path;
        }
        
        static wstring getFilePath(const wstring& path)
        {
            wstring::size_type i = path.rfind(PATH_SEPARATOR);
            return (i != wstring::npos) ? path.substr(0, i + 1) : path;
        }
        static wstring getFileName(const wstring& path)
        {
            wstring::size_type i = path.rfind(PATH_SEPARATOR);
            return (i != wstring::npos) ? path.substr(i + 1) : path;
        }
        static wstring getFileExt(const wstring& path)
        {
            wstring::size_type i = path.rfind('.');
            return (i != wstring::npos) ? path.substr(i) : Util::emptyStringW;
        }
        static wstring getLastDir(const wstring& path)
        {
            wstring::size_type i = path.rfind(PATH_SEPARATOR);
            if (i == wstring::npos)
                return Util::emptyStringW;
            wstring::size_type j = path.rfind(PATH_SEPARATOR, i - 1);
            return (j != wstring::npos) ? path.substr(j + 1, i - j - 1) : path;
        }
        
        template<typename string_t>
        static void replace(const string_t& search, const string_t& replacement, string_t& str)
        {
            typename string_t::size_type i = 0;
            while ((i = str.find(search, i)) != string_t::npos)
            {
                str.replace(i, search.size(), replacement);
                i += replacement.size();
            }
        }
        template<typename string_t>
        static inline void replace(const typename string_t::value_type* search, const typename string_t::value_type* replacement, string_t& str)
        {
            replace(string_t(search), string_t(replacement), str);
        }
        
        static void decodeUrl(const string& aUrl, string& protocol, string& host, uint16_t& port, string& path, string& query, string& fragment)
        {
            bool isSecure;
            decodeUrl(aUrl, protocol, host, port, path, isSecure, query, fragment);
        }
        static void decodeUrl(const string& aUrl, string& protocol, string& host, uint16_t& port, string& path, bool& isSecure, string& query, string& fragment);
        static map<string, string> decodeQuery(const string& query);
        
        static string validateFileName(string aFile);
        static string cleanPathChars(string aNick);
        static string formatStatus(int iStatus);
        
        static string formatBytes(const string& aString)
        {
            return formatBytes(toInt64(aString));
        }
        
		static wstring formatBytesW(const wstring& aString) // [+] IRainman opt
		{
			return formatBytesW(toInt64(aString));
		}
		
        static string getShortTimeString(time_t t = time(NULL));
        
        static string getTimeString();
        static string toAdcFile(const string& file);
        static string toNmdcFile(const string& file);
        
		static string formatBytes(int64_t aBytes); // TODO - template?
		static string formatBytes(uint32_t aBytes)
		{
			return formatBytes(int64_t(aBytes));
		}
		static string formatBytes(double aBytes); // TODO - template?
		static string formatBytes(uint64_t aBytes)
		{
			return formatBytes(double(aBytes));
		}
		static string formatBytes(unsigned long aBytes)
		{
			return formatBytes(double(aBytes));
		}
        static wstring formatBytesW(int64_t aBytes);
        
        static wstring formatExactSize(int64_t aBytes);
        
        static wstring formatSeconds(int64_t aSec, bool supressHours = false)
        {
            wchar_t buf[64];
            if (!supressHours)
                snwprintf(buf, _countof(buf), L"%01lu:%02d:%02d", (unsigned long)(aSec / (60 * 60)), (int)((aSec / 60) % 60), (int)(aSec % 60));
            else
                snwprintf(buf, _countof(buf), L"%02d:%02d", (int)(aSec / 60), (int)(aSec % 60));
            return buf;
        }
        
        static string formatParams(const string& msg, const StringMap& params, bool filter, const time_t t = time(NULL));
        static string formatTime(const string &msg, const time_t t);
        static string formatRegExp(const string& msg, const StringMap& params);
        
        static inline int64_t roundDown(int64_t size, int64_t blockSize)
        {
            return ((size + blockSize / 2) / blockSize) * blockSize;
        }
        
        static inline int64_t roundUp(int64_t size, int64_t blockSize)
        {
            return ((size + blockSize - 1) / blockSize) * blockSize;
        }
        
        static inline int roundDown(int size, int blockSize)
        {
            return ((size + blockSize / 2) / blockSize) * blockSize;
        }
        
        static inline int roundUp(int size, int blockSize)
        {
            return ((size + blockSize - 1) / blockSize) * blockSize;
        }
        
		static int64_t toInt64(const string& aString) // [+] IRainman opt
		{
			return toInt64(aString.c_str());
		}
        
		static int64_t toInt64(const char* aString)
        {
#ifdef _WIN32
			return _atoi64(aString);
#else
			return strtoll(aString, (char **)NULL, 10);
#endif
        }
        
		static int64_t toInt64(const wstring& aString) // [+] IRainman opt
		{
			return toInt64(aString.c_str());
		}
		
		static int64_t toInt64(const wchar_t* aString) // [+] IRainman opt
		{
#ifdef _WIN32
			return _wtoi64(aString);
#else
			// TODO return strtoll(aString, (char **)NULL, 10);
#endif
		}
		
        static int toInt(const string& aString)
        {
			return toInt(aString.c_str());
        }
		
		static int toInt(const char* aString) // [+] IRainman opt
		{
			return atoi(aString);
		}
		
		static int toInt(const wstring& aString) // [+] IRainman opt
		{
			return toInt(aString.c_str());
		}
		
		static int toInt(const wchar_t* aString) // [+] IRainman opt
		{
			return _wtoi(aString);
		}
		
        static uint32_t toUInt32(const string& str)
        {
            return toUInt32(str.c_str());
        }
        static uint32_t toUInt32(const char* c)
        {
#ifdef _MSC_VER
            /*
            * MSVC's atoi returns INT_MIN/INT_MAX if out-of-range; hence, a number
            * between INT_MAX and UINT_MAX can't be converted back to uint32_t.
            */
			uint32_t ret = atoi(c);
			if ((ret == INT_MAX || ret == INT_MIN) && errno == ERANGE)
			{
				ret = (uint32_t)_atoi64(c);
			}
            return ret;
#else
            return (uint32_t)atoi(c);
#endif
        }
        
        static double toDouble(const string& aString)
        {
            // Work-around for atof and locales...
            lconv* lv = localeconv();
            string::size_type i = aString.find_last_of(".,");
            if (i != string::npos && aString[i] != lv->decimal_point[0])
            {
                string tmp(aString);
                tmp[i] = lv->decimal_point[0];
                return atof(tmp.c_str());
            }
            return atof(aString.c_str());
        }
        
        static float toFloat(const string& aString)
        {
            return (float)toDouble(aString);
        }
        
        static string toString(short val)
        {
            char buf[8];
			snprintf(buf, _countof(buf), "%d", (int)val);
            return buf;
        }
		static string toString(uint16_t val)
        {
            char buf[8];
			snprintf(buf, _countof(buf), "%u", (unsigned int)val);
            return buf;
        }
        static string toString(int val)
        {
            char buf[16];
			snprintf(buf, _countof(buf), "%d", val);
            return buf;
        }
		static string toStringPercent(int val)
		{
			char buf[16];
			snprintf(buf, _countof(buf), "%d%%", val);
			return buf;
		}
        static string toString(unsigned int val)
        {
            char buf[16];
			snprintf(buf, _countof(buf), "%u", val);
            return buf;
        }
        static string toString(long val)
        {
			char buf[24]; //-V112
			snprintf(buf, _countof(buf), "%ld", val);
            return buf;
        }
        static string toString(unsigned long val)
        {
			char buf[24]; //-V112
			snprintf(buf, _countof(buf), "%lu", val);
            return buf;
        }
        static string toString(long long val)
        {
			char buf[24];
			snprintf(buf, _countof(buf), I64_FMT, val);
            return buf;
        }
        static string toString(unsigned long long val)
        {
			char buf[24];
			snprintf(buf, _countof(buf), U64_FMT, val);
            return buf;
        }
        static string toString(double val)
        {
			char buf[24];
			snprintf(buf, _countof(buf), "%0.2f", val);
            return buf;
        }
        
		static string toString(const string& p_sep, const StringList& p_lst);
		static string toString(const StringList& p_lst);
        
		static string toSupportsCommand(const StringList& p_feat)
		{
			const string l_result = "$Supports " + toString(" ", p_feat) + '|';
			return  l_result;
		}
		
        static wstring toStringW(int32_t val)
        {
            wchar_t buf[32];
            snwprintf(buf, _countof(buf), L"%ld", val);
            return buf;
        }
        
        static wstring toStringW(uint32_t val)
        {
            wchar_t buf[32];
			snwprintf(buf, _countof(buf), L"%u", val);
            return buf;
        }
        
        static wstring toStringW(int64_t val)
        {
			wchar_t buf[64];
            snwprintf(buf, _countof(buf), _T(I64_FMT), val);
            return buf;
        }
        
        static wstring toStringW(uint64_t val)
        {
			wchar_t buf[64];
			snwprintf(buf, _countof(buf), _T(U64_FMT), val);
            return buf;
        }
        
        static wstring toStringW(double val)
        {
			wchar_t buf[20];
            snwprintf(buf, _countof(buf), L"%0.2f", val);
            return buf;
        }
        
        static string toHexEscape(char val)
        {
            char buf[sizeof(int) * 2 + 1 + 1];
			snprintf(buf, _countof(buf), "%%%X", val & 0x0FF);
            return buf;
        }
        static char fromHexEscape(const string& aString)
        {
            unsigned int res = 0;
			if (sscanf(aString.c_str(), "%X", &res) == EOF)
			{
				// TODO log error!
			}
            return static_cast<char>(res);
        }
        
#if 0
        template<typename T>
        static T& intersect(T& t1, const T& t2)
        {
			for (auto i = t1.cbegin(); i != t1.cend();)
            {
                if (find_if(t2.begin(), t2.end(), bind1st(equal_to<typename T::value_type>(), *i)) == t2.end())
                    i = t1.erase(i);
                else
                    ++i;
            }
            return t1;
        }
#endif
        static string encodeURI(const string& /*aString*/, bool reverse = false);
        static string getLocalIp();
		static bool isPrivateIp(const string& p_ip);
		static bool isPrivateIp(uint32_t p_ip)
		{
			return ((p_ip & 0xff000000) == 0x0a000000 || // 10.0.0.0/8
			        (p_ip & 0xff000000) == 0x7f000000 || // 127.0.0.0/8
			        (p_ip & 0xffff0000) == 0xa9fe0000 || // 169.254.0.0/16
			        (p_ip & 0xfff00000) == 0xac100000 || // 172.16.0.0/12
			        (p_ip & 0xffff0000) == 0xc0a80000);  // 192.168.0.0/16
		}
        /**
         * Case insensitive substring search.
         * @return First position found or string::npos
         */
        static string::size_type findSubString(const string& aString, const string& aSubString, string::size_type start = 0) noexcept;
        static wstring::size_type findSubString(const wstring& aString, const wstring& aSubString, wstring::size_type start = 0) noexcept;
        
        
        static const string getIpCountry(const string& IP);
        
        static bool getAway()
        {
            return away;
        }
        static void setAway(bool aAway);
        static string getAwayMessage(StringMap& params);
        static void setAwayMessage(const string& aMsg)
        {
            awayMsg = aMsg;
        }
        
        static uint64_t getDirSize(const string &sFullPath);
        static bool validatePath(const string &sPath);
        static bool fileExists(const string &aFile);
        
        static uint32_t rand();
        static uint32_t rand(uint32_t high)
        {
            return rand() % high;
        }
        static uint32_t rand(uint32_t low, uint32_t high)
        {
            return rand(high - low) + low;
        }
        static double randd()
        {
            return ((double)rand()) / ((double)0xffffffff);
        }
        
		static string getRegistryCommaSubkey(const tstring& p_key);
		static string getRegistryValueString(const tstring& p_key, bool p_is_path = false);
		static bool setRegistryValueString(const tstring& p_key, const tstring& p_value);
		static bool deleteRegistryValue(const tstring& p_value);
        
		static string getExternalIP(const string& p_url, LONG p_timeOut = 500);

		static size_t getDataFromInet(LPCWSTR agent, const DWORD frameBufferSize, const string& url, string& data, LONG timeOut = 0, IDateReceiveReporter* reporter = NULL);
		static uint64_t getBinaryDataFromInet(LPCWSTR agent, const DWORD frameBufferSize, const string& url, std::vector<byte>& p_dataOut, LONG timeOut = 0, IDateReceiveReporter* reporter = NULL);

		
    private:
        /** In local mode, all config and temp files are kept in the same dir as the executable */
        static bool localMode;
        
        static string paths[PATH_LAST];
        
        static bool away;
        static string awayMsg;
        static time_t awayTime;
        static time_t startTime;
        
        typedef map<uint32_t, uint16_t> CountryList;
        typedef CountryList::iterator CountryIter;
        static CountryList countries;
        static CriticalSection g_cs;
		static NUMBERFMT g_nf;
        static StringList countryNames;
        
        static long mUptimeSeconds;
        struct CustomNetwork
        {
            uint32_t startip, endip;
            string m_description;
        };
        typedef vector<CustomNetwork> LocationsList;
        static LocationsList userLocations;
        static void loadBootConfig();
        static unordered_set<string> g_compress_ext; //[+]FlylinkDC++
    public:
        static bool is_compress_ext(const string& p_ext);
		static DWORD GetTextResource(const int p_res, LPCSTR& p_data);
		static void WriteTextResourceToFile(const int p_res, const tstring& p_file);
};

/** Case insensitive hash function for strings */
struct noCaseStringHash
{
    size_t operator()(const string* s) const
    {
        return operator()(*s);
    }
    
    size_t operator()(const string& s) const
    {
        size_t x = 0;
        const char* end = s.data() + s.size();
        for (const char* str = s.data(); str < end;)
        {
            wchar_t c = 0;
            int n = Text::utf8ToWc(str, c);
            if (n < 0)
            {
                x = x * 32 - x + '_';
                str += abs(n);
            }
            else
            {
                x = x * 32 - x + (size_t)Text::toLower(c);
                str += n;
            }
        }
        return x;
    }
    
    size_t operator()(const wstring* s) const
    {
        return operator()(*s);
    }
    size_t operator()(const wstring& s) const
    {
        size_t x = 0;
        const wchar_t* y = s.data();
        wstring::size_type j = s.size();
        for (wstring::size_type i = 0; i < j; ++i)
        {
            x = x * 31 + (size_t)Text::toLower(y[i]);
        }
        return x;
    }
    
    bool operator()(const string* a, const string* b) const
    {
        return stricmp(*a, *b) < 0;
    }
    bool operator()(const string& a, const string& b) const
    {
        return stricmp(a, b) < 0;
    }
    bool operator()(const wstring* a, const wstring* b) const
    {
        return stricmp(*a, *b) < 0;
    }
    bool operator()(const wstring& a, const wstring& b) const
    {
        return stricmp(a, b) < 0;
    }
};

/** Case insensitive string comparison */
struct noCaseStringEq
{
    bool operator()(const string* a, const string* b) const
    {
        return a == b || stricmp(*a, *b) == 0;
    }
    bool operator()(const string& a, const string& b) const
    {
        return stricmp(a, b) == 0;
    }
    bool operator()(const wstring* a, const wstring* b) const
    {
        return a == b || stricmp(*a, *b) == 0;
    }
    bool operator()(const wstring& a, const wstring& b) const
    {
        return stricmp(a, b) == 0;
    }
};

inline bool __fastcall EqualD(double A, double B)
{
    return fabs(A - B) <= 1e-6;
}

/** Case insensitive string ordering */
struct noCaseStringLess
{
    bool operator()(const string* a, const string* b) const
    {
        return stricmp(*a, *b) < 0;
    }
    bool operator()(const string& a, const string& b) const
    {
        return stricmp(a, b) < 0;
    }
    bool operator()(const wstring* a, const wstring* b) const
    {
        return stricmp(*a, *b) < 0;
    }
    bool operator()(const wstring& a, const wstring& b) const
    {
        return stricmp(a, b) < 0;
    }
};

} // namespace dcpp

#endif // !defined(UTIL_H)

/**
 * @file
 * $Id: Util.h 575 2011-08-25 19:38:04Z bigmuscle $
 */
