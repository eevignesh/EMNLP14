#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <vector>
#include <string>
#include <iostream>
#include <boost/algorithm/string/replace.hpp>

using namespace std;

static string removeHtmlTags(string html)
{
  //std::string html("<div style=\"width: 200px;\"><strong>Balance Sheets (USD $)<br></strong></div>");
  
  //std::string html(argv[1]);

  std::vector<std::string>    tags;
  std::vector<std::string>    text;

  for(;;)
  {
      std::string::size_type  startpos;

      startpos = html.find('<');
      if(startpos == std::string::npos)
      {
          // no tags left only text!
          text.push_back(html);
          break;
      }

      // handle the text before the tag    
      if(0 != startpos)
      {
          text.push_back(html.substr(0, startpos));
          html = html.substr(startpos, html.size() - startpos);
          startpos = 0;
      }

      //  skip all the text in the html tag
      std::string::size_type endpos;
      for(endpos = startpos;
          endpos < html.size() && html[endpos] != '>';
          ++endpos)
      {
          // since '>' can appear inside of an attribute string we need
          // to make sure we process it properly.
          if(html[endpos] == '"')
          {
              endpos++;
              while(endpos < html.size() && html[endpos] != '"')
              {
                  endpos++;
              }
          }
      }

      //  Handle text and end of html that has beginning of tag but not the end
      if(endpos == html.size())
      {
          html = html.substr(endpos, html.size() - endpos);
          break;
      }
      else
      {
          //  handle the entire tag
          endpos++;
          tags.push_back(html.substr(startpos, endpos - startpos));
          html = html.substr(endpos, html.size() - endpos);
      }
  }

  //std::cout << "tags:\n-----------------" << std::endl;

  // auto, iterators or range based for loop would probably be better but
  // this makes it a bit easier to read.    
  /*
  for(size_t i = 0; i < tags.size(); i++)
  {
      std::cout << tags[i] << std::endl;
  }

  std::cout << "\ntext:\n-----------------" << std::endl;*/

  string outString = "";
  for(size_t i = 0; i < text.size(); i++)
  {
      //std::cout << text[i] << std::endl;
      outString += string(text[i]);
  }
  boost::replace_all(outString, "\n", " ");
  return outString;
}


/*
  @brief: find and replace first occurrence of a substring in a string

  @parameter:
  s - stirng in which find and replace is carried out
  toReplace - substring to replace
  replaceWith - string to replace with

  @output:
  myreplce - string where toReplace is replaced with replaceWith

*/
static std::string myreplace(std::string &s,
                      std::string toReplace,
                      std::string replaceWith)
{
    return(s.replace(s.find(toReplace), toReplace.length(), replaceWith));
}

/*
 *  @brief: heuristic algo to check for the top 3 pdf x-indices which are most frequent
 *          string indices for setences in the pdf and are aldo separated by atleadt 
 *          100 pixels
 *
 *  @paramters:
 *      startInex - vector of staring indices for all sentences in a pdf
 *
 *  @output:
 *      a vecotr of 3 indices sorted in acending order. The lowest one corresponds to 
 *      descriptions, the second one to dialogues and the third one to speaker
 *
 * */

static vector<int> getMinLimits(vector<int> startIndex)
{
  int maxval = 0;
  for(int i = 0; i < startIndex.size(); i++)
  {
    maxval = (startIndex[i] > maxval) ? startIndex[i] : maxval;
  }

  // initialize histogram vector
  int *hist = new int[maxval];
  for(int i = 0; i < maxval; i++)
  {
    hist[i] = 0;
  }

  // increment values in a +-10 range
  for(int i = 0; i < startIndex.size(); i++)
  {
    
    //cout << "start-index at " << i << " : " << startIndex[i] << endl;

    int beg_ind = ( (startIndex[i]-10)>0) ?(startIndex[i]-10):0;
    int end_ind = ( (startIndex[i]+10)<maxval) ? (startIndex[i] +10):(maxval-1);

    for(int j = beg_ind; j <= end_ind; j++)
    {
      hist[j]++;
    }

  }

  int top3freq[] = {0,0,0};
  int top3ind[] = {0,0,0};

  for(int i = 0; i < maxval; i++)
  {
    cout << i << " : " << hist[i] << endl;
    if(hist[i] > top3freq[2])
    {

      /*
       *  bad way of checking if the new hist index is atleast 50 pixels away from
       *  the existing indices. -1 indicates that it is not close to any.
       *
       * */
      int checkVal = -1;
      for(int j = 0; j < 3; j++)
      {
        if (i >= (top3ind[j] - 50) && i <= (top3ind[j] + 50))
        {
          checkVal = j;
          break;
        }
      }


      if(hist[i] > top3freq[0])
      {
        // if it is the largest, insert at top
        if(checkVal == -1 || checkVal == 2)
        {
          top3freq[1] = top3freq[0]; top3freq[2] = top3freq[1];
          top3ind[1] = top3ind[0]; top3ind[2] = top3ind[1];
          top3freq[0] = hist[i]; top3ind[0] = i;
        }
        else if(checkVal == 1)
        {
          top3freq[1] = top3freq[0];
          top3ind[1] = top3ind[0];
          top3freq[0] = hist[i]; top3ind[0] = i;
        }
        else
        {
          top3freq[0] = hist[i]; top3ind[0] = i;
        }
      }
      else if(hist[i] > top3freq[1])
      {
        // second largest, insert in between
        if(checkVal == -1 || checkVal == 2)
        {
          top3freq[2] = top3freq[1];
          top3ind[2] = top3ind[1];
          top3freq[1] = hist[i]; top3ind[1] = i;
        }
        else if(checkVal == 1)
        {
          top3freq[1] = hist[i]; top3ind[1] = i;
        }
      }
      else
      {
        // else replace the last if necessary
        if(checkVal == -1 || checkVal == 2)
        {
          top3freq[2] = hist[i]; top3ind[2] = i;
        }
      
      }
    
    }
  }
 
  

  vector <int> outIndex;
  // push into a vector and sort
  outIndex.push_back(top3ind[0]);
  outIndex.push_back(top3ind[1]);
  outIndex.push_back(top3ind[2]);
  sort(outIndex.begin(), outIndex.end());

  delete []hist;
  return outIndex;
}

#endif
