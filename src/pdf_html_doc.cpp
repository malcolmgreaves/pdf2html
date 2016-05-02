////////////////////////////////////////////////////////////////////////////////////////////////////
// pdfhtml_doc.h
// Copyright (c) 2016 Pdfix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "resource.h"
#include "pdf_html_doc.h"
#include "pdf_html_types.h"
#include "pdf_html_utils.h"
#include <sstream>
#include <fstream>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class PdfHtmlDoc
////////////////////////////////////////////////////////////////////////////////////////////////////
PdfHtmlDoc::PdfHtmlDoc(Pdfix* pdfix, PdfDoc* doc)
  : m_pdfix(pdfix),
  m_doc(doc),
  m_own_doc(false),
  m_page(nullptr),
  m_page_num(0),
  m_page_view(nullptr),
  m_page_image(nullptr) {
}

PdfHtmlDoc::~PdfHtmlDoc() {
  if (m_own_doc && m_doc) {
    m_doc->Close();
    m_doc = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GetFormFieldHtml
////////////////////////////////////////////////////////////////////////////////////////////////////
void PdfHtmlDoc::GetActionHtml(PdfWidgetAnnot* annot,
  PdfActionEventType event,
  const std::string& obj_id,
  std::string& doc_js,
  std::string& obj_attrs) {
  try {
    PdfAction* action = nullptr;
    if (event == kActionEventAnnotMouseUp)
      action = annot->GetAction();
    if (!action)
      action = annot->GetAAction(event);
    if (!action)
      return;

    std::string js, props;
    std::string prefix;
    switch (event) {
    case kActionEventAnnotEnter:break;
    case kActionEventAnnotExit:break;
    case kActionEventAnnotMouseDown:
      prefix = "D"; break;
    case kActionEventAnnotMouseUp:
      prefix = "U"; break;
    case kActionEventAnnotFocus:
      prefix = "Fo"; break; //forms only!
    case kActionEventAnnotBlur:
      prefix = "Bl"; break; //forms only!
    case kActionEventAnnotPageOpen: break;
    case kActionEventAnnotPageClose: break;
    case kActionEventAnnotPageVisible:break;
    case kActionEventAnnotPageInvisible:break;
    case kActionEventFieldKeystroke:
      prefix = "K"; break;
    case kActionEventFieldFormat:
      prefix = "F"; break;
    case kActionEventFieldValidate:
      prefix = "V"; break;
    case kActionEventFieldCalculate:
      prefix = "C"; break;
    }

    std::string a_name = prefix + std::to_string((int)annot) + "()";
    PdfActionType action_type = action->GetSubtype();
    std::wstring str_js;

    if (action_type == kActionJavaScript) {
      str_js.resize(action->GetJavaScript(nullptr, 0));
      action->GetJavaScript((wchar_t*)str_js.c_str(), (int)str_js.size());
      FixJS(str_js);
    }
    else if (action_type == kActionResetForm) {
      str_js = L"window.resetForm(null);\n";
    }
    else if (action_type == kActionSubmitForm) {
      str_js = L"window.submitForm('http://192.168.1.110/');\n";
    }

    if (str_js.length() > 0) {
      js += "function " + a_name + " {\n";
      js += w2utf8(str_js.c_str()) + "\n";
      js += "}\n\n";
    }
    doc_js += js;
    obj_attrs += props;
  }
  catch (...) {
  }
}

void PdfHtmlDoc::GetFormFieldHtml(PdeElement* element, PdeElement* parent,
  PdfHtmlType html_type, PdfHtmlData* html_data) {
  PdeFormField* form_field = (PdeFormField*)element;
  try {
    // get the element bouding box and rect
    PdfRect elem_rect;
    PdfDevRect elem_dev_rect;
    element->GetBBox(&elem_rect);
    m_page_view->RectToDevice(&elem_rect, &elem_dev_rect);
    double elem_width = elem_dev_rect.right - elem_dev_rect.left;
    double elem_height = elem_dev_rect.bottom - elem_dev_rect.top;
    if (elem_height == 0 || elem_width == 0)
      return;

    std::string class_id = "obj_" + GetNewHtmlClassName();
    PdfWidgetAnnot* annot = form_field->GetWidgetAnnot();
    if (!annot)
      return;

    html_data->css += "." + class_id + " {\n";
    //html_data->css += "display: block; \n";
    //html_data->css += "position: absolute; \n";

    // get parent's elelemnt rect if set
    PdfDevRect parent_dev_rect;
    if (parent) {
      PdfRect parent_rect;
      parent->GetBBox(&parent_rect);
      m_page_view->RectToDevice(&parent_rect, &parent_dev_rect);
    }

    if (html_type == kPdfHtmlFixed) {
      int page_width(0), page_height(0);
      if (parent) {
        //update the element dev rect to be relative to parent element
        elem_dev_rect.left -= parent_dev_rect.left;
        elem_dev_rect.top -= parent_dev_rect.top;
        elem_dev_rect.right -= parent_dev_rect.left;
        elem_dev_rect.bottom -= parent_dev_rect.top;
        page_width = parent_dev_rect.right - parent_dev_rect.left;
        page_height = parent_dev_rect.bottom - parent_dev_rect.top;
      }
      else {
        page_width = m_page_view->GetDeviceWidth();
        page_height = m_page_view->GetDeviceHeight();
      }

      double left = elem_dev_rect.left / (double)page_width * 100.;
      double top = elem_dev_rect.top / (double)page_height * 100.;
      double width2 = (elem_dev_rect.right - elem_dev_rect.left) / (double)page_width * 100.;
      double height2 = (elem_dev_rect.bottom - elem_dev_rect.top) / (double)page_height * 100.;

      html_data->css += "left: " + std::to_string(left) + "%; \n";
      html_data->css += "top: " + std::to_string(top) + "%; \n";
      html_data->css += "width: " + std::to_string(width2) + "%; \n";
      html_data->css += "height: " + std::to_string(height2) + "%; \n";
    }
    else {
      // responsive?
    }

    PdfAnnotSubtype subtype = annot->GetSubtype();
    PdfAnnotFlags flags = annot->GetFlags();
    if ((flags & kAnnotFlagInvisible) || (flags & kAnnotFlagHidden))
      html_data->css += "display: none; \n";

    PdfAnnotAppearance ap;
    annot->GetAppearance(&ap);

    // form field with border and transparent background
    // this cannot be applyed on all annotation types
    /* if (ap.fill_type == kFillTypeSolid) {
      html_data->css += "background-color: rgb(" +
        std::to_string(ap.fill_color.r) + ", " +
        std::to_string(ap.fill_color.g) + ", " +
        std::to_string(ap.fill_color.b) + "); ";
    }
    else
      html_data->css += "background-color: transparent; \n";

    html_data->css += "border-width: " + std::to_string(ap.border_width) + "px; ";
    if (ap.border_width > 0)
    {
      html_data->css += "border-style: solid; ";
      html_data->css += "border-color: rgb(" +
        std::to_string(ap.border_color.r) + ", " +
        std::to_string(ap.border_color.g) + ", " +
        std::to_string(ap.border_color.b) + "); ";
    }
    */

    PdfFormField* field = annot->GetFormField();
    if (!field)
      return;

    html_data->js += "field_add_annot(" +
      std::to_string((int)field) + ", " +
      std::to_string((int)annot) + " );\n";

    PdfFieldType field_type = field->GetType();
    bool has_action = false;
    
    //TODO: in case of Btn field (Push, Check, Radio) we may require it's appearance to be
    //drawn as an image (SimpleFormCalculations.pdf)

    // PdfActionP action = PdfFormFieldGetFieldAction(field);
    // std::string a_name = "a_" + std::to_string((int)field) + "()";
    // if (action) {
    //std::wstring js_a;
    //PdfActionType action_type = PdfActionGetType(action);
    //if (action_type == kActionJavaScript) {
    //  js_a.resize(PdfActionGetJavaScript(action, nullptr, 0));
    //  PdfActionGetJavaScript(action, (wchar_t*)js_a.c_str(), js_a.size());
    //  FixJS(js_a);
    //}
    //else if (action_type == kActionResetForm) {
    //  js_a += L"window.resetForm(null);\n";
    //}
    //else if (action_type == kActionSubmitForm) {
    //  js_a += L"window.submitForm('http://192.168.1.110/');\n";
    //}
    //has_action = js_a.length() > 0;
    //if (has_action) {
    //  js + "function " + a_name + " {\n";
    //  js + w2utf8(js_a.c_str()) + "\n";
    //  js + "}\n\n";
    //}
    // }

    PdfFieldFlags field_flags = field->GetFlags();
    std::wstring value, default_value;
    value.resize(field->GetValue(NULL, 0));
    field->GetValue((wchar_t*)value.c_str(), (int)value.size());
    default_value.resize(field->GetDefaultValue(NULL, 0));
    field->GetDefaultValue((wchar_t*)default_value.c_str(), (int)default_value.size());

    std::wstring full_name;
    full_name.resize(field->GetFullName(NULL, 0));
    field->GetFullName((wchar_t*)full_name.c_str(), (int)full_name.size());
    std::wstring tooltip;
    tooltip.resize(field->GetTooltip(NULL, 0));
    field->GetTooltip((wchar_t*)tooltip.c_str(), (int)tooltip.size());
    if (tooltip.length() == 0)
      tooltip = full_name;

    int max_length = field->GetMaxLength();
    std::string name, inn_html, form_id, type, props;
    form_id = std::to_string((int)field);

    std::string action_js, action_obj_props;
    GetActionHtml(annot, kActionEventAnnotMouseUp, 
      w2utf8(full_name.c_str()), action_js, action_obj_props);
    GetActionHtml(annot, kActionEventAnnotMouseDown, 
      w2utf8(full_name.c_str()), action_js, action_obj_props);
    GetActionHtml(annot, kActionEventFieldKeystroke, 
      w2utf8(full_name.c_str()), action_js, action_obj_props);
    GetActionHtml(annot, kActionEventFieldFormat, 
      w2utf8(full_name.c_str()), action_js, action_obj_props);

    std::string pdf_obj_cls = html_type == kPdfHtmlFixed ? "pdf-obj-fixed" : " pdf-obj";

    props += action_obj_props;
    html_data->js += action_js;
    props += "id=\"" + w2utf8(full_name.c_str()) + "\" ";
    props += "class=\"" + class_id + " " + pdf_obj_cls + " pdf-form-field \"";
    props += "name=\"" + w2utf8(full_name.c_str()) + "\" ";
    props += "formid=\"" + form_id + "\" ";
    props += "annot=\"" + std::to_string((int)annot) + "\" ";

    if (field_flags & kFieldFlagDCommitOnSelChange) {
      props += "commitOnSelChange=\"true\" ";
    }

    //std::wstring font_name;
    //font_name.resize(PdfAnnotGetFontName(field, nullptr, 0));
    //PdfFormFieldGetFontName(field, (wchar_t*)font_name.c_str(), font_name.size());
    double font_size = ap.font_size;
    if (font_size != 0)
      html_data->css += "font-size: " + std::to_string(font_size) + "px; ";

    html_data->css += "padding: 0px 0px; \n";

    if (field_flags & kFieldFlagReadOnly)
      props += "readonly ";

    if (field_type == kFieldButton) {
      std::wstring caption;
      caption.resize(annot->GetCaption(nullptr, 0));
      annot->GetCaption((wchar_t*)caption.c_str(), (int)caption.size());
      name = "button";
      inn_html = w2utf8(caption.c_str());
    }
    else if (field_type == kFieldText && (field_flags & kFieldFlagMultiline)) {
      name = "textarea";
      inn_html = w2utf8(value.c_str());
      html_data->css += "resize: none; \n";
      html_data->css += "overflow: hidden; \n";
    }
    else if (field_type == kFieldCombo || field_type == kFieldList) {
      name = "select";
      int count = field->GetOptionCount();
      for (int i = 0; i < count; i++) {
        std::wstring opt_value, opt_caption;
        opt_value.resize(field->GetOptionValue(i, nullptr, 0));
        field->GetOptionValue(i, (wchar_t*)opt_value.c_str(), (int)opt_value.size());
        opt_caption.resize(field->GetOptionCaption(i, nullptr, 0));
        field->GetOptionCaption(i, (wchar_t*)opt_caption.c_str(), (int)opt_caption.size());
        // TODO: default selected value
        bool selected = (value.compare(opt_value.empty() ? opt_caption : opt_value) == 0);
        inn_html += "<option ";
        if (selected) inn_html += "selected ";
        inn_html += "value=\"" + w2utf8(opt_value.c_str()) + "\">" +
          w2utf8(opt_caption.c_str());
        inn_html += "</option>\n";
      }
      if (field_type == kFieldList)
        props += "size=3 ";
      if (field_flags & kFieldFlagMultiSelect)
        props += "multiple ";
    }
    else {
      std::string type = "text";
      if (field_type == kFieldText) {
        if (field_flags & kFieldFlagPassword) type = "password";
        else if (field_flags & kFieldFlagFileSelect) type = "file";
        else {
          //if (js_k.size()) {
          //  if (js_k.find(L"AFNumber_Keystroke") == 0) type = "number";
          //  else if (js_k.find(L"AFDate_Keystroke") == 0) {
          //    type = "date";
          //  }
          //  else if (js_k.find(L"AFTime_Keystroke") == 0) {
          //    type = "time";
          //  }
          //else if (js_str.find(L"AFPercent_Keystroke") == 0) {}
          //}
        }
        props += "value=\"" + w2utf8(value.c_str()) + "\" ";
        if (max_length)
          props += "maxLength=" + std::to_string(max_length) + " ";
      }
      else if (field_type == kFieldCheck || field_type == kFieldRadio) {
        if (field_type == kFieldRadio)
          type = "radio";
        else
          type = "checkbox";
        html_data->css += "margin: 0px 0px 0px 0px;\n";
        //get export value of this radio button/checkbox
        std::wstring exp_value;
        exp_value.resize(field->GetWidgetExportValue(annot, nullptr, 0));
        field->GetWidgetExportValue(annot, (wchar_t*)exp_value.c_str(), (int)exp_value.size());
        props += "value=\"" + w2utf8(exp_value) + "\" ";
      }

      name = "input";
      props += "type=\"" + type + "\" ";
    }
    if (default_value.length() > 0)
      props += "defaultValue=\"" + w2utf8(default_value.c_str()) + "\" ";
    // element events
    // props += "onkeypress=\"return do_change(this, false);\" ";
    // props += "onchange=\"do_change(this, false); if (do_change(this, true)) do_calculations();\" ";
    html_data->html += "<" + name + " "/* + style + " " */ + props + ">\n";
    html_data->html += inn_html + "\n";
    html_data->html += "</" + name + ">\n";

    html_data->css += "}\n\n";
  }
  catch (...) {
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GetTextHtml
////////////////////////////////////////////////////////////////////////////////////////////////////
std::string PdfHtmlDoc::GetTextStateCSS(const std::string& cls_id, 
  PdfTextState& ts,
  PdfTextAlignment alignment,
  double indent) {
  std::string css = "." + cls_id + " {\n";
  std::wstring font_name, sys_font_name;
  std::string font_family = "sans-serif";
  PdfFontCharset charset = kFontUnknownCharset;
  PdfFont* font = ts.font;
  PdfFontState fs;
  font->GetFontState(&fs);
  bool bold = fs.bold == 1;
  bool italic = fs.italic == 1;
  int fw = 400;
  if (bold)
    fw = 800;
  sys_font_name.resize(font->GetSystemFontName(nullptr, 0));
  if (sys_font_name.size() > 0) {
    font->GetSystemFontName((wchar_t*)sys_font_name.c_str(), (int)sys_font_name.size());
    charset = font->GetSystemFontCharset();
  }
  font_name.resize(font->GetFontName(nullptr, 0));
  font->GetFontName((wchar_t*)font_name.c_str(), (int)font_name.size());

  css += "font-family: ";
  if (sys_font_name.length())
    css += "\"" + w2utf8(sys_font_name) + "\",";
  css += font_family + ";\n";
  if (italic)
    css += "font-style: italic;\n";
  css += "font-size: " + std::to_string(ts.font_size/**96./72.*/) + "px;\n";
  css += "font-weight: " + std::to_string(fw) + ";\n";
  css += "color: " + Color2HexStr(ts.color_state.fill_color) + ";\n";
  if (ts.flags & kTextFlagUnderline)
    css += "text-decoration: underline;\n";
  if (ts.flags & kTextFlagStrikeout)
    css += "text-decoration: line-through;\n";
  if (ts.flags & kTextFlagHighlight)
    css += "background-color: yellow;\n";

  // doesn't work yet
  //switch (alignment) {
  //case kAlignmentLeft:        css += "text-align: left;\n"; break;
  //case kAlignmentLeftReflow:  css += "text-align: left;\n"; break;
  //case kAlignmentRight:       css += "text-align: right;\n"; break;
  //case kAlignmentRightReflow: css += "text-align: right;\n"; break;
  //case kAlignmentCenter:      css += "text-align: center;\n"; break;
  //case kAlignmentJustify:     css += "text-align: justify;\n"; break;
  //}
  css += "text-align: left;\n";
  if (indent)
    css += "text-indent: " + std::to_string(indent) + "px; \n";
  css += "}\n\n";

  return css;
};

int PdfHtmlDoc::GetCharAnnotFlag(PdfRect& char_bbox) {
  int flags = 0;
  // get annotations over the bbox
  int num_annots = m_page->GetNumAnnots();
  for (int i = 0; i < num_annots; i++) {
    PdfAnnot* annot = m_page->GetAnnot(i);
    int char_has_annot = annot->RectInAnnot(&char_bbox);
    if (!char_has_annot) {
      PdfAnnotSubtype subtype = annot->GetSubtype();
      // support any type of annotation you want
      switch (subtype) {
      case kAnnotLink: {
        flags |= kTextFlagLink;
        PdfLinkAnnot* link_annot = (PdfLinkAnnot*)annot;
        PdfAction* action = link_annot->GetAction();
        if (action) {
          std::wstring link;
          link.resize(action->GetURI(nullptr, 0));
          action->GetURI((wchar_t*)link.c_str(), (int)link.size());
        }
      } break;
      case kAnnotHighlight:
        flags |= kTextFlagHighlight;
        break;
      case kAnnotUnderline:
      case kAnnotSquiggly:
        flags |= kTextFlagUnderline;
      case kAnnotStrikeOut:
        flags |= kTextFlagStrikeout;
      }
    }
  }
  return flags;
}

void PdfHtmlDoc::GetTextHtml(PdeElement* element, PdeElement* parent,
  PdfHtmlType html_type, PdfHtmlData* html_data) {
  PdeText* text = (PdeText*)element;
  try {
    PdfRect elem_rect;
    PdfDevRect elem_dev_rect;
    element->GetBBox(&elem_rect);
    m_page_view->RectToDevice(&elem_rect, &elem_dev_rect);
    double elem_width = elem_dev_rect.right - elem_dev_rect.left;
    double elem_height = elem_dev_rect.bottom - elem_dev_rect.top;
    if (elem_dev_rect.right == elem_dev_rect.left || elem_dev_rect.top == elem_dev_rect.bottom)
      return;

    // fixed layout
    if (html_type == kPdfHtmlFixed) {
      //get font properties
      PdfTextState ts;
      text->GetTextState(&ts);
      double font_size = ts.font_size;
      int flags = 0; // PdfTextElementGetFontFlags(m_element, 0);
      std::string font_family = "sans-serif";
      std::string font_family_cls = "ff_ss";
      if (flags & kFontFixedPitch) {
        font_family = "\"Courier New\", monospace";
        font_family_cls = "ff_m";
      }
      if (flags & kFontScript) {
        font_family = "Verdana, serif";
        font_family_cls = "ff_s";
      }

      // in case parent is set modify the rect to relative position to it's parent
      PdfDevRect parent_dev_rect;
      if (parent != nullptr) {
        PdfRect parent_rect;
        parent->GetBBox(&parent_rect);
        m_page_view->RectToDevice(&parent_rect, &parent_dev_rect);
      }

      // go through lines one by one
      int num_lines = text->GetNumTextLines();
      for (int line_index = 0; line_index < num_lines; line_index++) {
        PdeTextLine* text_line = text->GetTextLine(line_index);
        PdfRect line_rect;
        PdfDevRect line_dev_rect;
        text_line->GetBBox(&line_rect);
        m_page_view->RectToDevice(&line_rect, &line_dev_rect);

        std::wstring text;
        text.resize(text_line->GetText(nullptr, 0));
        text_line->GetText((wchar_t*)text.c_str(), (int)text.size());
        double line_width = line_dev_rect.right - line_dev_rect.left;
        double line_height = line_dev_rect.bottom - line_dev_rect.top;

        int page_width(0), page_height(0);

        if (parent) {
          line_dev_rect.left -= parent_dev_rect.left;
          line_dev_rect.top -= parent_dev_rect.top;
          line_dev_rect.right -= parent_dev_rect.left;
          line_dev_rect.bottom -= parent_dev_rect.top;
          page_width = parent_dev_rect.right - parent_dev_rect.left;
          page_height = parent_dev_rect.bottom - parent_dev_rect.top;
        }
        else {
          page_width = m_page_view->GetDeviceWidth();
          page_height = m_page_view->GetDeviceHeight();
        }

        double left = line_dev_rect.left / (double)page_width * 100.;
        double top = line_dev_rect.top / (double)page_height * 100.;
        double width2 = (line_dev_rect.right - line_dev_rect.left) / (double)page_width * 100.;
        double height2 = (line_dev_rect.bottom - line_dev_rect.top) / (double)page_height * 100.;

        std::string class_id = "obj_" + GetNewHtmlClassName();
        html_data->css += "." + class_id + " {\n";
        html_data->css += "left: " + std::to_string(left) + "%; \n";
        html_data->css += "top: " + std::to_string(top) + "%; \n";
        html_data->css += "width: " + std::to_string(width2) + "%; \n";
        html_data->css += "height: " + std::to_string(height2) + "%; \n";
        html_data->css += "font-size: " + std::to_string(line_height) + "px;\n";
        html_data->css += "text-align: left;\n";
        html_data->css += "}\n";

        // dreate div element wich holds the class_id 
        html_data->html += "<div id=\"" + std::to_string(text_line->GetId()) + "\" ";
        html_data->html += "class=\"" + class_id + " " + font_family_cls + " pdf-txt-fixed pdf-obj-fixed\">\n";
        html_data->html += "<span ";
        html_data->html += "name=\"fix-text\" >";
        html_data->html += w2utf8(text.c_str()) + "\n";
        html_data->html += "</span>";
        html_data->html += "</div>\n";
      }
    }

    // Responsive layout
    else {
      // text indent
      double indent = text->GetIndent();

      // text alignment
      PdfTextAlignment alignment = text->GetAlignment();
      bool join_lines = true; 
      /* todo
        (alignment == kAlignmentLeftReflow) ||
        (alignment == kAlignmentRightReflow) ||
        (alignment == kAlignmentJustify); */

      auto same_ts = [](auto const& ts1, auto const& ts2) {
        return memcmp(&ts1, &ts2, sizeof(ts2)) == 0;
      };

      PdfTextState ts;
      text->GetTextState(&ts);
      bool first_ts = true;

      html_data->html += "<div >";
      html_data->html += "<p >";

      bool end_with_dot = false;
      // process text characters
      int num_lines = text->GetNumTextLines();
      for (int l = 0; l < num_lines; l++) {
        PdeTextLine* line = text->GetTextLine(l);
        if (!line)
          throw(m_pdfix->GetErrorType());
        int num_words = line->GetNumWords();
        for (int w = 0; w < num_words; w++) {
          PdeWord* word = line->GetWord(w);
          if (!word)
            throw(m_pdfix->GetErrorType());
          int length = word->GetNumChars();
          // read last character
          //std::wstring end_str;
          //end_str.resize(word->GetCharText(length - 1, nullptr, 0));
          //word->GetCharText(length - 1, (wchar_t*)end_str.c_str(), end_str.size());
          //end_with_dot = (end_str == L".") || (end_str == L"!") || (end_str == L"?");
          // iterate through each character
          for (int i = 0; i < length; i++) {
            PdfTextState char_ts;
            word->GetCharTextState(i, &char_ts);
            PdfRect char_bbox;
            word->GetCharBBox(i, &char_bbox);
            char_ts.flags |= GetCharAnnotFlag(char_bbox);
            std::wstring char_str;
            char_str.resize(word->GetCharText(i, nullptr, 0));
            word->GetCharText(i, (wchar_t*)char_str.c_str(), (int)char_str.size());
            if (first_ts || !same_ts(ts, char_ts)) {
              // end previous span 
              if (!first_ts)
                html_data->html += "</span>";
              // start new span 
              memcpy(&ts, &char_ts, sizeof(char_ts));
              std::string class_id = "obj_" + GetNewHtmlClassName();
              html_data->css += GetTextStateCSS(class_id, ts, alignment, indent);
              html_data->html += "<span ";
              html_data->html += "class=\"";
              html_data->html += class_id + " ";
              html_data->html += "pdf-obj\">";
              first_ts = false;
            }
            html_data->html += w2utf8(char_str);
          }
          // add whitespace between words
          if (w != num_words - 1)
            html_data->html += " ";
        }
        // add whitespace between lines
        if (l != num_lines - 1)
          html_data->html += " ";
        if (!join_lines)
          html_data->html += "<br>";
      }

      html_data->html += "</span>";
      html_data->html += "</p>";
      html_data->html += "</div>";
    }
  }
  catch (...) {
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GetImageHtml
////////////////////////////////////////////////////////////////////////////////////////////////////
void PdfHtmlDoc::GetImageHtml(PdeElement* element, PdeElement* parent, 
  PdfHtmlType html_type, PdfHtmlData* html_data) {
  PdeImage* image = (PdeImage*)element;
  try {
    if (html_type == kPdfHtmlResponsive) {
      html_data->html += "<br>";

      PdfRect elem_rect;
      PdfDevRect elem_dev_rect;
      element->GetBBox(&elem_rect);
      m_page_view->RectToDevice(&elem_rect, &elem_dev_rect);
      int elem_width = elem_dev_rect.right - elem_dev_rect.left;
      int elem_height = elem_dev_rect.bottom - elem_dev_rect.top;
      if (elem_height == 0 || elem_width == 0)
        return;

      static int img_counter = 0;
      img_counter++;

      // add image
      std::wstring bg_image = L"img" + std::to_wstring(img_counter) + L".jpg";
      std::wstring bg_image_path = m_settings.m_html_path + m_settings.m_img_dir + bg_image;
      CheckDir(m_settings.m_html_path + m_settings.m_img_dir);
      image->Save(bg_image_path.c_str(), kImageFormatPng, m_page_view);

      std::string class_id = "obj_" + GetNewHtmlClassName();
      html_data->css += "." + class_id + " {\n";
      html_data->css += "text-align: center;";
      html_data->css += "}\n\n";

      std::string pdf_obj_cls = html_type == kPdfHtmlFixed ? "pdf-obj-fixed" : " pdf-obj";
      html_data->html += "<div ";
      html_data->html += "class=\"" + class_id + " " + pdf_obj_cls + "\">\n";
      std::string div_class_id = "obj_" + GetNewHtmlClassName();
      html_data->css += "." + div_class_id + " {\n";
      html_data->css += "display: inline-block;\n";
      html_data->css += "position: relative;\n";
      html_data->css += "}\n\n";
      html_data->html += "<div class=\"" + div_class_id + "\">";

      std::string img_class_id = "obj_" + GetNewHtmlClassName();
      html_data->css += "." + img_class_id + " {\n";
      html_data->css += "width: 100%;\n";
      html_data->css += "position: relative;\n";
      html_data->css += "top: 0px;\n";
      html_data->css += "max-width: " + std::to_string(elem_width) + "px; \n";
      html_data->css += "}\n\n";

      html_data->html += "<img  ";
      html_data->html += "class=\"" + img_class_id + " pdf-img " + pdf_obj_cls + "\" ";
      html_data->html += "src=\"" + w2utf8(m_settings.m_img_dir.c_str()) +
        w2utf8(bg_image.c_str());
      html_data->html += "\">";
    }

    // all child elements should have fixed position
    PdeElement* parent = nullptr;
    if (html_type == kPdfHtmlResponsive)
      parent = element;
    int children_count = element->GetNumChildren();
    for (int i = 0; i < children_count; i++) {
      PdeElement* child = element->GetChild(i);
      if (!child)
        throw(m_pdfix->GetErrorType());
      PdfHtmlData child_html_data;
      if (child->GetType() == kPdeText)
        GetTextHtml(child, parent, kPdfHtmlFixed, html_data);
      else if (child->GetType() == kPdeFormField)
        GetFormFieldHtml(child, parent, kPdfHtmlFixed, html_data);
      html_data->Append(child_html_data);
    }

    if (html_type == kPdfHtmlResponsive) {
      html_data->html += "</div>";
      html_data->html += "</div>";
      html_data->html += "<br>";
    }
  }
  catch (...) {
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GetRectHtml
////////////////////////////////////////////////////////////////////////////////////////////////////
void PdfHtmlDoc::GetRectHtml(PdeElement* element, PdeElement* parent, 
  PdfHtmlType html_type, PdfHtmlData* html_data) {
  try {
    // process children
    int count = element->GetNumChildren();
    if (count == 0)
      return;

    // if there is only image - add image
    if (count == 1) {
      PdeElement* child = element->GetChild(0);
      if (!child)
        throw(m_pdfix->GetErrorType());
      if (child->GetType() == kPdeImage) {
        GetImageHtml(child, element, html_type, html_data);
        return;
      }
    }

    PdeRect* rect = (PdeRect*)element;
    std::string cls_id = "obj_" + GetNewHtmlClassName();
    std::string fill_color_str, stroke_color_str;
    PdfGraphicState gstate;
    rect->GetGraphicState(&gstate);
    if (gstate.color_state.fill_type != kFillTypeNone)
      fill_color_str = Color2HexStr(gstate.color_state.fill_color);
    if (gstate.color_state.stroke_type != kFillTypeNone)
      stroke_color_str = Color2HexStr(gstate.color_state.stroke_color);
    if (html_type == kPdfHtmlResponsive) {
      html_data->html += "<br>";
      html_data->html += "<div class=\"" + cls_id + " pdf-rect\">";
      html_data->css += "." + cls_id + " {\n";
      if (fill_color_str.length() > 0)
        html_data->css += "background-color: " + fill_color_str + ";\n";
      if (stroke_color_str.length() > 0) {
        html_data->css += "border: ";
        html_data->css += std::to_string(gstate.line_width);
        html_data->css += "px solid " + stroke_color_str + " ;";
      }
      // not to extend the rect to the full page width
      html_data->css += "display: inline-block;\n";
      // end not to extend the rect to the full page width
      html_data->css += "}\n";
    }
    
    // process children
    for (int i = 0; i < count; i++) {
      PdeElement* child = element->GetChild(i);
      if (!child)
        throw(m_pdfix->GetErrorType());
      PdfHtmlData child_html_data;
      GetElementHtml(child, element, html_type, &child_html_data);
      html_data->Append(child_html_data);
    }

    if (html_type == kPdfHtmlResponsive)
      html_data->html += "</div><br>";
  }
  catch (...) {
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GetTableHtml
////////////////////////////////////////////////////////////////////////////////////////////////////
void PdfHtmlDoc::GetTableHtml(PdeElement* element, PdeElement* parent
  , PdfHtmlType html_type, PdfHtmlData* html_data) {
  try {
    PdeTable* table = (PdeTable*)element;
    if (html_type == kPdfHtmlResponsive) {
      std::string fill_color_str, stroke_color_str;
      PdfGraphicState gstate;
      table->GetGraphicState(&gstate);
      if (gstate.color_state.fill_type != kFillTypeNone)
        fill_color_str = Color2HexStr(gstate.color_state.fill_color);
      if (gstate.color_state.stroke_type != kFillTypeNone)
        stroke_color_str = Color2HexStr(gstate.color_state.stroke_color);
    }
    int row_count = table->GetNumRows();
    int col_count = table->GetNumCols();
    if (html_type == kPdfHtmlResponsive) {
      html_data->html += "<style>\n";
      html_data->html += "table, th, td{\n";
      html_data->html += "border: 1px solid black;\n";
      html_data->html += "border-collapse: collapse;\n";
      html_data->html += "}\n";
      html_data->html += "</style>\n";
      html_data->html += "<table border=\"1\" style=\"width:100%\">\n";
      PdfRect table_bbox;
      table->GetBBox(&table_bbox);
      double table_width = table_bbox.right - table_bbox.left;
      for (int col = 0; col < col_count; col++) {
        PdeCell* cell = (PdeCell*)table->GetCell(0, col);
        if (!cell) {
          continue;
        }
        PdfRect bbox;
        cell->GetBBox(&bbox);
        int percent = (int)(((bbox.right - bbox.left) / table_width) * 100);
        html_data->html += "<col style=\"width:";
        html_data->html += std::to_string(percent);
        html_data->html += "%\">\n";
      }
    }

    // process table cells
    for (int row = 0; row < row_count; row++) {
      if (html_type == kPdfHtmlResponsive)
        html_data->html += "<tr>\n";
      for (int col = 0; col < col_count; col++) {
        PdeCell* cell = (PdeCell*)table->GetCell(row, col);
        if (!cell)
          throw(m_pdfix->GetErrorType());
        if (html_type == kPdfHtmlResponsive) {
          int row_span = cell->GetRowSpan();
          int col_span = cell->GetColSpan();
          if (row_span == 0 || col_span == 0)
            continue;
          html_data->html += "<td ";
          html_data->html += "colspan = \"" + std::to_string(col_span) + "\" ";
          html_data->html += "rowspan = \"" + std::to_string(row_span) + "\" ";
          html_data->html += ">";
        }
        // process cell's children
        int children_count = cell->GetNumChildren();
        if (children_count > 0) {
          for (int i = 0; i < children_count; i++) {
            PdeElement* child = cell->GetChild(i);
            if (!child)
              throw(m_pdfix->GetErrorType());
            PdfHtmlData child_html_data;
            GetElementHtml(child, cell, html_type, &child_html_data);
            html_data->Append(child_html_data);
          }
        }
        if (html_type == kPdfHtmlResponsive)
          html_data->html += "</td>";
      }
      if (html_type == kPdfHtmlResponsive)
        html_data->html += "</tr>";
    }
    if (html_type == kPdfHtmlResponsive) {
      html_data->html += "</table>";
      html_data->html += "<br>";
    }
  }
  catch (...) {
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GetElementHtml
////////////////////////////////////////////////////////////////////////////////////////////////////
void PdfHtmlDoc::GetElementHtml(PdeElement* element, PdeElement* parent, 
  PdfHtmlType html_type, PdfHtmlData* html_data) {
  try {
    PdfElementType elem_type = element->GetType();
    switch (elem_type) {
    case kPdeText:
      GetTextHtml(element, parent, html_type, html_data);
      break;
    case kPdeTable:
      GetTableHtml(element, parent, html_type, html_data);
      break;
    case kPdeRect:
      GetRectHtml(element, parent, html_type, html_data);
      break;
    case kPdeImage:
      GetImageHtml(element, parent, html_type, html_data);
      break;
    case kPdeFormField:
      GetFormFieldHtml(element, parent, html_type, html_data);
      break;
    case kPdeLine: {
      // add only horizontal lines
      PdeLine* line = (PdeLine*)element;
      PdfRect bbox;
      line->GetBBox(&bbox);
      double width = fabs(bbox.right - bbox.left);
      double height = fabs(bbox.top - bbox.bottom);
      if (html_type == kPdfHtmlResponsive && (width > height))
        html_data->html += "<hr>";
      break;
    }
    default: {
      if (html_type == kPdfHtmlResponsive)
        html_data->html += "<span class=\"dev-note\">" + std::to_string(elem_type) + "</span>";
      break;
    }
    }
  }
  catch (...) {
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GetPageHtml
////////////////////////////////////////////////////////////////////////////////////////////////////
void PdfHtmlDoc::GetPageHtml(PdfHtmlType html_type, PdfHtmlData* html_data) {
  try {
    m_page = m_doc->AcquirePage(m_page_num);
    if (!m_page)
      throw(m_pdfix->GetErrorType());

    // expected page width is 800
    int html_width = 1000;
    PdfRect crop_box;
    m_page->GetCropBox(&crop_box);
    double page_width = (crop_box.right - crop_box.left);
    double zoom = html_width / page_width;

    // render page
    m_page_view = m_page->AcquirePageView(zoom, kRotate0);
    if (!m_page_view)
      throw(m_pdfix->GetErrorType());
    PdfPageRenderParams params;
    m_page_view->DrawPage(&params, nullptr, nullptr);
    m_page_image = m_page_view->GetImage();
    if (!m_page_image)
      throw(m_pdfix->GetErrorType());

    html_data->css += ".pdf-page-" + std::to_string(m_page_num) + " {\n";
    // create background image for fixed layout
    if (html_type == kPdfHtmlFixed) {
      html_data->css += "width: 100%;\n";
      html_data->css += "height: " + std::to_string(m_page_view->GetDeviceHeight()) + "px; \n";
      std::wstring bg_image = L"page" + std::to_wstring(m_page_num) + L".png";
      std::wstring bg_image_path = m_settings.m_html_path +
        m_settings.m_img_dir + bg_image;
      CheckDir(m_settings.m_html_path + m_settings.m_img_dir);
      m_page_image->Save(bg_image_path.c_str(), kImageFormatPng);
      html_data->css += "background-image: url(\"" +
        w2utf8(m_settings.m_img_dir.c_str()) +
        w2utf8(bg_image.c_str()) +
        "\");\n";
    }
    html_data->css += "}\n";
    html_data->html += "<div id=\"page-" + std::to_string(m_page_num) + "\" " +
      "class=\"pdf-page pdf-page-" + std::to_string(m_page_num) + "\" " +
      "type=\"page\" " +
      "number=\"" + std::to_string(m_page_num) + "\" " +
      "r=\"" + std::to_string((double)m_page_view->GetDeviceHeight() / 
        (double)m_page_view->GetDeviceWidth()) + "\" " +
      "name=\"pdf-page\" " +
      ">\n";

    PdfPageMapParams pm_params;
    PdePageMap* page_map = m_page->AcquirePageMap(&pm_params, nullptr, nullptr);
    if (!page_map)
      throw(m_pdfix->GetErrorType());

    PdfHtmlSettings settings2(m_settings);

    int count = page_map->GetNumElements();
    PdeElement* prev_elem = nullptr;
    for (int i = 0; i < count; i++) {
      PdeElement* element = page_map->GetElement(i);
      if (!element)
        throw(m_pdfix->GetErrorType());
      PdfHtmlData element_html_data;
      GetElementHtml(element, nullptr, html_type, &element_html_data);
      html_data->Append(element_html_data);
    }
    html_data->html += "</div>\n";
  }
  catch (...) {
    std::wcout << L"error" << std::endl;
  }
  if (m_page)
    m_doc->ReleasePage(m_page);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GetHtml
////////////////////////////////////////////////////////////////////////////////////////////////////
void PdfHtmlDoc::GetHtml(PdfHtmlSettings& settings, PdfHtmlType html_type) {
  try {
    m_settings = settings;

    std::wstring js_file = L"pdfix_doc.js";
    std::wstring doc_title;

    doc_title.resize(m_doc->GetInfo(L"Title", nullptr, 0));
    m_doc->GetInfo(L"Title", (wchar_t*)doc_title.c_str(), (int)doc_title.size());

    m_html_data.html += "<!DOCTYPE html> \n";
    m_html_data.html += "<head>\n";
    m_html_data.html += "<meta charset=\"UTF-8\">\n";
    m_html_data.html += "<title>" + w2utf8(doc_title.c_str()) + "</title>\n";
    m_html_data.html += "<link rel=\"stylesheet\" type=\"text/css\" href=\"global.css\">\n";
    m_html_data.html += "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\n";

    // js_api
    m_html_data.html += "<script src=\"" + w2utf8(settings.m_js_dir) + "jquery.min.js\"></script>\n";
    if (settings.m_acroform) {
      m_html_data.html += "<script src=\"" + w2utf8(settings.m_js_dir) + "pdfix_js_api.js\"></script>\n";
      m_html_data.html += "<script src=\"" + w2utf8(settings.m_js_dir) + "pdfix_events.js\"></script>\n";
      m_html_data.html += "<script src=\"" + w2utf8(settings.m_js_dir) + "pdfix_af.js\"></script>\n";
    }
    m_html_data.html += "<script src=\"" + w2utf8(settings.m_js_dir) +
      w2utf8(js_file.c_str()) + "\"></script>\n";
    m_html_data.html += "<script src=\"" + w2utf8(settings.m_js_dir) + "pdfix_content.js\"></script>\n";
    m_html_data.html += "</head>\n";

    // html body
    m_html_data.html += "<body onload=\"doc_did_load();\">\n";
    m_html_data.html += "<form id=\"pdf-form\">\n";
    m_html_data.html += "<div class=\"pdf-document\" id=\"pdf-document\" ";
    m_html_data.html += "layout=\"";
    if (html_type == kPdfHtmlFixed)
      m_html_data.html += "fixed";
    if (html_type == kPdfHtmlResponsive)
      m_html_data.html += "responsive";
    m_html_data.html += "\"";
    m_html_data.html += ">\n";

    if (settings.m_acroform) {
      // add document JavaScript
      for (int i = 0; i < m_doc->GetNumDocumentJavaScripts(); i++) {
        std::wstring w_js;
        w_js.resize(m_doc->GetDocumentJavaScript(i, nullptr, 0));
        m_doc->GetDocumentJavaScript(i, (wchar_t*)w_js.c_str(), (int)w_js.size());
        FixJS(w_js);
        m_html_data.js += w2utf8(w_js.c_str()) + "\n\n";
      }

      // store all document field names
      int field_count = m_doc->GetNumFormFields();
      for (int i = 0; i < field_count; i++) {
        PdfFormField* field = m_doc->GetFormField(i);
        std::wstring field_name;
        field_name.resize(field->GetFullName(NULL, 0));
        field->GetFullName((wchar_t*)field_name.c_str(), (int)field_name.size());
        m_html_data.js += "all_fields.push([\"" +
          w2utf8(field_name.c_str()) + "\", " +
          std::to_string((int)field) + ", []]);\n";
      }
      m_html_data.js += "\n\n";

      // push all calculated fields into calc_fields array
      int calc_field_count = m_doc->GetNumCalculatedFormFields();
      for (int i = 0; i < calc_field_count; i++) {
        PdfFormField* field = m_doc->GetCalculatedFormField(i);
        std::wstring field_name;
        field_name.resize(field->GetFullName(nullptr, 0));
        field->GetFullName((wchar_t*)field_name.c_str(), (int)field_name.size());
        m_html_data.js += "calc_fields.push(\"" + w2utf8(field_name.c_str()) + "\");\n";
        //get the calculation script
        std::wstring calc_js;
        PdfAction* action_c = field->GetAAction(kActionEventFieldCalculate);
        if (action_c) {
          calc_js.resize(action_c->GetJavaScript(nullptr, 0));
          action_c->GetJavaScript((wchar_t*)calc_js.c_str(), (int)calc_js.size());
        }
        if (calc_js.length()) {
          m_html_data.js += "function C" + std::to_string((int)field) + "() {\n";
          m_html_data.js += w2utf8(calc_js.c_str());
          m_html_data.js += "\n}\n\n";
        }
      }
    }

    CheckDir(settings.m_html_path);

    // save core javascripts and css
    SaveFileFromResource(IDR_JQUERY, settings.m_html_path + settings.m_js_dir + L"jquery.min.js");
    if (settings.m_acroform) {
      SaveFileFromResource(IDR_AFSCRIPT_JS, settings.m_html_path + settings.m_js_dir + L"pdfix_af.js");
      SaveFileFromResource(IDR_API_JS, settings.m_html_path + settings.m_js_dir + L"pdfix_js_api.js");
      SaveFileFromResource(IDR_EVENTS_JS, settings.m_html_path + settings.m_js_dir + L"pdfix_events.js");
    }
    if (html_type == kPdfHtmlFixed) {
      SaveFileFromResource(IDR_CONTENT_JS, settings.m_html_path + settings.m_js_dir + L"pdfix_content.js");
      SaveFileFromResource(IDR_GLOBAL_FIXED_CSS, settings.m_html_path + L"global.css");
    }
    else {
      SaveFileFromResource(IDR_CONTENT_JS, settings.m_html_path + settings.m_js_dir + L"pdfix_content.js");
      SaveFileFromResource(IDR_GLOBAL_RESPONSIVE_CSS, settings.m_html_path + L"global.css");
    }

    // process all pages
    for (m_page_num = 0; m_page_num < m_doc->GetNumPages(); m_page_num++) {
      if (html_type == kPdfHtmlFixed && m_page_num > 0)
        m_html_data.html += "<br><br>\n";

      std::cout << "Pdfix HTML page: " << m_page_num << std::endl;

      PdfHtmlData page_html_data;
      GetPageHtml(html_type, &page_html_data);
      m_html_data.Append(page_html_data);

      SaveHtml();
    }
    m_html_data.html += "</div>\n";	  //pdf-document
    m_html_data.html += "</form>\n";	//pdf-form
    m_html_data.html += "</body>\n";
    m_html_data.html += "</html>\n";

    SaveHtml();
  }
  catch (...) {
    std::cout << "error" << std::endl;
  }
}

void PdfHtmlDoc::SaveHtml() {
  SaveFileFromString(m_html_data.html, m_settings.m_html_path + L"index.html");
  SaveFileFromString(m_html_data.css, m_settings.m_html_path + L"style.css");
  if (m_html_data.js.length() > 0) {
    std::wstring script_js = m_settings.m_html_path + m_settings.m_js_dir + L"pdfix_doc.js";
    SaveFileFromString(m_html_data.js, script_js);
  }
}