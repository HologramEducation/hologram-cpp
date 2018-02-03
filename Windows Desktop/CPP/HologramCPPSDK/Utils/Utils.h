#pragma once
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <locale>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

typedef struct _LOCATION {
	time_t date;
	double latitude;
	double longitude;
	double altitude;
	double uncertainty;

}LOCATION;

typedef struct _SMS {
	std::wstring sender;
	time_t timestamp;
	std::wstring message;
}SMS;

// All functions prepended with of are taken from Openframeworks
//https://stackoverflow.com/questions/18906027/missing-punctuation-from-c-hex2bin
static std::string hex2bin(std::string const& s) {
	if (s.length() % 2 == 0) {
		return "";
	}

	std::string sOut;
	sOut.reserve(s.length() / 2);

	std::string extract;
	for (std::string::const_iterator pos = s.begin(); pos<s.end(); pos += 2)
	{
		extract.assign(pos, pos + 2);
		sOut.push_back(std::stoi(extract, nullptr, 16));
	}
	return sOut;
}

static std::string bin2hex(std::wstring const &s) {
	std::ostringstream oss;

	for (auto ch : s)
		oss << std::hex << std::setw(2) << std::setfill('0') << (int)ch;

	return oss.str();
}

static std::wstring StringToWstring(std::string source) {
	wchar_t *wbuffer = new wchar_t[source.length() + 1];
	size_t numChars;
	mbstowcs_s(&numChars, wbuffer, source.length() + 1, source.c_str(), source.length());
	std::wstring retWstr = wbuffer;
	delete[] wbuffer;
	return retWstr;
}

static std::string wStringToString(std::wstring source) {
	char *buffer = new char[source.length() + 1];
	size_t numChars;
	wcstombs_s(&numChars, buffer, source.length()+1, source.c_str(), source.length());
	std::string retStr = buffer;
	delete[] buffer;
	return retStr;
}

static bool StringToWstring(unsigned int nCodePage, const std::string& str, std::wstring& wstr)
{
	bool fRet = false;
	wchar_t* pBuffer = NULL;
	size_t nSize = 0;

	if (str.empty())
		return false;

	nSize = str.size();
	pBuffer = new wchar_t[nSize + 1];
	if (!pBuffer)
		return false;

	memset(pBuffer, 0, sizeof(wchar_t) * (nSize + 1));

	if (!MultiByteToWideChar(nCodePage, 0, str.c_str(), nSize, pBuffer, nSize + 1))
		goto END;

	wstr = pBuffer;

	// Done
	fRet = true;

END:
	if (pBuffer)
		delete[] pBuffer;

	return fRet;
}

//--------------------------------------------------
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
