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

#include "stdinc.h"
#include "Util.h"
#include "version.h"

#ifdef _WIN32

#include "w.h"
#include "shlobj.h"

#endif

#include "CID.h"
#include "File.h"
#include "SettingsManager.h"
#include "ResourceManager.h"
#include "StringTokenizer.h"
#include "SettingsManager.h"
#include "SimpleXML.h"
#include "version.h"
#include "OnlineUser.h"
#include "Socket.h"

#include "User.h"
#include <fstream>
#include "wininet.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <ctype.h>
#endif
#include <locale.h>

#include <boost/algorithm/string.hpp>
#include "../client/LogManager.h"
#include "../windows/resource.h" // TDOD - плохо что тут инклудитс€ винда?

namespace dcpp
{

const string g_tth = "TTH:"; // [+] IRainman opt.

CriticalSection Util::g_cs;
time_t Util::startTime = time(NULL);
string Util::emptyString;
wstring Util::emptyStringW;
tstring Util::emptyStringT;

bool Util::away = false;
string Util::awayMsg;
time_t Util::awayTime;

Util::CountryList Util::countries;
Util::LocationsList Util::userLocations;
unordered_set<string> Util::g_compress_ext;

StringList Util::countryNames;
NUMBERFMT Util::g_nf = { 0 };

string Util::paths[Util::PATH_LAST];

bool Util::localMode = true;

static void sgenrand(unsigned long seed);

extern "C" void bz_internal_error(int errcode)
{
    dcdebug("bzip2 internal error: %d\n", errcode);
}

#if (_MSC_VER >= 1400)
void WINAPI invalidParameterHandler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
    //do nothing, this exist because vs2k5 crt needs it not to crash on errors.
}
#endif

#ifdef _WIN32

typedef HRESULT(WINAPI* _SHGetKnownFolderPath)(GUID& rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);

static string getDownloadsPath(const string& def)
{
    // Try Vista downloads path
    static _SHGetKnownFolderPath getKnownFolderPath = 0;
    static HINSTANCE shell32 = NULL;
    
    if (!shell32)
    {
        shell32 = ::LoadLibrary(_T("Shell32.dll"));
        if (shell32)
        {
            getKnownFolderPath = (_SHGetKnownFolderPath)::GetProcAddress(shell32, "SHGetKnownFolderPath");
            
            if (getKnownFolderPath)
            {
                PWSTR path = NULL;
                // Defined in KnownFolders.h.
                static GUID downloads = {0x374de290, 0x123f, 0x4565, {0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b}};
                if (getKnownFolderPath(downloads, 0, NULL, &path) == S_OK)
                {
                    string ret = Text::fromT(path) + "\\";
                    ::CoTaskMemFree(path);
                    return ret;
                }
            }
        }
    }
    
    return def + "Downloads\\";
}

#endif

void Util::initialize()
{
    Text::initialize();
    
    sgenrand((unsigned long)time(NULL));
    
#if (_MSC_VER >= 1400)
    _set_invalid_parameter_handler(reinterpret_cast<_invalid_parameter_handler>(invalidParameterHandler));
#endif
    
#ifdef _WIN32
	static TCHAR g_sep[2] = _T(",");
	static wchar_t g_Dummy[16] = { 0 };
	g_nf.lpDecimalSep = g_sep;
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, g_Dummy, 16);
	g_nf.Grouping = _wtoi(g_Dummy);
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, g_Dummy, 16);
	g_nf.lpThousandSep = g_Dummy;

    TCHAR buf[MAX_PATH + 1] = { 0 };
    ::GetModuleFileName(NULL, buf, MAX_PATH);
    
    string exePath = Util::getFilePath(Text::fromT(buf));
    
    // Global config path is StrongDC++ executable path...
    paths[PATH_GLOBAL_CONFIG] = exePath;
    
    paths[PATH_USER_CONFIG] = paths[PATH_GLOBAL_CONFIG] + "Settings\\";
    
    loadBootConfig();
    
    if (!File::isAbsolute(paths[PATH_USER_CONFIG]))
    {
        paths[PATH_USER_CONFIG] = paths[PATH_GLOBAL_CONFIG] + paths[PATH_USER_CONFIG];
    }
    
    paths[PATH_USER_CONFIG] = validateFileName(paths[PATH_USER_CONFIG]);
    
    if (localMode)
    {
        paths[PATH_USER_LOCAL] = paths[PATH_USER_CONFIG];
    }
    else
    {
        if (::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, buf) == S_OK)
        {
            paths[PATH_USER_CONFIG] = Text::fromT(buf) + "\\StrongDC++\\";
        }
        
        paths[PATH_USER_LOCAL] = ::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, buf) == S_OK ? Text::fromT(buf) + "\\StrongDC++\\" : paths[PATH_USER_CONFIG];
    }
    
    paths[PATH_DOWNLOADS] = getDownloadsPath(paths[PATH_USER_CONFIG]);
    paths[PATH_RESOURCES] = exePath;
    paths[PATH_LOCALE] = exePath;
    
#else
    paths[PATH_GLOBAL_CONFIG] = "/etc/";
    const char* home_ = getenv("HOME");
    string home = home_ ? Text::toUtf8(home_) : "/tmp/";
    
    paths[PATH_USER_CONFIG] = home + "/.strongdc++/";
    
    loadBootConfig();
    
    if (!File::isAbsolute(paths[PATH_USER_CONFIG]))
    {
        paths[PATH_USER_CONFIG] = paths[PATH_GLOBAL_CONFIG] + paths[PATH_USER_CONFIG];
    }
    
    paths[PATH_USER_CONFIG] = validateFileName(paths[PATH_USER_CONFIG]);
    
    if (localMode)
    {
        // @todo implement...
    }
    
    paths[PATH_USER_LOCAL] = paths[PATH_USER_CONFIG];
    paths[PATH_RESOURCES] = "/usr/share/";
    paths[PATH_LOCALE] = paths[PATH_RESOURCES] + "locale/";
    paths[PATH_DOWNLOADS] = home + "/Downloads/";
#endif
    
    paths[PATH_FILE_LISTS] = paths[PATH_USER_LOCAL] + "FileLists" PATH_SEPARATOR_STR;
    paths[PATH_HUB_LISTS] = paths[PATH_USER_LOCAL] + "HubLists" PATH_SEPARATOR_STR;
    paths[PATH_NOTEPAD] = paths[PATH_USER_CONFIG] + "Notepad.txt";
    paths[PATH_EMOPACKS] = paths[PATH_RESOURCES] + "EmoPacks" PATH_SEPARATOR_STR;
    
    File::ensureDirectory(paths[PATH_USER_CONFIG]);
    File::ensureDirectory(paths[PATH_USER_LOCAL]);
    
    try
    {
        // This product includes GeoIP data created by MaxMind, available from http://maxmind.com/
        // Updates at http://www.maxmind.com/app/geoip_country
        string file = getPath(PATH_USER_CONFIG) + "GeoIpCountryWhois.csv";
        string data = File(file, File::READ, File::OPEN).read();
        
        const char* start = data.c_str();
        string::size_type linestart = 0;
        string::size_type comma1 = 0;
        string::size_type comma2 = 0;
        string::size_type comma3 = 0;
        string::size_type comma4 = 0;
        string::size_type lineend = 0;
        CountryIter last = countries.end();
        uint32_t startIP = 0;
        uint32_t endIP = 0, endIPprev = 0;
        
        for (;;)
        {
            comma1 = data.find(',', linestart);
            if (comma1 == string::npos) break;
            comma2 = data.find(',', comma1 + 1);
            if (comma2 == string::npos) break;
            comma3 = data.find(',', comma2 + 1);
            if (comma3 == string::npos) break;
            comma4 = data.find(',', comma3 + 1);
            if (comma4 == string::npos) break;
            lineend = data.find('\n', comma4);
            if (lineend == string::npos) break;
            
            startIP = Util::toUInt32(start + comma2 + 2);
            endIP = Util::toUInt32(start + comma3 + 2);
            uint16_t* country = (uint16_t*)(start + comma4 + 2);
            if ((startIP - 1) != endIPprev)
                last = countries.insert(last, make_pair((startIP - 1), (uint16_t)16191));
            last = countries.insert(last, make_pair(endIP, *country));
            
            endIPprev = endIP;
            linestart = lineend + 1;
        }
    }
    catch (const FileException&)
    {
    }
    File::ensureDirectory(Util::getPath(Util::PATH_USER_CONFIG));
}
#ifndef _CONSOLE
bool Util::is_compress_ext(const string& p_ext)
{
    Lock l(g_cs);
    return g_compress_ext.count(p_ext) > 0;
}

void Util::load_compress_ext()
{
    Lock l(g_cs);
    const tstring l_filename = Text::toT(Util::getPath(Util::PATH_USER_CONFIG)) + _T("CustomCompressExt.ini");
    std::ifstream l_file_in(l_filename.c_str());
    string l_CurLine;
    bool l_end_file;
    if (l_file_in)
    {
        do
        {
            l_end_file = getline(l_file_in, l_CurLine).eof();
            if (!l_CurLine.empty() && l_CurLine[0] == '.')
            {
                string l_ext;
                for (size_t i = 0; i < l_CurLine.size(); ++i)
                {
                    if (l_CurLine[i] == ' ' || l_CurLine[i] == 0x09 || l_CurLine[i] == 0x0D || l_CurLine[i] == 0x0A)
                        break;
                    l_ext += l_CurLine[i];
                }
                if (!l_ext.empty())
                {
                    std::pair< unordered_set<string>::const_iterator, bool > l_it = g_compress_ext.insert(Text::toLower(l_ext));
                    if (!l_it.second)
                        LogManager::getInstance()->message("CustomCompressExt.ini - дубликатное расширение:" + l_ext);
                }
            }
        }
        while (!l_end_file);
    }
    else
    {
        if (HRSRC hResInfo = FindResource(NULL, MAKEINTRESOURCE(IDR_CUSTOM_COMPRESS_EXT), RT_RCDATA))
            if (HGLOBAL hResGlobal = LoadResource(NULL, hResInfo))
                if (LPCSTR l_data = (LPCSTR)LockResource(hResGlobal))
                    if (const int l_size = SizeofResource(NULL, hResInfo))
                    {
                        std::ofstream l_file_out(l_filename.c_str());
                        l_file_out.write(l_data, l_size);
                    }
    }
}

void Util::load_customlocations()
{
    Lock l(g_cs);
    // !SMT!-IP loading custom networks
    const tstring l_filename = Text::toT(Util::getPath(Util::PATH_USER_CONFIG)) + _T("CustomLocations.ini");
    std::ifstream l_file_in(l_filename.c_str());
    string l_CurLine;
    bool l_end_file;
    if (l_file_in)
        do
        {
            const char* l_LogLine = "CustomLocations.ini: [";
            l_end_file = getline(l_file_in, l_CurLine).eof();
            if (!l_CurLine.empty() && isdigit((unsigned char)l_CurLine[0]))
                if (l_CurLine.find('-') != string::npos && count(l_CurLine.begin(), l_CurLine.end(), '.') >= 6)
                {
                    unsigned a = 0, b = 0, c = 0, d = 0, a2 = 0, b2 = 0, c2 = 0, d2 = 0;
                    const int l_Items = sscanf_s(l_CurLine.c_str(), "%d.%d.%d.%d-%d.%d.%d.%d", &a, &b, &c, &d, &a2, &b2, &c2, &d2);
                    if (l_Items == 8)
                    {
                        const string::size_type l_space = l_CurLine.find(' ');
                        if (l_space != string::npos)
                        {
                            CustomNetwork net;
                            net.startip = (a << 24) + (b << 16) + (c << 8) + d;
                            net.endip = (a2 << 24) + (b2 << 16) + (c2 << 8) + d2 + 1;
                            net.m_description = l_CurLine.substr(l_space + 1);
                            boost::trim_left(net.m_description);
                            userLocations.push_back(net);
                        }
                        else
                            LogManager::getInstance()->message(l_LogLine + l_CurLine + "] Ќе найден пробел");
                    }
                    else
                        LogManager::getInstance()->message(l_LogLine + l_CurLine + "] Ќе найдена маска %d.%d.%d.%d-%d.%d.%d.%d");
                }
                else if (l_CurLine.find('+') != string::npos && count(l_CurLine.begin(), l_CurLine.end(), '.') >= 3)
                {
                    unsigned a = 0, b = 0, c = 0, d = 0, n = 0;
                    const int l_Items = sscanf_s(l_CurLine.c_str(), "%d.%d.%d.%d+%d", &a, &b, &c, &d, &n);
                    if (l_Items == 5)
                    {
                        const string::size_type l_space = l_CurLine.find(' ');
                        if (l_space != string::npos)
                        {
                            CustomNetwork net;
                            net.startip = (a << 24) + (b << 16) + (c << 8) + d;
                            net.endip = net.startip + n;
                            net.m_description = l_CurLine.substr(l_space + 1);
                            boost::trim_left(net.m_description);
                            userLocations.push_back(net);
                        }
                        else
                            LogManager::getInstance()->message(l_LogLine + l_CurLine + "] Ќе найден пробел");
                    }
                    else
                        LogManager::getInstance()->message(l_LogLine + l_CurLine + "] Ќе найдена маска %d.%d.%d.%d+%d");
                }
        }
        while (!l_end_file);
        
}
#endif

void Util::migrate(const string& file)
{
    if (localMode)
    {
        return;
    }
    
    if (File::getSize(file) != -1)
    {
        return;
    }
    
    string fname = getFileName(file);
    string old = paths[PATH_GLOBAL_CONFIG] + "Settings\\" + fname;
    if (File::getSize(old) == -1)
    {
        return;
    }
    
    File::renameFile(old, file);
}

void Util::loadBootConfig()
{
    // Load boot settings
    try
    {
        SimpleXML boot;
        boot.fromXML(File(getPath(PATH_GLOBAL_CONFIG) + "dcppboot.xml", File::READ, File::OPEN).read());
        boot.stepIn();
        
        if (boot.findChild("LocalMode"))
        {
            localMode = boot.getChildData() != "0";
        }
        
        boot.resetCurrentChild();
        
        if (boot.findChild("ConfigPath"))
        {
            StringMap params;
#ifdef _WIN32
            // @todo load environment variables instead? would make it more useful on *nix
            TCHAR path[MAX_PATH];
            
            params["APPDATA"] = Text::fromT((::SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path), path));
            params["PERSONAL"] = Text::fromT((::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path), path));
#endif
            paths[PATH_USER_CONFIG] = Util::formatParams(boot.getChildData(), params, false);
        }
    }
    catch (const Exception&)
    {
        // Unable to load boot settings...
    }
}

#ifdef _WIN32
static const char badChars[] =
{
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, '<', '>', '/', '"', '|', '?', '*', 0
};
#else

static const char badChars[] =
{
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, '<', '>', '\\', '"', '|', '?', '*', 0
};
#endif

/**
 * Replaces all strange characters in a file with '_'
 * @todo Check for invalid names such as nul and aux...
 */
string Util::validateFileName(string tmp)
{
    string::size_type i = 0;
    
    // First, eliminate forbidden chars
    while ((i = tmp.find_first_of(badChars, i)) != string::npos)
    {
        tmp[i] = '_';
        i++;
    }
    
    // Then, eliminate all ':' that are not the second letter ("c:\...")
    i = 0;
    while ((i = tmp.find(':', i)) != string::npos)
    {
        if (i == 1)
        {
            i++;
            continue;
        }
        tmp[i] = '_';
        i++;
    }
    
    // Remove the .\ that doesn't serve any purpose
    i = 0;
    while ((i = tmp.find("\\.\\", i)) != string::npos)
    {
        tmp.erase(i + 1, 2);
    }
    i = 0;
    while ((i = tmp.find("/./", i)) != string::npos)
    {
        tmp.erase(i + 1, 2);
    }
    
    // Remove any double \\ that are not at the beginning of the path...
    i = 1;
    while ((i = tmp.find("\\\\", i)) != string::npos)
    {
        tmp.erase(i + 1, 1);
    }
    i = 1;
    while ((i = tmp.find("//", i)) != string::npos)
    {
        tmp.erase(i + 1, 1);
    }
    
    // And last, but not least, the infamous ..\! ...
    i = 0;
    while (((i = tmp.find("\\..\\", i)) != string::npos))
    {
        tmp[i + 1] = '_';
        tmp[i + 2] = '_';
        tmp[i + 3] = '_';
        i += 2;
    }
    i = 0;
    while (((i = tmp.find("/../", i)) != string::npos))
    {
        tmp[i + 1] = '_';
        tmp[i + 2] = '_';
        tmp[i + 3] = '_';
        i += 2;
    }
    
    // Dots at the end of path names aren't popular
    i = 0;
    while (((i = tmp.find(".\\", i)) != string::npos))
    {
        if (i != 0)
            tmp[i] = '_';
        i += 1;
    }
    i = 0;
    while (((i = tmp.find("./", i)) != string::npos))
    {
        if (i != 0)
            tmp[i] = '_';
        i += 1;
    }
    
    
    return tmp;
}

string Util::cleanPathChars(string aNick)
{
    string::size_type i = 0;
    
    while ((i = aNick.find_first_of("/.\\", i)) != string::npos)
    {
        aNick[i] = '_';
    }
    return aNick;
}

string Util::getShortTimeString(time_t t)
{
    char buf[255];
    tm* _tm = localtime(&t);
    if (_tm == NULL)
    {
        strcpy(buf, "xx:xx");
    }
    else
    {
        strftime(buf, 254, SETTING(TIME_STAMPS_FORMAT).c_str(), _tm);
    }
    return Text::toUtf8(buf);
}

/**
 * Decodes a URL the best it can...
 * Default ports:
 * http:// -> port 80
 * https:// -> port 443
 * dchub:// -> port 411
 */
void Util::decodeUrl(const string& url, string& protocol, string& host, uint16_t& port, string& path, bool& isSecure, string& query, string& fragment)
{
    auto fragmentEnd = url.size();
    auto fragmentStart = url.rfind('#');
    
    size_t queryEnd;
    if (fragmentStart == string::npos)
    {
        queryEnd = fragmentStart = fragmentEnd;
    }
    else
    {
        dcdebug("f");
        queryEnd = fragmentStart;
        fragmentStart++;
    }
    
    auto queryStart = url.rfind('?', queryEnd);
    size_t fileEnd;
    
    if (queryStart == string::npos)
    {
        fileEnd = queryStart = queryEnd;
    }
    else
    {
        dcdebug("q");
        fileEnd = queryStart;
        queryStart++;
    }
    
    auto protoStart = 0;
    auto protoEnd = url.find("://", protoStart);
    
    auto authorityStart = protoEnd == string::npos ? protoStart : protoEnd + 3;
    auto authorityEnd = url.find_first_of("/#?", authorityStart);
    
    size_t fileStart;
    if (authorityEnd == string::npos)
    {
        authorityEnd = fileStart = fileEnd;
    }
    else
    {
        dcdebug("a");
        fileStart = authorityEnd;
    }
    
    protocol = (protoEnd == string::npos ? Util::emptyString : url.substr(protoStart, protoEnd - protoStart));
    
    if (authorityEnd > authorityStart)
    {
        dcdebug("x");
        size_t portStart = string::npos;
        if (url[authorityStart] == '[')
        {
            // IPv6?
            auto hostEnd = url.find(']');
            if (hostEnd == string::npos)
            {
                return;
            }
            
            host = url.substr(authorityStart + 1, hostEnd - authorityStart - 1);
            if (hostEnd + 1 < url.size() && url[hostEnd + 1] == ':')
            {
                portStart = hostEnd + 2;
            }
        }
        else
        {
            size_t hostEnd;
            portStart = url.find(':', authorityStart);
            if (portStart != string::npos && portStart > authorityEnd)
            {
                portStart = string::npos;
            }
            
            if (portStart == string::npos)
            {
                hostEnd = authorityEnd;
            }
            else
            {
                hostEnd = portStart;
                portStart++;
            }
            
            dcdebug("h");
            host = url.substr(authorityStart, hostEnd - authorityStart);
        }
        
        if (portStart == string::npos)
        {
            if (protocol == "http")
            {
                port = 80;
            }
            else if (protocol == "https")
            {
                port = 443;
                isSecure = true;
            }
            else if (protocol == "dchub"  || protocol.empty())
            {
                port = 411;
            }
        }
        else
        {
            dcdebug("p");
            port = static_cast<uint16_t>(Util::toInt(url.substr(portStart, authorityEnd - portStart)));
        }
    }
    
    dcdebug("\n");
    path = url.substr(fileStart, fileEnd - fileStart);
    query = url.substr(queryStart, queryEnd - queryStart);
    fragment = url.substr(fragmentStart, fragmentStart);
}

map<string, string> Util::decodeQuery(const string& query)
{
    map<string, string> ret;
    size_t start = 0;
    while (start < query.size())
    {
        auto eq = query.find('=', start);
        if (eq == string::npos)
        {
            break;
        }
        
        auto param = eq + 1;
        auto end = query.find('&', param);
        
        if (end == string::npos)
        {
            end = query.size();
        }
        
        if (eq > start && end > param)
        {
            ret[query.substr(start, eq - start)] = query.substr(param, end - param);
        }
        
        start = end + 1;
    }
    
    return ret;
}

void Util::setAway(bool aAway)
{
    away = aAway;
    
    SettingsManager::getInstance()->set(SettingsManager::AWAY, aAway);
    
    if (away)
        awayTime = time(NULL);
}

string Util::getAwayMessage(StringMap& params)
{
    params["idleTI"] = Text::fromT(formatSeconds(time(NULL) - awayTime));
    return formatParams(awayMsg.empty() ? SETTING(DEFAULT_AWAY_MESSAGE) : awayMsg, params, false, awayTime);
}

string Util::formatBytes(int64_t aBytes) // TODO fix copy-paste
{
    char buf[64];
	buf[0] = 0;
    if (aBytes < 1024)
    {
		snprintf(buf, _countof(buf), "%d %s", (int)aBytes & 0xffffffff, CSTRING(B));
    }
    else if (aBytes < 1048576)
    {
        snprintf(buf, _countof(buf), "%.02f %s", (double)aBytes / (1024.0), CSTRING(KB));
    }
    else if (aBytes < 1073741824)
    {
        snprintf(buf, _countof(buf), "%.02f %s", (double)aBytes / (1048576.0), CSTRING(MB));
    }
    else if (aBytes < (int64_t)1099511627776)
    {
        snprintf(buf, _countof(buf), "%.02f %s", (double)aBytes / (1073741824.0), CSTRING(GB));
    }
    else if (aBytes < (int64_t)1125899906842624)
    {
		snprintf(buf, _countof(buf), "%.03f %s", (double)aBytes / (1099511627776.0), CSTRING(TB));
    }
    else if (aBytes < (int64_t)1152921504606846976)
    {
		snprintf(buf, _countof(buf), "%.03f %s", (double)aBytes / (1125899906842624.0), CSTRING(PB));
    }
    else
    {
		snprintf(buf, _countof(buf), "%.03f %s", (double)aBytes / (1152921504606846976.0), CSTRING(EB));
    }
    return buf;
}
string Util::formatBytes(double aBytes) // TODO fix copy-paste
{
	char buf[64];
	buf[0] = 0;
	if (aBytes < 1024)
	{
		snprintf(buf, _countof(buf), "%d %s", (int)aBytes & 0xffffffff, CSTRING(B));
	}
	else if (aBytes < 1048576)
	{
		snprintf(buf, _countof(buf), "%.02f %s", aBytes / (1024.0), CSTRING(KB));
	}
	else if (aBytes < 1073741824)
	{
		snprintf(buf, _countof(buf), "%.02f %s", aBytes / (1048576.0), CSTRING(MB));
	}
	else if (aBytes < (int64_t)1099511627776)
	{
		snprintf(buf, _countof(buf), "%.02f %s", aBytes / (1073741824.0), CSTRING(GB));
	}
	else if (aBytes < (int64_t)1125899906842624)
	{
		snprintf(buf, _countof(buf), "%.03f %s", aBytes / (1099511627776.0), CSTRING(TB));
	}
	else if (aBytes < (int64_t)1152921504606846976)
	{
		snprintf(buf, _countof(buf), "%.03f %s", aBytes / (1125899906842624.0), CSTRING(PB));
	}
	else
	{
		snprintf(buf, _countof(buf), "%.03f %s", aBytes / (1152921504606846976.0), CSTRING(EB));
	}
	return buf;
}

wstring Util::formatBytesW(int64_t aBytes)
{
    wchar_t buf[64];
    if (aBytes < 1024)
    {
        snwprintf(buf, _countof(buf), L"%d %s", (int)(aBytes & 0xffffffff), CWSTRING(B));
    }
    else if (aBytes < 1048576)
    {
        snwprintf(buf, _countof(buf), L"%.02f %s", (double)aBytes / (1024.0), CWSTRING(KB));
    }
    else if (aBytes < 1073741824)
    {
        snwprintf(buf, _countof(buf), L"%.02f %s", (double)aBytes / (1048576.0), CWSTRING(MB));
    }
    else if (aBytes < (int64_t)1099511627776)
    {
        snwprintf(buf, _countof(buf), L"%.02f %s", (double)aBytes / (1073741824.0), CWSTRING(GB));
    }
    else if (aBytes < (int64_t)1125899906842624)
    {
		snwprintf(buf, _countof(buf), L"%.03f %s", (double)aBytes / (1099511627776.0), CWSTRING(TB));
    }
    else if (aBytes < (int64_t)1152921504606846976)
    {
		snwprintf(buf, _countof(buf), L"%.03f %s", (double)aBytes / (1125899906842624.0), CWSTRING(PB));
    }
    else
    {
		snwprintf(buf, _countof(buf), L"%.03f %s", (double)aBytes / (1152921504606846976.0), CWSTRING(EB)); //TODO Crash beta-16
    }
    
    return buf;
}

wstring Util::formatExactSize(int64_t aBytes)
{
#ifdef _WIN32
	wchar_t l_number[64];
	l_number[0] = 0;
	snwprintf(l_number, _countof(l_number), _T(I64_FMT), aBytes);
	wchar_t l_buf_nf[64];
	l_buf_nf[0] = 0;
	GetNumberFormat(LOCALE_USER_DEFAULT, 0, l_number, &g_nf, l_buf_nf, _countof(l_buf_nf));
	snwprintf(l_buf_nf, _countof(l_buf_nf), _T("%s %s"), l_buf_nf, CWSTRING(B));
	return l_buf_nf;
#else
    wchar_t buf[64];
	snwprintf(buf, _countof(buf), _T(I64_FMT), (long long int)aBytes);
    return tstring(buf) + TSTRING(B);
#endif
}

string Util::getLocalIp()
{
#ifndef _CONSOLE //[+] PPA for MakeDefs
    if (!SettingsManager::getInstance()->isDefault(SettingsManager::BIND_INTERFACE))
    {
        return Socket::getBindAddress();
    }
#endif
    string tmp;
    char buf[256];
	if (!gethostname(buf, 255))
	{
		const hostent* he = gethostbyname(buf);
		if (he == nullptr || he->h_addr_list[0] == 0)
        return Util::emptyString;
    sockaddr_in dest;
    int i = 0;
    
    // We take the first ip as default, but if we can find a better one, use it instead...
    memcpy(&(dest.sin_addr), he->h_addr_list[i++], he->h_length);
    tmp = inet_ntoa(dest.sin_addr);
    if (Util::isPrivateIp(tmp) || strncmp(tmp.c_str(), "169", 3) == 0)
    {
        while (he->h_addr_list[i])
        {
            memcpy(&(dest.sin_addr), he->h_addr_list[i], he->h_length);
            string tmp2 = inet_ntoa(dest.sin_addr);
            if (!Util::isPrivateIp(tmp2) && strncmp(tmp2.c_str(), "169", 3) != 0)
            {
                tmp = tmp2;
            }
            i++;
        }
    }
	}
    return tmp;
}

bool Util::isPrivateIp(const string& ip)
{
    struct in_addr addr;
    addr.s_addr = inet_addr(ip.c_str());
    if (addr.s_addr  != INADDR_NONE)
    {
		const uint32_t haddr = ntohl(addr.s_addr);
		return isPrivateIp(haddr);
    }
    return false;
}

typedef const uint8_t* ccp;
static wchar_t utf8ToLC(ccp& str)
{
    wchar_t c = 0;
    if (str[0] & 0x80)
    {
        if (str[0] & 0x40)
        {
            if (str[0] & 0x20)
            {
                if (str[1] == 0 || str[2] == 0 ||
                        !((((unsigned char)str[1]) & ~0x3f) == 0x80) ||
                        !((((unsigned char)str[2]) & ~0x3f) == 0x80))
                {
                    str++;
                    return 0;
                }
                c = ((wchar_t)(unsigned char)str[0] & 0xf) << 12 |
                    ((wchar_t)(unsigned char)str[1] & 0x3f) << 6 |
                    ((wchar_t)(unsigned char)str[2] & 0x3f);
                str += 3;
            }
            else
            {
                if (str[1] == 0 ||
                        !((((unsigned char)str[1]) & ~0x3f) == 0x80))
                {
                    str++;
                    return 0;
                }
                c = ((wchar_t)(unsigned char)str[0] & 0x1f) << 6 |
                    ((wchar_t)(unsigned char)str[1] & 0x3f);
                str += 2;
            }
        }
        else
        {
            str++;
            return 0;
        }
    }
    else
    {
        c = Text::asciiToLower((char)str[0]);
        str++;
        return c;
    }
    
    return Text::toLower(c);
}

string Util::toString(const string& sep, const StringList& lst)
{
    string ret;
    for (StringList::const_iterator i = lst.begin(), iend = lst.end(); i != iend; ++i)
    {
        ret += *i;
        if (i + 1 != iend)
            ret += sep;
    }
    return ret;
}

string Util::toString(const StringList& lst)
{
    if (lst.size() == 1)
        return lst[0];
    string tmp("[");
    for (StringList::const_iterator i = lst.begin(), iend = lst.end(); i != iend; ++i)
    {
        tmp += *i + ',';
    }
    if (tmp.length() == 1)
        tmp.push_back(']');
    else
        tmp[tmp.length() - 1] = ']';
    return tmp;
}

string::size_type Util::findSubString(const string& aString, const string& aSubString, string::size_type start) noexcept
{
    if (aString.length() < start)
        return (string::size_type)string::npos;
        
    if (aString.length() - start < aSubString.length())
        return (string::size_type)string::npos;
        
    if (aSubString.empty())
        return 0;
        
    // Hm, should start measure in characters or in bytes? bytes for now...
    const uint8_t* tx = (const uint8_t*)aString.c_str() + start;
    const uint8_t* px = (const uint8_t*)aSubString.c_str();
    
    const uint8_t* end = tx + aString.length() - start - aSubString.length() + 1;
    
    wchar_t wp = utf8ToLC(px);
    
    while (tx < end)
    {
        const uint8_t* otx = tx;
        if (wp == utf8ToLC(tx))
        {
            const uint8_t* px2 = px;
            const uint8_t* tx2 = tx;
            
            for (;;)
            {
                if (*px2 == 0)
                    return otx - (uint8_t*)aString.c_str();
                    
                if (utf8ToLC(px2) != utf8ToLC(tx2))
                    break;
            }
        }
    }
    return (string::size_type)string::npos;
}

wstring::size_type Util::findSubString(const wstring& aString, const wstring& aSubString, wstring::size_type pos) noexcept
{
    if (aString.length() < pos)
        return static_cast<wstring::size_type>(wstring::npos);
        
    if (aString.length() - pos < aSubString.length())
        return static_cast<wstring::size_type>(wstring::npos);
        
    if (aSubString.empty())
        return 0;
        
    wstring::size_type j = 0;
    wstring::size_type end = aString.length() - aSubString.length() + 1;
    
    for (; pos < end; ++pos)
    {
        if (Text::toLower(aString[pos]) == Text::toLower(aSubString[j]))
        {
            wstring::size_type tmp = pos + 1;
            bool found = true;
            for (++j; j < aSubString.length(); ++j, ++tmp)
            {
                if (Text::toLower(aString[tmp]) != Text::toLower(aSubString[j]))
                {
                    j = 0;
                    found = false;
                    break;
                }
            }
            
            if (found)
                return pos;
        }
    }
    return static_cast<wstring::size_type>(wstring::npos);
}

string Util::encodeURI(const string& aString, bool reverse)
{
    // reference: rfc2396
    string tmp = aString;
    if (reverse)
    {
        string::size_type idx;
        for (idx = 0; idx < tmp.length(); ++idx)
        {
            if (tmp.length() > idx + 2 && tmp[idx] == '%' && isxdigit(tmp[idx + 1]) && isxdigit(tmp[idx + 2]))
            {
                tmp[idx] = fromHexEscape(tmp.substr(idx + 1, 2));
                tmp.erase(idx + 1, 2);
            }
            else     // reference: rfc1630, magnet-uri draft
            {
                if (tmp[idx] == '+')
                    tmp[idx] = ' ';
            }
        }
    }
    else
    {
        static const string disallowed = ";/?:@&=+$," // reserved
                                         "<>#%\" "    // delimiters
                                         "{}|\\^[]`"; // unwise
        string::size_type idx;
        for (idx = 0; idx < tmp.length(); ++idx)
        {
            if (tmp[idx] == ' ')
            {
                tmp[idx] = '+';
            }
            else
            {
                if (tmp[idx] <= 0x1F || tmp[idx] >= 0x7f || (disallowed.find_first_of(tmp[idx])) != string::npos)
                {
                    tmp.replace(idx, 1, toHexEscape(tmp[idx]));
                    idx += 2;
                }
            }
        }
    }
    return tmp;
}

/**
 * This function takes a string and a set of parameters and transforms them according to
 * a simple formatting rule, similar to strftime. In the message, every parameter should be
 * represented by %[name]. It will then be replaced by the corresponding item in
 * the params stringmap. After that, the string is passed through strftime with the current
 * date/time and then finally written to the log file. If the parameter is not present at all,
 * it is removed from the string completely...
 */
string Util::formatParams(const string& msg, const StringMap& params, bool filter, const time_t t)
{
    string result = msg;
    
    string::size_type i, j, k;
    i = 0;
    while ((j = result.find("%[", i)) != string::npos)
    {
        if ((result.size() < j + 2) || ((k = result.find(']', j + 2)) == string::npos))
        {
            break;
        }
        string name = result.substr(j + 2, k - j - 2);
        StringMap::const_iterator smi = params.find(name);
        if (smi == params.end())
        {
            result.erase(j, k - j + 1);
            i = j;
        }
        else
        {
            if (smi->second.find_first_of("%\\./") != string::npos)
            {
                string tmp = smi->second;   // replace all % in params with %% for strftime
                string::size_type m = 0;
                while ((m = tmp.find('%', m)) != string::npos)
                {
                    tmp.replace(m, 1, "%%");
                    m += 2;
                }
                if (filter)
                {
                    // Filter chars that produce bad effects on file systems
                    m = 0;
                    while ((m = tmp.find_first_of("\\./", m)) != string::npos)
                    {
                        tmp[m] = '_';
                    }
                }
                
                result.replace(j, k - j + 1, tmp);
                i = j + tmp.size();
            }
            else
            {
                result.replace(j, k - j + 1, smi->second);
                i = j + smi->second.size();
            }
        }
    }
    
    result = formatTime(result, t);
    
    return result;
}

string Util::formatRegExp(const string& msg, const StringMap& params)
{
    string result = msg;
    string::size_type i, j, k;
    i = 0;
    while ((j = result.find("%[", i)) != string::npos)
    {
        if ((result.size() < j + 2) || ((k = result.find(']', j + 2)) == string::npos))
        {
            break;
        }
        string name = result.substr(j + 2, k - j - 2);
        StringMap::const_iterator smi = params.find(name);
        if (smi != params.end())
        {
            result.replace(j, k - j + 1, smi->second);
            i = j + smi->second.size();
        }
        else
        {
            i = k + 1;
        }
    }
    return result;
}

uint64_t Util::getDirSize(const string &sFullPath)
{
    uint64_t total = 0;
    
    WIN32_FIND_DATA fData;
    HANDLE hFind;
    
    hFind = FindFirstFile(Text::toT(sFullPath + "\\*").c_str(), &fData);
    
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            string name = Text::fromT(fData.cFileName);
            if (name == "." || name == "..")
                continue;
            if ((fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !BOOLSETTING(SHARE_HIDDEN))
                continue;
            if (fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                string newName = sFullPath + PATH_SEPARATOR + name;
                if (stricmp(newName + PATH_SEPARATOR, SETTING(TEMP_DOWNLOAD_DIRECTORY)) != 0)
                {
                    total += getDirSize(newName);
                }
            }
            else
            {
                total += (uint64_t)fData.nFileSizeLow | ((uint64_t)fData.nFileSizeHigh) << 32;
            }
        }
        while (FindNextFile(hFind, &fData));
        FindClose(hFind);
    }
    return total;
}

bool Util::validatePath(const string &sPath)
{
    if (sPath.empty())
        return false;
        
    if ((sPath.substr(1, 2) == ":\\") || (sPath.substr(0, 2) == "\\\\"))
    {
        if (GetFileAttributes(Text::toT(sPath).c_str()) & FILE_ATTRIBUTE_DIRECTORY)
            return true;
    }
    
    return false;
}

bool Util::fileExists(const string &aFile)
{
    DWORD attr = GetFileAttributes(Text::toT(aFile).c_str());
    return (attr != INVALID_FILE_ATTRIBUTES);
}

string Util::formatTime(const string &msg, const time_t t)
{
    if (!msg.empty())
    {
        tm* loc = localtime(&t);
        
        if (!loc)
        {
            return Util::emptyString;
        }
        
        size_t bufsize = msg.size() + 256;
        string buf(bufsize, 0);
        
        errno = 0;
        
        buf.resize(strftime(&buf[0], bufsize - 1, msg.c_str(), loc));
        
        while (buf.empty())
        {
            if (errno == EINVAL)
                return Util::emptyString;
                
            bufsize += 64;
            buf.resize(bufsize);
            buf.resize(strftime(&buf[0], bufsize - 1, msg.c_str(), loc));
        }
        
#ifdef _WIN32
        if (!Text::validateUtf8(buf))
#endif
        {
            buf = Text::toUtf8(buf);
        }
        return buf;
    }
    return Util::emptyString;
}

/* Below is a high-speed random number generator with much
   better granularity than the CRT one in msvc...(no, I didn't
   write it...see copyright) */
/* Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.
   Any feedback is very welcome. For any question, comments,
   see http://www.math.keio.ac.jp/matumoto/emt.html or email
   matumoto@math.keio.ac.jp */
/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned long mt[N]; /* the array for the state vector  */
static int mti = N + 1; /* mti==N+1 means mt[N] is not initialized */

/* initializing the array with a NONZERO seed */
static void sgenrand(unsigned long seed)
{
    /* setting initial seeds to mt[N] using         */
    /* the generator Line 25 of Table 1 in          */
    /* [KNUTH 1981, The Art of Computer Programming */
    /*    Vol. 2 (2nd Ed.), pp102]                  */
    mt[0] = seed & ULONG_MAX;
    for (mti = 1; mti < N; mti++)
        mt[mti] = (69069 * mt[mti - 1]) & ULONG_MAX;
}

uint32_t Util::rand()
{
    unsigned long y;
    static unsigned long mag01[2] = {0x0, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */
    
    if (mti >= N)   /* generate N words at one time */
    {
        int kk;
        
        if (mti == N + 1) /* if sgenrand() has not been called, */
            sgenrand(4357); /* a default initial seed is used   */
            
        for (kk = 0; kk < N - M; kk++)
        {
            y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
            mt[kk] = mt[kk + M] ^(y >> 1) ^ mag01[y & 0x1];
        }
        for (; kk < N - 1; kk++)
        {
            y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
            mt[kk] = mt[kk + (M - N)] ^(y >> 1) ^ mag01[y & 0x1];
        }
        y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
        mt[N - 1] = mt[M - 1] ^(y >> 1) ^ mag01[y & 0x1];
        
        mti = 0;
    }
    
    y = mt[mti++];
    y ^= TEMPERING_SHIFT_U(y);
    y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
    y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
    y ^= TEMPERING_SHIFT_L(y);
    
    return y;
}

/*  getIpCountry
    This function returns the country(Abbreviation) of an ip
    for exemple: it returns "PT", whitch standards for "Portugal"
    more info: http://www.maxmind.com/app/csv
*/
const string Util::getIpCountry(const string& IP)
{
    if (!IP.empty())
    {
        if (BOOLSETTING(GET_USER_COUNTRY))
        {
            Lock l(g_cs);
            typedef unordered_map<string, string> TCacheMap;
            static TCacheMap g_cache_locations;
            TCacheMap::const_iterator l_locations = g_cache_locations.find(IP);
            if (l_locations != g_cache_locations.end())
                return l_locations->second;
            dcassert(count(IP.begin(), IP.end(), '.') == 3);
            if (count(IP.begin(), IP.end(), '.') != 3)
                return emptyString;
            unsigned u1, u2, u3, u4;
            const int iItems = sscanf_s(IP.c_str(), "%u.%u.%u.%u", &u1, &u2, &u3, &u4);
            if (iItems == 4)
            {
                uint32_t ipnum = (u1 << 24) + (u2 << 16) + (u3 << 8) + u4;
                for (LocationsList::const_iterator j = userLocations.begin(); j != userLocations.end(); ++j)
                    if (j->startip <= ipnum && ipnum < j->endip)
                    {
#ifdef _DEBUG
                        dcdebug(string("userLocations - " + IP + j->m_description + "\n").c_str());
#endif
                        g_cache_locations[IP] = j->m_description;
                        return j->m_description;
                    }
                    
                auto i = countries.lower_bound(ipnum);
                if (i != countries.end())
                {
                    return string((char*) & (i->second), 2);
                    //return countryNames[i->second];
                }
            }
        }
    }
    return emptyString;
}

string Util::getTimeString()
{
    char buf[64];
    time_t _tt;
    time(&_tt);
    tm* _tm = localtime(&_tt);
    if (_tm == NULL)
    {
        strcpy(buf, "xx:xx:xx");
    }
    else
    {
        strftime(buf, 64, "%X", _tm);
    }
    return buf;
}

string Util::toAdcFile(const string& file)
{
    if (file == "files.xml.bz2" || file == "files.xml")
        return file;
        
    string ret;
    ret.reserve(file.length() + 1);
    ret += '/';
    ret += file;
    for (string::size_type i = 0; i < ret.length(); ++i)
    {
        if (ret[i] == '\\')
        {
            ret[i] = '/';
        }
    }
    return ret;
}
string Util::toNmdcFile(const string& file)
{
    if (file.empty())
        return Util::emptyString;
        
    string ret(file.substr(1));
    for (string::size_type i = 0; i < ret.length(); ++i)
    {
        if (ret[i] == '/')
        {
            ret[i] = '\\';
        }
    }
    return ret;
}

string Util::translateError(int aError)
{
    const string l_error_code = "[error: " + toString(aError) + "]";
#ifdef _WIN32
    LPTSTR lpMsgBuf = 0;
    DWORD l_wininet_flag = 0;
    LPCVOID lpSource = NULL;
    // http://code.google.com/p/flylinkdc/issues/detail?id=1077
    // http://stackoverflow.com/questions/2159458/why-is-formatmessage-failing-to-find-a-message-for-wininet-errors/2159488#2159488
    if (aError >= INTERNET_ERROR_BASE && aError < INTERNET_ERROR_LAST)
    {
        l_wininet_flag = FORMAT_MESSAGE_FROM_HMODULE;
        lpSource = GetModuleHandle(_T("wininet.dll"));
    }
    DWORD chars = FormatMessage(
                      FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS |
                      l_wininet_flag,
                      lpSource,
                      aError,
#if defined (_CONSOLE) || defined (_DEBUG)
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US), // US
#else
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
#endif
                      (LPTSTR) &lpMsgBuf,
                      0,
                      NULL
                  );
    if (chars == 0)
    {
        return l_error_code;
    }
    string tmp = Text::fromT(lpMsgBuf);
    // Free the buffer.
    LocalFree(lpMsgBuf);
    string::size_type i = 0;
    
    while ((i = tmp.find_first_of("\r\n", i)) != string::npos)
    {
        tmp.erase(i, 1);
    }
    return tmp + l_error_code;
#else // _WIN32
    return Text::toUtf8(strerror(aError));
#endif // _WIN32
}

TCHAR* Util::strstr(const TCHAR *str1, const TCHAR *str2, int *pnIdxFound)
{
    TCHAR *s1, *s2;
    TCHAR *cp = const_cast<TCHAR*>(str1);
    if (!*str2)
        return const_cast<TCHAR*>(str1);
    int nIdx = 0;
    while (*cp)
    {
        s1 = cp;
        s2 = (TCHAR *) str2;
        while (*s1 && *s2 && !(*s1 - *s2))
            s1++, s2++;
        if (!*s2)
        {
            if (pnIdxFound != NULL)
                *pnIdxFound = nIdx;
            return cp;
        }
        cp++;
        nIdx++;
    }
    if (pnIdxFound != NULL)
        *pnIdxFound = -1;
    return NULL;
}

string Util::formatStatus(int iStatus)
{
    string status;
    
    if (iStatus & Identity::NORMAL)
    {
        status += "Normal ";
    }
    if (iStatus & Identity::AWAY)
    {
        status += "Away ";
    }
    if (iStatus & Identity::SERVER)
    {
        status += "Fileserver ";
    }
    if (iStatus & Identity::FIREBALL)
    {
        status += "Fireball ";
    }
    if (iStatus & Identity::TLS)
    {
        status += "TLS ";
    }
    
    return (status.empty() ? "Unknown " : status) + "(" + toString(iStatus) + ")";
}
std::string Util::getRegistryCommaSubkey(const tstring& p_key)
{
	std::string l_result;
	std::string l_sep;
	HKEY l_hk = nullptr;
	TCHAR l_buf[512];
	l_buf[0] = 0;
	tstring l_key =  FLYLINKDC_REGISTRY_PATH _T("\\");
	l_key += p_key;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, l_key.c_str(), 0, KEY_READ, &l_hk) == ERROR_SUCCESS)
	{
		DWORD l_dwIndex = 0;
		DWORD l_len = _countof(l_buf);
		while (RegEnumValue(l_hk, l_dwIndex++, l_buf, &l_len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			l_result += l_sep + Text::fromT(l_buf);
			l_len = _countof(l_buf);
			if (l_sep.empty())
				l_sep = ',';
		}
		::RegCloseKey(l_hk);
	}
	return l_result;
}

string Util::getRegistryValueString(const tstring& p_key, bool p_is_path)
{
	HKEY hk = nullptr;
	TCHAR l_buf[512];
	l_buf[0] = 0;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, FLYLINKDC_REGISTRY_PATH, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
		DWORD l_bufLen = sizeof(l_buf);
		::RegQueryValueEx(hk, p_key.c_str(), NULL, NULL, (LPBYTE)l_buf, &l_bufLen);
        ::RegCloseKey(hk);
		if (l_bufLen)
        {
			string l_result = Text::fromT(l_buf);
            if (p_is_path)
				AppendPathSeparator(l_result); //[+]PPA
            return l_result;
        }
    }
    return emptyString;
}

bool Util::deleteRegistryValue(const tstring& p_value)
{
	HKEY hk = nullptr;
	if (::RegCreateKeyEx(HKEY_CURRENT_USER, FLYLINKDC_REGISTRY_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) != ERROR_SUCCESS)
	{
		return false;
	}
	const LSTATUS status = ::RegDeleteValue(hk, p_value.c_str());
	::RegCloseKey(hk);
	dcassert(status == ERROR_SUCCESS);
	return status == ERROR_SUCCESS;
}
//[+] SSA
bool Util::setRegistryValueString(const tstring& p_key, const tstring& p_value)
{
	HKEY hk = nullptr;
	if (::RegCreateKeyEx(HKEY_CURRENT_USER, FLYLINKDC_REGISTRY_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) != ERROR_SUCCESS)
	{
		return false;
	}
	const LSTATUS status = ::RegSetValueEx(hk, p_key.c_str(), NULL, REG_SZ, (LPBYTE)p_value.c_str(), sizeof(TCHAR) * (p_value.length() + 1));
	::RegCloseKey(hk);
	dcassert(status == ERROR_SUCCESS);
	return status == ERROR_SUCCESS;
}
    
string Util::getExternalIP(const string& p_url, LONG p_timeOut /* = 500 */)
{
	CFlyLog l_log("[GetIP]");
	string l_downBuf;
	getDataFromInet(_T("GetIP"), 256, p_url, l_downBuf, p_timeOut);
	if (!l_downBuf.empty())
	{
		SimpleXML xml;
    try
    {
			xml.fromXML(l_downBuf);
			if (xml.findChild("html"))
        {
				xml.stepIn();
				if (xml.findChild("body"))
        {
					const string l_IP = xml.getChildData().substr(20);
					l_log.step("Download : " + p_url + " IP = " + l_IP);
					if (isValidIP(l_IP))
            {
						return l_IP;
            }
            else
            {
						dcassert(0);
                    }
                }
            }
        }
		catch (SimpleXMLException & e)
        {
			l_log.step(string("Error parse XML: ") + e.what());
        }
    }
	else
		l_log.step("Error download : " + Util::translateError(GetLastError()));
	return Util::emptyString;
}

//[+] SSA
size_t Util::getDataFromInet(LPCWSTR agent, const DWORD frameBufferSize, const string& url, string& data, LONG timeOut /*=0*/, IDateReceiveReporter* reporter /* = NULL */)
    {
	std::vector<byte> l_bin_data;
	const size_t l_bin_size = Util::getBinaryDataFromInet(agent, frameBufferSize, url, l_bin_data, timeOut, reporter);
	if (l_bin_size)
	{
		data = string((char*)l_bin_data.data(), l_bin_size);
    }
	else
	{
		data.clear();
}

	return l_bin_size;
}
//[+] SSA
uint64_t Util::getBinaryDataFromInet(LPCWSTR agent, const DWORD frameBufferSize, const string& url, std::vector<byte>& p_dataOut, LONG timeOut /*=0*/, IDateReceiveReporter* reporter /* = NULL */)
{
	dcassert(frameBufferSize);
	dcassert(!url.empty());
    // FlylinkDC++ Team TODO: http://code.google.com/p/flylinkdc/issues/detail?id=632
    if (url.empty())
        return 0;
        
        CInternetHandle hInternet(InternetOpen(agent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0));
        if (!hInternet)
	{
		LogManager::getInstance()->message("InternetOpen [" + url + "] error = " + Util::translateError(GetLastError()));
		dcassert(0);
            return 0;
	}
            
	
        if (timeOut)
            InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeOut, sizeof(timeOut));
            
	CInternetHandle hURL(InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD, 0));
        if (!hURL)
        {
		dcassert(0);
		LogManager::getInstance()->message("InternetOpenUrl [" + url + "] error = " + Util::translateError(GetLastError()));
		// TODO - залогировать коды ошибок дл€ статы
            return 0;
        }
        bool isUserCancel = false;
	uint64_t totalBytesRead = 0;
	for (;;)
        {
		DWORD l_BytesRead = 0;
		p_dataOut.resize(totalBytesRead + frameBufferSize);
		if (!InternetReadFile(hURL, &p_dataOut[totalBytesRead], frameBufferSize, &l_BytesRead))
            {
			dcassert(0);
			LogManager::getInstance()->message("InternetReadFile [" + url + "] error = " + Util::translateError(GetLastError()));
			// TODO - залогировать коды ошибок дл€ статы
			return 0;
		}
		if (l_BytesRead == 0)
		{
                break;
            }
            else
            {
			totalBytesRead += l_BytesRead;
                if (reporter)
                {
				if (!reporter->ReportResultReceive(l_BytesRead))
                    {
                        isUserCancel = true;
                        break;
                    }
                }
            }
        }
        if (isUserCancel)
        {
		p_dataOut.clear();
            totalBytesRead = 0;
        }
	return totalBytesRead;
}


DWORD Util::GetTextResource(const int p_res, LPCSTR& p_data)
        {
	HRSRC hResInfo = FindResource(nullptr, MAKEINTRESOURCE(p_res), RT_RCDATA);
	if (hResInfo)
            {
		HGLOBAL hResGlobal = LoadResource(nullptr, hResInfo);
		if (hResGlobal)
		{
			p_data = (LPCSTR)LockResource(hResGlobal);
			if (p_data)
			{
				return SizeofResource(nullptr, hResInfo);
            }
        }
    }
	dcassert(0);
	return 0;
}

void Util::WriteTextResourceToFile(const int p_res, const tstring& p_file)
    {
	LPCSTR l_data;
	if (const DWORD l_size = GetTextResource(p_res, l_data))
	{
		std::ofstream l_file_out(p_file.c_str());
		l_file_out.write(l_data, l_size);
		return;
    }
	dcassert(0);
}

}

/**
 * @file
 * $Id: Util.cpp 575 2011-08-25 19:38:04Z bigmuscle $
 */
