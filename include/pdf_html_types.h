////////////////////////////////////////////////////////////////////////////////////////////////////
// pdf_html_types.h
// Copyright (c) 2016 Pdfix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef pdf_html_types_h
#define pdf_html_types_h

#include "pdfix.h"
#include <string>

typedef enum {
  kPdfHtmlFixed = 0,
  kPdfHtmlResponsive
} PdfHtmlType;

const int kTextFlagLink = 0x200;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class PdfHtmlSettings
////////////////////////////////////////////////////////////////////////////////////////////////////
class PdfHtmlSettings {
public:
  PdfHtmlSettings()
    : m_acroform(nullptr) {
  }

  PdfHtmlSettings(
    const std::wstring& html_path,
    PdfHtmlType html_type = kPdfHtmlFixed,
    bool acroform = true,
    const std::wstring& js_dir = L"js/",
    const std::wstring& img_dir = L"images/")
    : m_html_path(html_path),
    m_js_dir(js_dir),
    m_img_dir(img_dir),
    m_acroform(acroform) {
  }

  PdfHtmlSettings(PdfHtmlSettings& settings)
    : m_html_path(settings.m_html_path),
    m_js_dir(settings.m_js_dir),
    m_img_dir(settings.m_img_dir),
    m_acroform(settings.m_acroform) {
  }

public:
  std::wstring m_html_path;     // absolute path for storing html
  std::wstring m_img_dir;	      // relative path for storing the javascripts
  std::wstring m_js_dir;	      // relative path for storing the images
  bool m_acroform;              // include acroform
};

typedef class PdfHtmlSettings* PdfHtmlSettingsP;

#endif