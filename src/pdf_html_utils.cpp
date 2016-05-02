////////////////////////////////////////////////////////////////////////////////////////////////////
// pdf_html_utils.h
// Copyright (c) 2016 Pdfix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "pdf_html_utils.h"
#include <Windows.h>
extern HINSTANCE ghInstance;
#if defined _MSC_VER
#include <direct.h>
#elif defined __GNUC__
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
////////////////////////////////////////////////////////////////////////////////////////////////////
void ClearDir(const std::wstring& dir) {
  WIN32_FIND_DATA ffd;
  HANDLE hFind;
  std::wstring _dir(dir);
  if (_dir.back() != '/' && _dir.back() != '\\')
    _dir += L"/";
  std::wstring files = _dir + L"*.*";
  hFind = FindFirstFile(files.c_str(), &ffd);
  if (hFind == INVALID_HANDLE_VALUE)
    return;
  do {
    if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
      continue;
    std::wstring path = _dir + ffd.cFileName;
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      ClearDir(path);
      RemoveDirectory(path.c_str());
    }
    else {
      DeleteFile(path.c_str());
    }
  } while (FindNextFile(hFind, &ffd));
  FindClose(hFind);
}

int CreateDir(const std::string& dir) {
#if defined _MSC_VER 
  return _mkdir(dir.data());
#elif defined __GNUC__
  return mkdir(dir.data(), 0777);
#endif
}

void CheckDir(const std::wstring& path) {
  const auto _path = w2utf8(path.c_str());
  struct stat info;
  if (stat(_path.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR))
    if (CreateDir(_path))
      throw(0);
}

void Trim(std::wstring& str, const std::wstring& whitespace) {
  const auto strBegin = str.find_first_not_of(whitespace);
  if (strBegin == std::string::npos)
    return; // no content
  const auto strEnd = str.find_last_not_of(whitespace);
  const auto strRange = strEnd - strBegin + 1;
  str.erase(0, strBegin);
  str.erase(strRange, str.size() - strRange);
}

std::wstring GetPathDir(const std::wstring& path) {
  auto pos = path.find_last_of(L"\\/");
  if (pos == std::wstring::npos)
    throw(0);
  std::wstring dir(path.begin(), path.begin() + pos);
  return dir;
}

void SaveFileFromString(const std::string& str, const std::wstring& path) {
  CheckDir(GetPathDir(path));
  std::ofstream doc_stm(path, std::fstream::binary);
  doc_stm << str;
  doc_stm.close();
}

void SaveFileFromResource(int res_id, const std::wstring& path) {
  CheckDir(GetPathDir(path));
  HRSRC lRes = FindResource(ghInstance, MAKEINTRESOURCE(res_id), RT_HTML);
  HANDLE hFilter = LoadResource(ghInstance, lRes);
  int lSize = SizeofResource(ghInstance, lRes);
  LPSTR lBuffer = (char*)LockResource(hFilter);
  if (hFilter)
    UnlockResource(hFilter);
  std::string js;
  js.resize(lSize + 1);
  js = std::string(lBuffer, lSize);
  std::ofstream out(path, std::fstream::binary);
  out << js;
  out.close();
}

void ReplaceInStr(std::wstring& js, std::wstring find, std::wstring repl) {
  size_t pos = 0;
  while (true) {
    pos = js.find(find, pos);
    if (pos == std::wstring::npos)
      break;
    js.erase(pos, find.length());
    js.insert(pos, repl.c_str());
    pos += repl.length();
  }
}

void FixJS(std::wstring& js) {
  Trim(js);
  return;
}

std::string GetNewHtmlClassName() {
  static int counter = 0;
  return std::to_string(counter++);
}

std::string w2utf8(const wchar_t unicode) {
  std::string out;
  if ((unsigned int)unicode < 0x80) {
    out.push_back((char)unicode);
  }
  else {
    if ((unsigned int)unicode >= 0x80000000) {
      return out;
    }
    int nbytes = 0;
    if ((unsigned int)unicode < 0x800) {
      nbytes = 2;
    }
    else if ((unsigned int)unicode < 0x10000) {
      nbytes = 3;
    }
    else if ((unsigned int)unicode < 0x200000) {
      nbytes = 4;
    }
    else if ((unsigned int)unicode < 0x4000000) {
      nbytes = 5;
    }
    else {
      nbytes = 6;
    }
    static uint8_t prefix[] = { 0xc0, 0xe0, 0xf0, 0xf8, 0xfc };
    int order = 1 << ((nbytes - 1) * 6);
    int code = unicode;
    out.push_back(prefix[nbytes - 2] | (code / order));
    for (int i = 0; i < nbytes - 1; i++) {
      code = code % order;
      order >>= 6;
      out.push_back(0x80 | (code / order));
    }
  }
  return out;
}

std::string w2utf8(const wchar_t* wstr, int len) {
  if (!wstr)
    throw (0);
  if (len < 0) {
    len = (int)wcslen(wstr);
  }
  std::string out;
  while (len-- > 0) {
    out.append(w2utf8(*wstr++));
  }
  return out;
}

std::string w2utf8(const std::wstring& wstr) {
  return w2utf8(wstr.c_str());
}

std::string Color2HexStr(const PdfRGB& color) {
  std::stringstream stream;
  stream << "#";
  stream << std::setfill('0');
  stream << std::hex << std::setw(2) << color.r;
  stream << std::hex << std::setw(2) << color.g;
  stream << std::hex << std::setw(2) << color.b;
  return stream.str();
}