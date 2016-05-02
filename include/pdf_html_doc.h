////////////////////////////////////////////////////////////////////////////////////////////////////
// pdf_html_doc.h
// Copyright (c) 2016 Pdfix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef pdf_html_doc_h
#define pdf_html_doc_h

#include "pdf_html_types.h"
#include <string>

struct PdfHtmlData {
  std::string html;
  std::string css;
  std::string js;
  void Append(PdfHtmlData& data) {
    html += data.html;
    css += data.css;
    js += data.js;
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class PdfHtmlDoc
// Converts PDF documents to HTML
////////////////////////////////////////////////////////////////////////////////////////////////////
class PdfHtmlDoc {
public:
  // Constructs html document from PDFDoc object
  PdfHtmlDoc(Pdfix* pdfix, PdfDoc* doc);

  // Close PDF documents
  ~PdfHtmlDoc();

  // Generates HTML output based on PdfHtmlSettings settings 
  void GetHtml(PdfHtmlSettings& settings, PdfHtmlType html_type);

private:
  std::string GetTextStateCSS(const std::string& cls_id,
    PdfTextState& ts,
    PdfTextAlignment alignment,
    double indent);

  int GetCharAnnotFlag(PdfRect& char_bbox);

  void GetActionHtml(PdfWidgetAnnot* annot,
    PdfActionEventType event,
    const std::string& obj_id,
    std::string& doc_js,
    std::string& obj_attrs);

  // Process element by type
  void GetFormFieldHtml(PdeElement* element, PdeElement* parent, 
    PdfHtmlType html_type, PdfHtmlData* html_data);
  void GetTextHtml(PdeElement* element, PdeElement* parent, 
    PdfHtmlType html_type, PdfHtmlData* html_data);
  void GetImageHtml(PdeElement* element, PdeElement* parent, 
    PdfHtmlType html_type, PdfHtmlData* html_data);
  void GetRectHtml(PdeElement* element, PdeElement* parent, 
    PdfHtmlType html_type, PdfHtmlData* html_data);
  void GetTableHtml(PdeElement* element, PdeElement* parent, 
    PdfHtmlType html_type, PdfHtmlData* html_data);
  void GetElementHtml(PdeElement* element, PdeElement* parent, 
    PdfHtmlType html_type, PdfHtmlData* html_data);
  void GetPageHtml(PdfHtmlType html_type, PdfHtmlData* html_data);

  // Saves html, css and js files to specified directories
  void SaveHtml();

private:
  PdfDoc* m_doc;                // PDF document object
  PdfHtmlSettings m_settings;   // conversion settings
  Pdfix* m_pdfix;               // pointer to the Pdfix main library class
  bool m_own_doc;               // true if PDF document was opened in constructor
  std::wstring m_path;          // stores PDF document path from constructor
  PdfPage* m_page;              // current page
  int m_page_num;               // current page number
  PdfPageView* m_page_view;     // current page view
  PdfImage* m_page_image;       // current page rendering output
  PdfHtmlData m_html_data;      // html data
};

#endif