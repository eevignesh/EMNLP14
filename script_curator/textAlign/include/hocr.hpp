/*
  Adapted from the hocr code from Rene Rebe available for PDF parsing
  The License from original distribution is below
*/


/*
 * The ExactImage library's hOCR to PDF parser
 * Copyright (C) 2008-2009 René Rebe, ExactCODE GmbH Germany
 * Copyright (C) 2008 Archivista
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2. A copy of the GNU General
 * Public License can be found in the file LICENSE.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT-
 * ABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * Alternatively, commercial licensing options are available from the
 * copyright holder ExactCODE GmbH Germany.
 */

#ifndef __HOCR_HPP__
#define __HOCR_HPP__

#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <cctype>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;

/*
 * @brief: stores the line extracted from a hocr output
 *
 * @members:
 * _line - the line extracted in raster scan format
 * x_cord - the begin and end x-coordinate of the line
 *
 * */

typedef struct _HocrLine
{
  string line;
  pair<int, int> x_cord;
  pair<int, int> y_cord;
}HocrLine;



static int res = 300;
static bool sloppy = false;
static std::string txtString;
extern vector <HocrLine> hLines;

static inline std::string lowercaseStr(const std::string& _s)
{
  std::string s(_s);
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  return s;
}



// custom copy to trip newlines, likewise
static inline bool isMyBlank(char c)
{
  switch (c) {
  case ' ':
  case '\t':
  case '\r':
  case '\n':
    return true;
    break;
  default: return false;
  }
}

static inline std::string peelWhitespaceStr(const std::string& _s)
{
  std::string s(_s);
  // trailing whitespace
  for (int i = s.size() - 1; i >= 0 && isMyBlank(s[i]); --i)
    s.erase(i, 1);
  // leading whitespace
  while (!s.empty() && isMyBlank(s[0]))
    s.erase(0, 1);
  return s;
}

// lower-case, and strip leading/trailing white-space
static inline std::string sanitizeStr(const std::string& _s)
{
  return peelWhitespaceStr(lowercaseStr(_s));
}

// HTML decode

static inline std::string htmlDecode(const std::string& _s)
{
  std::string s(_s);
  std::string::size_type i;
  
  while ((i = s.find("&amp;")) != std::string::npos)
    s.replace(i, 5, "&");

  while ((i = s.find("&lt;")) != std::string::npos)
    s.replace(i, 4, "<");
  
  while ((i = s.find("&gt;")) != std::string::npos)
    s.replace(i, 4, ">");

  while ((i = s.find("&quot;")) != std::string::npos)
    s.replace(i, 6, "\"");
  
  // TODO: '&8212;' and more - when implemented, best locked on
  // each '&' and matched to the next ';'
  return s;
}


// state per char: bbox, bold, italic, boldItalic
// state per line: bbox, align: left, right justified

static struct BBox {
  BBox()
    : x1(0), y1(0), x2(0), y2(0)
  {}
  
  bool operator== (const BBox& other)
  {
    return
      x1 == other.x1 &&
      y1 == other.y1 &&
      x2 == other.x2 &&
      y2 == other.y2;
  }
  
  double x1, y1, x2, y2;
} lastBBox;

static std::ostream& operator<< (std::ostream& s, const BBox& b)
{
  s << b.x1 << ", " << b.y1 << ", " << b.x2 << ", " << b.y2;
  return s;
}

static enum Style {
  None    = 0,
  Bold    = 1,
  Italic  = 2,
  BoldItalic = (Bold | Italic)
} lastStyle;


// TODO: implement parsing, if of any guidance for the PDF
static enum Align {
  Left    = 0,
  Right   = 1,
  Justify = 2,
} lastAlign;

struct Span {
  BBox bbox;
  Style style;
  std::string text;
};

static struct Textline {
  std::vector<Span> spans;
  typedef std::vector<Span>::iterator span_iterator;
  
  void draw()
  {
    int min_xcord = 10000;
    int max_xcord = 0;

    int min_ycord = 100000;
    int max_ycord = 0;

    string newText = "";

    double y1 = 0, y2 = 0, yavg = 0;
    int n = 0;
    for (span_iterator it = spans.begin(); it != spans.end(); ++it, ++n)
      {
	if (it == spans.begin()) {
	  y1 = it->bbox.y1;
	  yavg = y2 = it->bbox.y2;
	} else {
	  if (it->bbox.y1 < y1)
	    y1 = it->bbox.y1;
	  if (it->bbox.y2 > y2)
	    y2 = it->bbox.y2;
	  yavg += it->bbox.y2;
	}
      }
    if (n > 0)
      yavg /= n;
    
    int height = (int)round(std::abs(y2 - y1) * 72. / res);
    if (height < 8) // TODO: allow configuration?
      height = 8;
    
    //std::cerr << "drawing with height: " << height << std::endl;
    
    // remove trailing whitespace
    for (span_iterator it = spans.end(); it != spans.begin(); --it)
      {
	span_iterator it2 = it; --it2;
	for (int i = it2->text.size() - 1; i >= 0; --i) {
	  if (isMyBlank(it2->text[i]))
	    it2->text.erase(i);
	  else
	    goto whitespace_cleaned;
	}
      }
  
  whitespace_cleaned:
    
    for (span_iterator it = spans.begin(); it != spans.end(); ++it, ++n)
      {
	// escape decoding, TODO: maybe change our SAX parser to emmit a single
	// text element, and thus decode it earlier
	std::string text = htmlDecode(it->text);
	BBox bbox = it->bbox;
	
	// one might imprecicely place text sloppily in favour of "sometimes"
	// improved cut'n paste-able text in not so advanced PDF Viewers
	if (sloppy) {
	  span_iterator it2 = it;
	  for (++it2; it2 != spans.end(); ++it2)
	    {
	      if (it->style != it2->style)
		break;
	      
	      std::string nextText = htmlDecode(it2->text);
	      
	      // TODO: in theory expand bbox, if later needed
	      text += nextText;
	      
	      // stop on whitespaces to sync on gaps in justified text
	      if (nextText != peelWhitespaceStr(nextText)) {
		++it2; // we consumed the glyph, so proceeed
		break;
	      }
	    }
	  it = --it2;
	}
	
	const char* font = "Helvetica";
	switch (it->style) {
	case Bold: 
	  font = "Helvetica-Bold"; break;
	case Italic:
	  font = "Helvetica-Oblique"; break;
	case BoldItalic:
	  font = "Helvetica-BoldOblique"; break;
	default:
	  ; // already initialized
	}
	
	//std::cerr << "(" << text << ") ";
	//pdfContext->textTo(72. * bbox.x1 / res, 72. * yavg / res);
	//pdfContext->showText(font, text, height);
	//cout << "(" << bbox.y1 << "," <<  bbox.x1 << ":" << bbox.x2 << ") : " << text << endl;

  if(bbox.x2 >= max_xcord)
    max_xcord = bbox.x2;
  if(bbox.x1 <= min_xcord)
    min_xcord = bbox.x1;
 
  if(bbox.y2 >= max_ycord)
    max_ycord = bbox.y2;
  if(bbox.y1 <= min_ycord)
    min_ycord = bbox.y1;
  //cout << "(min: " << min_ycord << ", max: " << max_ycord << ")" << endl;

  // Hack to replace ascii value corresponding to single quote with "\'" 
  size_t pos_dash = text.find("&#39;");
  if(pos_dash != string::npos)
  {
    text.replace(pos_dash, string("&#39;").length(), "\'");
  }
  txtString += text;
  newText += text;
  }
  txtString += "\n";
  //xCordinate = make_pair(min_xcord, max_xcord);
  //
 
    HocrLine hLine;
    hLine.line = newText;
    hLine.x_cord = make_pair(min_xcord, max_xcord);
    hLine.y_cord = make_pair(min_ycord, max_ycord);
    hLines.push_back(hLine);
    //cout << "(" << min_xcord << "," << max_xcord << ") : " << newText << " ---------------------------------------------------- " << endl;
    //std::cerr << std::endl;
  }
  
  void flush()
  {
    if (!spans.empty())
      draw();
    spans.clear();
  }


  void push_back(Span s)
  {
    //std::cerr << "push_back (" << s.text << ") " << s.style << std::endl;
    
    // do not insert newline garbage (empty string after white-
    // space peeling) at the beginning of a line
    if (spans.empty()) {
      s.text = peelWhitespaceStr(s.text);
      if (s.text.empty())
	return;
    }
    
    // if the direction wrapps, assume new line
    if (!spans.empty() && s.bbox.x1 < spans.back().bbox.x1)
      flush();
    
    // unify inserted spans with same properties, for now to
    // not draw them at the same position, but one text operator
    if (!spans.empty() &&
	(spans.back().bbox == s.bbox) &&
	(spans.back().style == s.style))
      spans.back().text += s.text;
    else
      spans.push_back(s);
  }
  
} textline;


static BBox parseBBox(std::string s)
{
  BBox b; // self initialized to zero
  
  const char* tS = "title=\"";
  
  std::string::size_type i = s.find(tS);
  if (i == std::string::npos)
    return b;
  
  std::string::size_type i2 = s.find("\"", i + strlen(tS));
  if (i2 == std::string::npos)
    return b;
  
  std::stringstream stream(s.substr(i + strlen(tS), i2 - i - strlen(tS)));
  std::string dummy;
  stream >> dummy >> b.x1 >> b.y1 >> b.x2 >> b.y2;

  return b;
}

static void elementStart(const std::string& _name, const std::string& _attr = "")
{
  std::string name(sanitizeStr(_name)), attr(sanitizeStr(_attr));
  
  //std::cerr << "elementStart: '" << name << "', attr: '" << attr << "'" << std::endl;
  
  BBox b = parseBBox(attr);
  if (b.x2 > 0 && b.y2 > 0)
    lastBBox = b;
  
  if (name == "b" || name == "strong")
    lastStyle = Style(lastStyle | Bold);
  else if (name == "i" || name == "em")
    lastStyle = Style(lastStyle | Italic);
  
}

static void elementText(const std::string& text)
{
  //std::cerr << "elementText: \"" << text << "\"" << std::endl;
  Span s;
  s.bbox = lastBBox;
  s.style = lastStyle;
  s.text += text;
  
  textline.push_back(s);
}

static void elementEnd(const std::string& _name)
{
  std::string name (sanitizeStr(_name));
  
  //std::cerr << "elementEnd: " << name << std::endl;
  
  if (name == "b" || name == "strong")
    lastStyle = Style(lastStyle & ~Bold);
  else if (name == "i" || name == "em")
    lastStyle = Style(lastStyle & ~Italic);
  
  // explicitly flush line of text on manual preak or end of paragraph
  else if (name == "br" || name == "p")
    textline.flush();
}


// returns the string before the first whitespace
static std::string tagName(std::string t)
{
  std::string::size_type i = t.find(' ');
  if (i != std::string::npos)
    t.erase(i);
  return t;
}

static bool hocr2pdf(std::istream& hocrStream)
{
  // TODO: soft hyphens
  // TODO: better text placement, using one TJ with spacings
  // TODO: more image compressions, jbig2, Fax
  
  // TODO: do not use global vars, use a state container
  //::pdfContext = pdfContext;
  //::res = res;
  //::sloppy = sloppy;
  //::txtStream = txtStream;
  
  //pdfContext->beginText();
  
  // minimal, cuneiform HTML ouptut parser
  char c;
  std::vector<std::string> openTags;  
  std::string* curTag = 0;
  std::string closingTag;
  while (hocrStream.get(c), hocrStream.good()) {
    // consume tag element text
    if (curTag && c != '>') {
      *curTag += c;
      continue;
    }
    
    switch (c) {
    case '<':
      if (hocrStream.peek() != '/') {
	openTags.push_back("");
	curTag = &openTags.back();
      } else {
	closingTag.clear();
	curTag = &closingTag;
      }
      break;
    case '>':
      if (curTag != &closingTag) {
	bool closed = false;
	if (!curTag->empty() && curTag->at(curTag->size() - 1) == '/')
	  {
	    curTag->erase(curTag->size() - 1);
	    closed = true;
	  }
	
	// HTML asymetric tags, TODO: more of those (and !DOCTYPE)?
	// TODO: maybe specially treat meta & co?
	{
	  std::string lowTag = lowercaseStr(tagName(*curTag));
	  if (lowTag == "br" || lowTag == "img" || lowTag == "meta")
	    closed = true;
	}
	
	//std::cout << "tag start: " << openTags.back()
	//          << (closed ? " immediately closed" : "") << std::endl;
	{
	  std::string element = tagName(*curTag);
	  std::string attr = *curTag;
	  attr.erase(0, element.size());
	  elementStart(element, attr);
	}
	
	if (closed) {
	  elementEnd(*curTag);
	  openTags.pop_back();
	}
      }
      else {
	// garuanteed to begin with a /, remove it
	curTag->erase(0, 1);
	// get just the tag name from the stack
	std::string lastOpenTag = (openTags.empty() ? "" : openTags.back());
	lastOpenTag = tagName(lastOpenTag);
	if (lastOpenTag != *curTag) {
	  std::cout << "Warning: tag mismatch: '" << *curTag
		    << "' can not close last open: '"
		    << lastOpenTag
		    << "'" << std::endl;
	}
	else
	  openTags.pop_back();
	elementEnd(*curTag);
      }
      curTag = 0;
      break;
      
    default:
      elementText(std::string(1, c));
      break;
    }
  }
  
  while (!openTags.empty()) {
    std::string tag = tagName(openTags.back()); openTags.pop_back();
    // skip special tags such as !DOCTYPE
    if (tag.empty() || tag[0] != '!')
      std::cerr << "Warning: unclosed tag: '" << tag << "'" << std::endl;
  }
  
  textline.flush();
  
 // pdfContext->endText();

  //if (txtStream) {
    // for now hypenation compensator, later to be inserted to the
    // generic code-flow to detect and write out soft-hypens on-the-go
    
    // regex: ([a-z])-\n([a-z]) -> \1\2
    // + insert \n at next space of next line
    for (std::string::iterator it = txtString.begin();
	 it != txtString.end();)
      {
	if ((*it == '\n') && // lock on newlines
	    (it != txtString.begin() && it[-1] == '-') && // hyphen in front
	    (it != txtString.end() - 1 && islower(it[+1])) // and the next is lower case
	    )
	  {
	    it = txtString.erase(it -1, it + 1); // erase "\n-"
	    
	    // so, newline removed, insert a break at the next word, same line
	    for (; it != txtString.end() && *it != '\n'; ++it) {
	      if (*it == ' ') {
		*it = '\n';
		++it;
		break;
	      }
	    }
	  }
	else
	  ++it;
      }
    
    //HocrLine ho; 
    //ho.x_cord = xCordinate; ho.line = txtString;
    //hocrOut.push_back(ho);
    //cout << ho.line << " ************************************ " << endl;
    //*txtStream << txtString;
  //}
   return true; // or error
}


/*  @brief: Returns a list of html files (except the ones that begin with a dot)
 *
 *  @paramters:
 *  out - list containg the files in the directory
 *  directory - name of the directory
 *  
 * */
static void GetDirectories(std::vector<string> &out, const string &directory)
{

  DIR *dir;
  dirent *ent;
  class stat st;

  dir = opendir(directory.c_str());
  while ((ent = readdir(dir)) != NULL)
  {
    const string file_name = ent->d_name;
    const string full_file_name = directory + "/" + file_name;

    if (file_name[0] == '.')
      continue;

    if (stat(full_file_name.c_str(), &st) == -1)
      continue;

    const bool is_directory = (st.st_mode & S_IFDIR) != 0;
    
    if(full_file_name.size() < 5)
      continue;

    string strExtension = full_file_name.substr(full_file_name.size() - 4, 4);
    if (is_directory || strExtension != "html" || full_file_name.find("tidy.html") == string::npos)
      continue;

    out.push_back(full_file_name);
  }
  closedir(dir);
}


/*
 *  @brief: parses the hocr files in a folder to get lines and positions
 *
 *  @paramters: 
 *  hocrDirname - name of the directory containing the hocr files
 *  allHocrLines - contains all the hocr lines extracted from the directory
 * */

static void hocrParser(string hocrDirname, vector <HocrLine> &allHocrLines)
{

  //vector <HocrLine> allHocrLines;
  vector <string> hocrFiles;
  GetDirectories(hocrFiles, hocrDirname);  

  for (int i = 1; i <= hocrFiles.size(); i++)
  {
    cout << "processsing " << i << "/" << hocrFiles.size() << " ..." << endl;
    char pageName[50];
    sprintf(pageName, "page_%03d_hocr_tidy.html", i);
    string hocrFilename = hocrDirname + string("/") + string(pageName);
    ifstream hocrfile;
    hocrfile.open (hocrFilename.c_str());
    if(!hocrfile)
    {
      cout<< "Cannot open "<< hocrFilename <<": Not such a file"<<endl;
      exit(EXIT_FAILURE);
    }

    // flush the hLines stream
    hLines.clear();
    
    cout << "Starting to parse hocrfile ... ";
    hocr2pdf(hocrfile);
    cout << "done";
    hocrfile.close();
    
        // insert hocr lines from current page into the allHocrLines
        // skipping the first line
        //
    int ctr = 0;
    if (hLines.size() > 1)
    {
      while (((hLines[ctr+1].y_cord.second - hLines[ctr].y_cord.second) < 50) && (ctr < hLines.size()))
      {
        //cout << "Words header: " << hLines[ctr].line << " == " << hLines[ctr].y_cord.second << endl;
        ctr++;
      }
      ctr++;
    }
    //cout << "next line:" << hLines[ctr].y_cord.second << endl;
    //cout << "The first line is: " << hLines[ctr].line << endl;

    allHocrLines.insert(allHocrLines.end(), hLines.begin()+ctr, hLines.end());
  }

  cout << " =============================================================== " << endl;
  for (int i = 0; i < allHocrLines.size(); i++)
  {
    cout << "(" << allHocrLines[i].x_cord.first << "," << allHocrLines[i].x_cord.second << ")" <<  allHocrLines[i].line << endl;
  }

}

#endif
