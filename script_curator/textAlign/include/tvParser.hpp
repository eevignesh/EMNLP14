

#ifndef __TVPARSER_HPP__
#define __TVPARSER_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm> 
#include "hocr.hpp"
#include <boost/tokenizer.hpp>
#include "utils.hpp"

#define MAX_SRT_LINES 9999

// define the pixel limits to break the pdf formatting
// for each section
//#define DIALOGUE_MAX1 900
//#define DIALOGUE_MIN1 700
//#define DIALOGUE_MIN1 610

#define DIALOGUE_MAX2 2400

//#define DESCRIPTION_MAX 600
//#define DESCRIPTION_MIN 400

//#define SPEAKER_MAX 1300
//#define SPEAKER_MIN 1000

using namespace std;
namespace alignment
{

  /*
   * @brief: class to read a vector of sentences and tokenize them to vector of words
   *
   * @members:
   * _sentences - vector of sentence strings
   * _wordTokes - vector of words
   * _sentenceToWordIndex - a map which gives the index of the first word for each sentence as indexed by _wordTokens
   * _wordToSentenceIndex - the line number (sentence number) from which the corresponding word is extracted from.
   * */
  typedef class _WordTokenizer
  {
    public:
      vector <string> _wordTokens;
      vector <int> _sentenceToWordIndex;
      vector <int> _wordToSentenceIndex;

      /*
       * @brief: initializes with a vector of sentences, tokenizes and cretes the indexing
       *         additionally introduces dummy sentences in between blocks
       *
       * @paramters:
       * sentences - the input vector of sentences to tokenize
       * lineToBlockIds - blockIds for each line in the script
       * dialogueToLineIds - line id for each dialogue
       * numWords_srt - number of words in a srt file
       *
       * */
      _WordTokenizer(vector <string> sentences, 
          vector <int> lineToBlockIds, 
          vector <int> dialogueToLineIds,
          int numWords_srt);

      /*
       * @brief: initializes with a vector of sentences, tokenizes and cretes the indexing
       *
       * @paramters:
       * sentences - the input vector of sentences to tokenize
       * */
      _WordTokenizer(vector <string> sentences);

      /*
       * @brief: initializes with a single sentence, tokenizes and cretes dummy index (should depricate)
       *
       * @paramters:
       * sentence - the input sentence to tokenize
       * */

      _WordTokenizer(string sentence);

  } WordTokenizer;


  /*
   * @brief: class to parse an srt file
   *
   * @members:
   * _timeStamps - string vector of time stamps from srt file
   * _dialogues - string vector of dialogues
   *
   * */

  typedef class _SrtParser
  {
    public:
      vector <string> _timeStamps;
      vector <string> _dialogues;

      /*
       * @brief: function to extract the timestamps and dialogues
       *
       * @paramters:
       * srtFilename - string filename of the srt file
       * */
      void parseFile(string srtFilename);
  }SrtParser;


  /*-------------------------------------------------------------------------------
   *                               DEPRICATE
   *-------------------------------------------------------------------------------
   * @brief: class to parse the script file, includes some tricks to break the script
   *         into blocks based on standard practices for writing a script file
   *
   * @members:
   * _lines - string vector, where each string is a separate line in the script
   * _lineToBlockIndex - int vector, where each value gives the index of the corresponding block
   * _blockToLinesIndex - vector of int vectors, where each entry gives the indices of lines
   *                      present in the block
   *
   * */
   
  typedef class _ScriptParser
  {
    public:
      vector <string> _lines;
      vector <int> _lineToBlockIndex;
      vector < vector<int> > _blockToLinesIndex;

      /*
       * @brief: function to extract the lines and blocks in a script
       *
       * @parameters:
       * scriptFileName - name of the script file to parse
       *
       * */
       void parseFile(string scriptFilename);

      /*
       * @brief: function to check if the line indicates the start of a new block
       *
       * @parameters:
       * scriptFileName - line to check for block break
       *
       * @output:
       * if line indicates block-break return true, else return false
       *
       * */
       bool checkBlockBreak(string line);

  }ScriptParser;

  /* 
   *  @brief: each block of the script is a set of consecutive dialogues or descriptions or speaker
   *
   *  @members:
   *  _scriptLines: vector of line ids which go into the block
   *  _blockId: tells if the block is dialogue, description, speaker or undecided
   *
   * */

  typedef struct _ScriptBlock
  {
    vector <int> _scriptLines;
    int _blockId;
  } ScriptBlock;

  /*
   *  @brief: class to parse the hocr files in directory, includes some tricks 
   *  to break the script into dialogues based on standard practices for writing
   *  a script file
   *
   *  @members:
   *  _hocrsentences - string vector, where each string is a separate hocr-line in the script
   *  _dialogues - list of dialogues extracted based on formatting rules
   *  _lineToSectionIndex - int vector, where each value gives the index of the corresponding
   *                       section (dialogue, description, speaker, undecided)
   *  _lineToBlockIndex - break the script into blocks, where a block has consecutive lines
   *                     from the same section
   *  _dialogueToLineIndex - gives the line-id of the dialogue in the script
   *  _blocks - vector of blocks in the script
   *  _dialogueBlockIds - ids of the blocks which are dialogues
   *  _dialogueSpeakers - same size as dialogueBlockIds and wherever applicable holds the name of the speaker
   *  _timeStamps - begining and ending time for each block in the script after alignment
   * 
   * */
   
  typedef class _HocrScriptParser
  {
    public:
      vector <HocrLine> _hocrSentences;
      vector <string> _dialogues;
      vector <int> _lineToSectionIndex;
      vector <int> _lineToBlockIndex;
      vector <int> _dialogueToLineIndex;
      vector <ScriptBlock> _blocks;
      vector <int> _dialogueBlockIds;
      vector <string> _dialogueSpeakers;
      vector < pair<int,int> > _timeStamps;
      /*
       * @brief: function to extract the lines and blocks in a script
       *
       * @parameters:
       * scriptFileName - name of the script file to parse
       *
       */       
       void parseFile(string scriptFilename);

  }HocrScriptParser;
  
  /*
   * @brief: section names
   *
   * */
  enum
  {
    UNDECIDED,
    DIALOGUE,
    DESCRIPTION,
    SPEAKER,
  };

  /*
   *  @brief: parses a time-stamp string of the format hh:mm:ss,begPos --> hh:mm:ss, endPos
   *          to get begining and ending time in seconds
   *
   *  @paramters:
   *  timeStamp - string of the time-stamp for the dialogue
   *
   *  @output:
   *  pair of integers giving begining and ending time in seconds for the dialogue
   *
   * */
  static pair <int, int> parseTime(string timeStamp)
  {
    pair <int, int> ts = make_pair(0,0);

    size_t breakPosBeg = timeStamp.find(':');
    int hh_beg = atoi(timeStamp.substr(breakPosBeg - 2, 2).c_str());
    int mm_beg = atoi(timeStamp.substr(breakPosBeg + 1, 2).c_str());
    int ss_beg = atoi(timeStamp.substr(breakPosBeg + 4, 2).c_str());
    ts.first = (hh_beg)*3600 + (mm_beg)*60 + ss_beg;

    timeStamp = timeStamp.substr(breakPosBeg + 7, timeStamp.size() - breakPosBeg - 7);

    size_t breakPosEnd = timeStamp.find(':');
    int hh_end = atoi(timeStamp.substr(breakPosEnd - 2, 2).c_str());
    int mm_end = atoi(timeStamp.substr(breakPosEnd + 1, 2).c_str());
    int ss_end = atoi(timeStamp.substr(breakPosEnd + 4, 2).c_str());
    ts.second = (hh_end)*3600 + (mm_end)*60 + ss_end;

    return ts;
  }

  /*
   *  @brief: check if time-stamps from three consecutive dialogues are vlaid,
   *          and if true gives the time extent of the description
   *
   *  @paramters:
   *  timeStamp1 - time stamp of first dialogue
   *  timeStamp2 - time stamp of second dialogue
   *  timeStamp3 - time stamp of third dialogue
   *
   *  @output:
   *  output.first = begin time of description
   *  output.second = end time of description
   *  (both first and second are set to -1, if the input time-stamps are not valid)
   *
   * */
  static pair <int, int> assertTimeStamps(pair <int, int> timeStamp1,
      pair <int,int> timeStamp2, pair <int, int> timeStamp3)
  {
    pair <int, int> descrTimeStamp = make_pair(-1, -1);

    if(timeStamp1.first < 0 || timeStamp1.second < 0)
    {
      return descrTimeStamp;
    }
 
    if( !(timeStamp3.first < 0 || timeStamp3.second < 0))
    {
      descrTimeStamp.first = timeStamp1.second;
      descrTimeStamp.second = timeStamp3.second;
      return descrTimeStamp;
    }

    if( !(timeStamp2.first < 0 || timeStamp2.second < 0))
    {
      descrTimeStamp.first = timeStamp1.second;
      descrTimeStamp.second = timeStamp2.second;
      return descrTimeStamp;
    }


    return descrTimeStamp;

  }

}

#endif
