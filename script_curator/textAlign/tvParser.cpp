/*
	Created by Vignesh Ramanathan and Armand Joulin, 2013.
	Compute the alignement between two texts
	Partialy use code from Sten Hjelmqvist (through wikipedia)
*/

#ifndef __TVPARSER_CPP
#define __TVPARSER_CPP

#include "tvParser.hpp"

namespace alignment
{

  /*******************************************************************************/

  _WordTokenizer::_WordTokenizer(vector <string> sentences, 
      vector <int> lineToBlockIds,
      vector <int> dialogueToLineIds,
      int numWords_srt)
  {
    int wordIndex = 0;
    vector<int>::iterator it = _wordToSentenceIndex.end();


    int numWords_script = 0;
    vector <string> dummyWordTokens;
    int numBlocks = 0;
    vector <int> blockIds;
    for (int i = 0; i < sentences.size(); i++)
    {
      string lowerSentence = sentences[i];

      // convert to lower case

      transform(lowerSentence.begin(), lowerSentence.end(), lowerSentence.begin(), ::tolower);
      boost::replace_all(lowerSentence, "*", " ");
      boost::tokenizer<> sentenceTokenizer(lowerSentence);
      dummyWordTokens.insert(dummyWordTokens.begin(), sentenceTokenizer.begin(), sentenceTokenizer.end());
      numWords_script += dummyWordTokens.size();
      dummyWordTokens.clear();

      blockIds.push_back( lineToBlockIds[dialogueToLineIds[i]] );
    }
    vector <int>::iterator it2;
    it2 = unique(blockIds.begin(), blockIds.end());
    blockIds.resize( distance(blockIds.begin(), it2) );
    numBlocks = blockIds.size();
    numBlocks = (numBlocks > 0)?numBlocks:1;


    int dummyCount = 50;
    if(numWords_script < (numWords_srt + 200) )
    {
      dummyCount = ceil(double(numWords_srt + 200 - numWords_script)/double(numBlocks)) + 50;    
    }
    else
    {
      dummyCount = 50;
    }
    cout << "The num_blocks = " << numBlocks << ", num_srt = " << numWords_srt << ", num_script = " << numWords_script << ", dummy word count = " << dummyCount << endl;


    int blockIds_curr, blockIds_next;
    // Insert dummy sentence to account for noise
    vector <string> dummySentence;
    for(int i = 0; i < dummyCount; i++)
    {
      dummySentence.push_back("xxxx");
    }
    _wordTokens.insert(_wordTokens.end(), dummySentence.begin(), dummySentence.end());
    _wordToSentenceIndex.insert(it, dummyCount, 0);
    it = _wordToSentenceIndex.end();
    wordIndex += dummyCount;


    for(int i = 0; i < sentences.size(); i++)
    {
      string lowerSentence = sentences[i];

      // convert to lower case

      transform(lowerSentence.begin(), lowerSentence.end(), lowerSentence.begin(), ::tolower);
      boost::replace_all(lowerSentence, "*", " ");
      boost::tokenizer<> sentenceTokenizer(lowerSentence);

      // get word begining index of a sentence
      _wordTokens.insert(_wordTokens.end(), sentenceTokenizer.begin(), sentenceTokenizer.end()); 
      _sentenceToWordIndex.push_back(wordIndex);
      
      // enter the sentence id n times
      _wordToSentenceIndex.insert(it, (_wordTokens.size() - wordIndex), i); 
      it = _wordToSentenceIndex.end();

      // Insert dummy sentence between block changes for robustness
      if( i < (sentences.size() - 1))
      {
        // identify the block id of current dialogue
        blockIds_curr = lineToBlockIds[dialogueToLineIds[i]];
        blockIds_next = lineToBlockIds[dialogueToLineIds[i+1]];

        //cout << "B = " << blockIds_curr << sentences[i] << " ~~~~~~~~~~~~~~~ " << endl;      
        if(blockIds_curr != blockIds_next)
        {
          _wordTokens.insert(_wordTokens.end(), dummySentence.begin(), dummySentence.end());
          _wordToSentenceIndex.insert(it, dummyCount, i);
          it = _wordToSentenceIndex.end();
        }
      }

      wordIndex = _wordTokens.size();
      cout << wordIndex << endl;
    }

 
  }


  /*******************************************************************************/

  _WordTokenizer::_WordTokenizer(vector <string> sentences)
  {
    int wordIndex = 0;
    vector<int>::iterator it = _wordToSentenceIndex.end();

    for(int i = 0; i < sentences.size(); i++)
    {
      string lowerSentence = sentences[i];
      transform(lowerSentence.begin(), lowerSentence.end(), lowerSentence.begin(), ::tolower);
      boost::tokenizer<> sentenceTokenizer(lowerSentence);

      _wordTokens.insert(_wordTokens.end(), sentenceTokenizer.begin(), sentenceTokenizer.end());
 
      _sentenceToWordIndex.push_back(wordIndex);
      
      // enter the sentence id n times
      _wordToSentenceIndex.insert(it, (_wordTokens.size() - wordIndex), i); 
      it = _wordToSentenceIndex.end();

      /*
      for (int j = wordIndex; j < _wordTokens.size() ; j++)
      {
        cout << i << " : " << _wordTokens[j] << endl;
      }*/

      wordIndex = _wordTokens.size();

    }
 
  }

  /*******************************************************************************/

  _WordTokenizer::_WordTokenizer(string sentence)
  {
    int wordIndex = 0;
    //vector <string> wordsInSentence;
    string lowerSentence = sentence;
    transform(lowerSentence.begin(), lowerSentence.end(), lowerSentence.begin(), ::tolower);
    boost::tokenizer<> sentenceTokenizer(lowerSentence);

    _wordTokens.insert(_wordTokens.begin(), sentenceTokenizer.begin(), sentenceTokenizer.end());
    _sentenceToWordIndex.push_back(wordIndex);
  }

  /*******************************************************************************/

  void SrtParser::parseFile(string srtFilename)
  {

    // first empty the time stamp and dialogue vectors
    _dialogues.clear();
    _timeStamps.clear();

    // check if subtitle file exists
    ifstream subfile;
    subfile.open (srtFilename.c_str());
    if(!subfile)
    {
      cout<< "Cannot open "<< srtFilename <<": Not such a file"<<endl;
      exit(EXIT_FAILURE);
    } 

    // to check what the current line is
    enum {
      LINE_ID,
      TIME_STAMP,
      DIALOGUE
    };

    int srtLineIdentifier = LINE_ID;
    int currentLineId = 0, prevLineId;
    string buf, dialogueBuffer;
    bool pushFlag = false;
    // parse the srt file
    vector<string> sub;
    while(!subfile.eof()) // To get you all the lines.
    {
      getline(subfile,buf); // Saves the line in buf.
      //cout << buf << ":" << srtLineIdentifier << ":" << (int)(buf[0]) << endl;
      // Weirdly the break inbetween dialogues in a srt file is enforced by a charcter with ASCII value 13 !
      if (buf != "\n" && buf != "" && buf != " " && (int)(buf.c_str()[0]) != 13 && (int)(buf.c_str()[0]) != 0)
      {
        switch (srtLineIdentifier)
        {
          case LINE_ID: // to do if the line is a LINE_ID
            prevLineId = currentLineId;
            currentLineId = atoi(buf.c_str());

            // sanity check
            if (currentLineId != (prevLineId+1) && currentLineId != MAX_SRT_LINES)
            {
              cout << "The Srt file maybe corrupted at: " << prevLineId << endl;
              //exit(EXIT_FAILURE);
            }
      
            srtLineIdentifier = TIME_STAMP;
            break;

          case TIME_STAMP: // to do if the line is a time stamp
            _timeStamps.push_back(buf);
            srtLineIdentifier = DIALOGUE;
            dialogueBuffer = "";
            break;

          case DIALOGUE: // to do if line is a dialogue
            pushFlag = true;
            std::transform(buf.begin(), buf.end(), buf.begin(), ::tolower);
            if (buf.c_str()[0] != '<')
              dialogueBuffer = dialogueBuffer + string(buf + string(" ")); // keep concatenating the dialogues
            break;      
        }
      }
      else
      {
        // in case of new line, set LINE_ID and push current dialogue into the vector
        // cout << "blah" << currentLineId << ":" << (int)(buf == "") << endl;
        if (currentLineId>0 && pushFlag)
        {
          pushFlag = false;
          srtLineIdentifier = LINE_ID;
          _dialogues.push_back(removeHtmlTags(dialogueBuffer));

#ifdef DEBUG
          cout << "<" << currentLineId << "> : " <<  removeHtmlTags(dialogueBuffer) << endl;
#endif
        }
      }

      if(buf!="\n" && buf!="")
      {
        sub.push_back(buf); // append to buf.
      }
    }
      
    // last two lines in a srt are garbage, so pop them out ....
    if (!_dialogues.empty())
    {
      _dialogues.pop_back();
      _timeStamps.pop_back();
    }
    subfile.close();
  }

  /*********************************************
   *               DEPRICATE
   * *******************************************/
  
  bool ScriptParser::checkBlockBreak(string line)
  {
    
    if(line == "" || line == "\n")
    {
      cout << "empy line" << endl;
      return true;
    }
    
    vector <string> wordTokens;
    boost::tokenizer<> sentenceTokenizer(line);
    wordTokens.insert(wordTokens.end(), sentenceTokenizer.begin(), sentenceTokenizer.end());
    
    // Consider a block break, when the first word in a sentence is completely characterized
    // and also note it as the block label
    if (wordTokens.size() == 0)
      return true;

    string firstWord = wordTokens[0];
    for(unsigned int i = 0; i < wordTokens[0].size(); i++)
    {
      if(firstWord[i] >= 'a' && firstWord[i] <= 'z')
        return false;
    }
    
    return true;
  }


  /*********************************************
   *               DEPRICATE
   * *******************************************/
  
  void ScriptParser::parseFile(string scriptFilename)
  {
    // check if script file exists
    ifstream scriptfile;
    scriptfile.open (scriptFilename.c_str());

    if(!scriptfile)
    {
      cout<< "Cannot open "<< scriptFilename << ": Not such a file" << endl;
      exit(EXIT_FAILURE);
    } 

    // copy script file to lines and deduce block breaks
    string buf;     
    int blockId = -1, lineCtr = 0;
    vector <int> tempBlock;
    //vector <vector <int> > :: iterator it = _blockToLineIndex.begin();
    while(!scriptfile.eof()) 
    {
      getline(scriptfile,buf); // Saves the line in buf.
      _lines.push_back(buf); // append to script.
      //cout << buf;

      // if a block break, increment block index
      if (checkBlockBreak(buf) || blockId < 0)
      {
        if(blockId >= 0)
        {
          _blockToLinesIndex.push_back(tempBlock);
        }
        blockId++;
        tempBlock.clear();
      }

      // push block and line indices
      tempBlock.push_back(lineCtr++);
      _lineToBlockIndex.push_back(blockId);

    }

    if(tempBlock.size() > 0)
    {
      _blockToLinesIndex.push_back(tempBlock);
      tempBlock.clear();
    }

    // print for checking
    /*for(int i = 0; i < _blockToLinesIndex.size(); i++)
    {
      for(int j = 0; j < _blockToLinesIndex[i].size(); j++)
      {
        cout << _blockToLinesIndex[i][j] << " ";
      }
      cout << endl;
    }*/

    scriptfile.close();
    
  }

  /*******************************************************************************/
  /* 
   * TODO: replace special characters by there ASCII character (#$39; -> \') for instance
   *       there seems to be a bug in the hocr parser, where sometimes a word falls into a new line,
   *       even when it is not supposed to
   *
   * */
  void HocrScriptParser::parseFile(string hocrDirectory)
  {
    hocrParser(hocrDirectory, _hocrSentences);

    // get the min-max limits for the x-cordinate begining of
    // sentences in a pdf-file to enable classification as 
    // dialogue, description and speaker name
    /*     
           pdfLimits - three pairs where, 
           pdfLimits[0] = <min, max> pixel limits of description in a pdf
           pdfLimits[1] = <min, max> pixel limits of dialogue in a pdf
           pdfLimits[2] = <min, max> pixel limits of speaker in a pdf
    */

    vector <int> startIndices;
    for(int i = 0; i < _hocrSentences.size(); i++)
    {
      int charCount = 0;
      for(string::iterator it = _hocrSentences[i].line.begin();  it != _hocrSentences[i].line.end(); ++it)
      {
        char c = *it;
        if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
          charCount++;
      }
      if(charCount >=3)
        startIndices.push_back(_hocrSentences[i].x_cord.first);
    }
    vector <int> freqBegs = getMinLimits(startIndices);
    pair <int,int> pdfLimits[3];
    pdfLimits[0] = make_pair(freqBegs[0] - 150, (freqBegs[0] + freqBegs[1])/2 - 1);
    pdfLimits[1] = make_pair((freqBegs[0] + freqBegs[1])/2 + 1, (freqBegs[1] + freqBegs[2])/2 - 1);
    pdfLimits[2] = make_pair((freqBegs[1] + freqBegs[2])/2 + 1, freqBegs[2] + 150);


    int DIALOGUE_MIN1 = pdfLimits[1].first;
    int DIALOGUE_MAX1 = pdfLimits[1].second;
    cout << "dialogue limits: " << DIALOGUE_MIN1 << " ... to ... " << DIALOGUE_MAX1 << endl;

    int DESCRIPTION_MIN = pdfLimits[0].first;
    int DESCRIPTION_MAX = pdfLimits[0].second;
    cout << "description limits: " << DESCRIPTION_MIN << " ... to ... " << DESCRIPTION_MAX << endl;


    int SPEAKER_MIN = pdfLimits[2].first;
    int SPEAKER_MAX = pdfLimits[2].second;
    cout << "speaker limits: " << SPEAKER_MIN << " ... to ... " << SPEAKER_MAX << endl;


    for (int i = 0; i < _hocrSentences.size(); i++)
    {
      

      if(_hocrSentences[i].x_cord.first <= DIALOGUE_MAX1
        && _hocrSentences[i].x_cord.first >= DIALOGUE_MIN1)
//        && _hocrSentences[i].x_cord.second <= DIALOGUE_MAX2)
      {
        _lineToSectionIndex.push_back(DIALOGUE);
        // Create list of dialogues
        _dialogues.push_back(_hocrSentences[i].line);
        _dialogueToLineIndex.push_back(i);
      } //Dialogues
      else if(_hocrSentences[i].x_cord.first <= DESCRIPTION_MAX
        && _hocrSentences[i].x_cord.first >= DESCRIPTION_MIN)
      {
        _lineToSectionIndex.push_back(DESCRIPTION);
      } //Descriptions
      else if(_hocrSentences[i].x_cord.first <= SPEAKER_MAX
         && _hocrSentences[i].x_cord.first >= SPEAKER_MIN)
      {
        _lineToSectionIndex.push_back(SPEAKER);
      } //Speaker
      else
      {
        _lineToSectionIndex.push_back(UNDECIDED);
      } //Maybe noise

    }

    int blockIndex = 0;
    vector <int> tempBlock;
    _lineToBlockIndex.push_back(blockIndex);
    tempBlock.push_back(0);
    
    int last_speaker_blockind = -2;
    string last_speaker = "null-1";

    //cout << blockIndex << " : " << _hocrSentences[0].line << endl;
    for(int i = 1; i < _hocrSentences.size(); i++)
    {
      if(_lineToSectionIndex[i] != _lineToSectionIndex[i-1])
      {
        // push the previous block into _blocks
        ScriptBlock tempScriptBlock;        
        tempScriptBlock._scriptLines = tempBlock;
        tempScriptBlock._blockId = _lineToSectionIndex[i-1];
        _blocks.push_back(tempScriptBlock);
        if(tempScriptBlock._blockId == DIALOGUE)
        {
          _dialogueBlockIds.push_back(blockIndex);
          if (last_speaker_blockind == (blockIndex-1))
          {
            _dialogueSpeakers.push_back(last_speaker);
          }
          else
          {
            _dialogueSpeakers.push_back(string("null-1"));
          }          
          last_speaker = "null-1";
          last_speaker_blockind = -2;
        }
        
        if(tempScriptBlock._blockId == SPEAKER)
        {
          last_speaker_blockind = blockIndex;
          last_speaker = _hocrSentences[i-1].line;
          cout << "SPEAKER: " << last_speaker << endl;
        }
       

        // push an initial time stamp for each block as well
        _timeStamps.push_back(make_pair(-1,-1) );

        tempBlock.clear();
        blockIndex++;
      }
      tempBlock.push_back(i);
      _lineToBlockIndex.push_back(blockIndex);
      //cout << blockIndex << " : " << _hocrSentences[i].line << endl;
    }
    
    if(tempBlock.size() > 0)
    {
      // push the previous block into _blocks
      ScriptBlock tempScriptBlock;        
      tempScriptBlock._scriptLines = tempBlock;
      tempScriptBlock._blockId = _lineToSectionIndex[_hocrSentences.size()-1];
      _blocks.push_back(tempScriptBlock);
      if(tempScriptBlock._blockId == DIALOGUE)
      {
        _dialogueBlockIds.push_back(blockIndex);
        if (last_speaker_blockind == (blockIndex-1))
        {
          _dialogueSpeakers.push_back(last_speaker);
        }
        else
        {
          _dialogueSpeakers.push_back(string("null-1"));
        }          
        last_speaker = "null-1";
        last_speaker_blockind = -2;
      }

      // push an intial time stamp for each block as well
      _timeStamps.push_back(make_pair(-1,-1) );
    }
  }

}
#endif
