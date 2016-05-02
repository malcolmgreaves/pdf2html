////////////////////////////////////////////////////////////////////////////////////////////////////
// pdfix_export_html.cpp
// Copyright (c) 2016 PDFix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <tchar.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <windows.h>
#include <memory>
#include <functional>

#include "resource.h"
#include "pdfix.h"
#include "pdf_html_types.h"
#include "pdf_html_doc.h"
#include "pdf_html_utils.h"

HINSTANCE ghInstance = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Main method
// param_1(optional)  -r (responsible output)
// param_2(required)  input PDF file
// param_3(required)  HTML output folder
////////////////////////////////////////////////////////////////////////////////////////////////////
int _tmain(int argc, _TCHAR* argv[]) {
  if (argc <= 3) {
    printf("invalid params: -r (optional) <input_file> <output_folder>");
    return 1;
  }

  auto responsive = false;
  std::wstring input_file;
  std::wstring html_folder;

  // check parameters
  for (int i = 1; i < argc; i++) {
    std::wstring arg(argv[i]);
    if (arg.compare(L"-r") == 0) {
      responsive = true;
    }
    else if (input_file.size() == 0) {
      input_file = arg;
    }
    else if (html_folder.size() == 0) {
      html_folder = arg;
    }
  }
  if (input_file.empty()) {
    printf("Missing input file");
    return 1;
  }
  if (html_folder.empty())
    html_folder = L"c:/temp/html/";

  CheckDir(html_folder);
  ClearDir(html_folder);

  // set valid dll name
  std::string lib_name = "pdfix";
#ifdef _WIN64
  lib_name += "64";
#endif
  lib_name += ".dll";

  Pdfix_init(lib_name.c_str());

  Pdfix* pdfix = GetPdfix();
  if (!pdfix)
    return 0;

  PdfDoc* doc = nullptr;
  try {
    auto authorized = pdfix->Authorize(L"e_mail", L"license_key");
    if (!authorized)
      throw(0);
    doc = pdfix->OpenDoc(input_file.c_str(), L"");
    if (!doc)
      throw(0);
    PdfHtmlSettings settings(html_folder);
    PdfHtmlDoc html_doc(pdfix, doc);
    if (responsive)
      html_doc.GetHtml(settings, kPdfHtmlResponsive);
    else
      html_doc.GetHtml(settings, kPdfHtmlFixed);
  }
  catch (...) {
  }

  if (doc)
    doc->Close();
  if (pdfix)
    pdfix->Destroy();

  return 0;
}

