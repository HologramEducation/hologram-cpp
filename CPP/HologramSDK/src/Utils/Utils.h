#pragma once
#include <cstdio>
#include <vector>
#include <locale>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <map>

#if defined( __WIN32__ ) || defined( _WIN32 )
#define TARGET_WINDOWS
#ifndef IOTCORE
//#define USERAS 1
#endif
#include <windows.h>
#elif defined( __APPLE_CC__)
#define TARGET_MACOS
#elif defined (__ANDROID__)
#define TARGET_ANDROID
#elif defined(__ARMEL__)
#define TARGET_LINUX
#define TARGET_LINUX_ARM
#else
#define TARGET_LINUX
#endif

typedef struct _LOCATION {
	time_t date;
	double latitude;
	double longitude;
	double altitude;
	double uncertainty;

}LOCATION;

typedef struct _SMS {
	_SMS(std::wstring sender, time_t timestamp, std::wstring message) :
		sender(sender), timestamp(timestamp), message(message) {

	}
	_SMS() {}
	std::wstring sender;
	time_t timestamp;
	std::wstring message;
}SMS;

const std::wstring GSM = L"@£$¥èéùìòÇ\nØø\rÅåΔ_ΦΓΛΩΠΨΣΘΞ ÆæßÉ !\"#¤%&'()*+,-./0123456789:;<=>?¡ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÑÜ§¿abcdefghijklmnopqrstuvwxyzäöñüà";
static std::map<char, wchar_t> EXT = { 
	{ 0x40, L'|' }, 
	{ 0x14, L'^' }, 
	{ 0x65, L'€' }, 
	{ 0x28, L'{' }, 
	{ 0x29, L'}' }, 
	{ 0x3C, L'[' }, 
	{ 0x3D, L'~' }, 
	{ 0x3E, L']' }, 
	{ 0x2F, L'\\'} 
};

// All functions prepended with of are taken from Openframeworks
//https://stackoverflow.com/questions/18906027/missing-punctuation-from-c-hex2bin
static std::string fromHex(std::string const& s) {
	if (s.length() % 2 != 0) {
		return "";
	}

	std::string sOut;
	sOut.reserve(s.length() / 2);

	std::string extract;
	for (std::string::const_iterator pos = s.begin(); pos < s.end(); pos += 2)
	{
		extract.assign(pos, pos + 2);
		sOut.push_back(std::stoi(extract, nullptr, 16));
	}
	return sOut;
}

static std::string toHex(const std::string& s)
{
	std::ostringstream ret;

	for (std::string::size_type i = 0; i < s.length(); ++i)
		ret << std::hex << std::setfill('0') << std::setw(4) << std::nouppercase << (int)s[i];

	return ret.str();
}

static std::string toHex(const int i)
{
    std::stringstream ret;
    ret << std::hex << std::setfill('0') << std::setw(2) << i;
    
    return ret.str();
}
static wchar_t gsm7toChar(unsigned char gsmChar, bool & inExtended) {
	if (inExtended) {
		inExtended = false;
		if (EXT.count(gsmChar) > 0) {
			return EXT[gsmChar];
		}
	}
	else if (gsmChar == 0x1B) {
		inExtended = true;
		return ' ';
	}
	else if (gsmChar < GSM.length())
		return GSM[gsmChar];
	return ' ';
}

static std::wstring convertGSM7to8bit(std::string message, int msg_len) {
	int last = 0;
	int current = 0;
	int i = 0;
	std::wstring msg;
	bool inExt = false;
	for (int count = 0; count < msg_len; count++) {
		int offset = count % 8;
		last = current;
		if (offset < 7) {
			current = int(((unsigned char)fromHex(message.substr(i * 2, 2))[0]));
			i += 1;
			char gsmChar = (last >> (8 - offset)) | (current << offset);
			msg += gsm7toChar(gsmChar & 0x7F, inExt);
		}
	}
	return msg;
}

static std::string switchCharPairs(std::string strToSwap) {
	std::string swappedString;
	if (strToSwap.length() % 2 == 0) {
		for (unsigned int i = 0; i < strToSwap.length(); i += 2) {
			std::string swapPair = strToSwap.substr(i, 2);
			swappedString += swapPair[1] + swapPair[0];
		}
	}
	return swappedString;
}

static std::wstring toWString(std::string source) {
	wchar_t *wbuffer = (wchar_t *)malloc((source.length() + 1) * sizeof(wchar_t));
	size_t len = mbstowcs(wbuffer, source.c_str(), source.length());
	std::wstring retWstr = wbuffer;
	delete[] wbuffer;
	return retWstr.substr(0, len);
}

static std::string fromWString(std::wstring source) {
	char *buffer = new char[source.length() + 1];
	size_t len = wcstombs(buffer, source.c_str(), source.length());
	std::string retStr = buffer;
	delete[] buffer;
	return retStr.substr(0, len);
}

//--------------------Openframeworks String Manipulation ------------------------------
static std::string ofTrimFront(const std::string & src, const std::string& locale) {
	auto dst = src;
	std::locale loc = std::locale(locale.c_str());
	dst.erase(dst.begin(), std::find_if_not(dst.begin(), dst.end(), [&](char & c) {return std::isspace<char>(c, loc); }));
	return dst;
}

//--------------------------------------------------
static std::string ofTrimBack(const std::string & src, const std::string& locale) {
	auto dst = src;
	std::locale loc = std::locale(locale.c_str());
	dst.erase(std::find_if_not(dst.rbegin(), dst.rend(), [&](char & c) {return std::isspace<char>(c, loc); }).base(), dst.end());
	return dst;
}

//--------------------------------------------------
static std::string ofTrim(const std::string & src, const std::string& locale) {
	return ofTrimFront(ofTrimBack(src, locale), locale);
}

//--------------------------------------------------
static std::vector <std::string> ofSplitString(const std::string & source, const std::string & delimiter, bool ignoreEmpty = false, bool trim = false) {
	std::vector<std::string> result;
	if (delimiter.empty()) {
		result.push_back(source);
		return result;
	}
	std::string::const_iterator substart = source.begin(), subend;
	while (true) {
		subend = search(substart, source.end(), delimiter.begin(), delimiter.end());
		std::string sub(substart, subend);
		if (trim) {
			sub = ofTrim(sub, "");
		}
		if (!ignoreEmpty || !sub.empty()) {
			result.push_back(sub);
		}
		if (subend == source.end()) {
			break;
		}
		substart = subend + delimiter.size();
	}
	return result;
}

//--------------------------------------------------
static std::string ofJoinString(const std::vector<std::string>& stringElements, const std::string& delimiter) {
	std::string str;
	if (stringElements.empty()) {
		return str;
	}
	auto numStrings = stringElements.size();
	std::string::size_type strSize = delimiter.size() * (numStrings - 1);
	for (const std::string &s : stringElements) {
		strSize += s.size();
	}
	str.reserve(strSize);
	str += stringElements[0];
	for (decltype(numStrings) i = 1; i < numStrings; ++i) {
		str += delimiter;
		str += stringElements[i];
	}
	return str;
}

//--------------------------------------------------
static void ofStringReplace(std::string& input, const std::string& searchStr, const std::string& replaceStr) {
	auto pos = input.find(searchStr);
	while (pos != std::string::npos) {
		input.replace(pos, searchStr.size(), replaceStr);
		pos += replaceStr.size();
		std::string nextfind(input.begin() + pos, input.end());
		auto nextpos = nextfind.find(searchStr);
		if (nextpos == std::string::npos) {
			break;
		}
		pos += nextpos;
	}
}

//--------------------------------------------------
static bool ofIsStringInString(const std::string& haystack, const std::string& needle) {
	return haystack.find(needle) != std::string::npos;
}

//--------------------------------------------------
static std::size_t ofStringTimesInString(const std::string& haystack, const std::string& needle) {
	const size_t step = needle.size();

	size_t count(0);
	size_t pos(0);

	while ((pos = haystack.find(needle, pos)) != std::string::npos) {
		pos += step;
		++count;
	}

	return count;
}
