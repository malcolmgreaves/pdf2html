////////////////////////////////////////////////////////////////////////////////////////////////////
// pdf_html_utils.h
// Copyright (c) 2016 Pdfix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef pdf_html_utils_h
#define pdf_html_utils_h

#include "pdf_html_types.h"
#include <algorithm>
#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
////////////////////////////////////////////////////////////////////////////////////////////////////
void CheckDir(const std::wstring& path);
void ClearDir(const std::wstring& dir);
void Trim(std::wstring& str, const std::wstring& whitespace = L" \t\n\r");
std::wstring GetPathDir(const std::wstring& path);
void SaveFileFromString(const std::string& str, const std::wstring& path);
void SaveFileFromResource(int res_id, const std::wstring& path);
void FixJS(std::wstring& js);
std::string GetNewHtmlClassName();
std::string w2utf8(const wchar_t unicode);
std::string w2utf8(const wchar_t* wstr, int len = -1);
std::string w2utf8(const std::wstring& wstr);
std::string Color2HexStr(const PdfRGB& color);

#endif