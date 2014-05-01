/*
	Created by Vignesh Ramanathan and Armand Joulin, 2013.
  Main function to carry out text alignment	
*/

#include "textAlign.hpp"
#include "tvParser.hpp"
#include "speaker_writer.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>

size_t decode_html_entities_utf8(char *dest, const char *src);

using namespace alignment;

// structure to hold the scene based breaks of each description block
typedef struct _DescriptionBlocks
{
  string _sentence;
  pair <int, int> _sceneMatchChars; // begining and ending character of the description in corresponding scene
  int _sceneId;

  _DescriptionBlocks(string sentence, pair <int, int> sceneMatchChars, int sceneId)
  {
    _sentence = sentence;
    _sceneMatchChars = sceneMatchChars;
    _sceneId = sceneId;
  }

}DescriptionBlocks;

typedef vector<DescriptionBlocks> descriptionScenes; //to hold all the description block scene breaks

/*
 *  @brief: check if a stirng corresponds to a scene break
 *
 *  @paramters:
 *    description - the string to check for
 *
 *  @output:
 *     true if description is a scene break, else false
 *
 * */

inline bool isBreak(string description)
{
  string tempStr(description);
//  boost::to_upper(description);

  // lazy way to check if string contains any lowercse character, if not call it a scene break
//  if (tempStr == description)
//  {
    cout << tempStr << ":" << tempStr.find("EXT.") << endl;
    //if(tempStr.find("INT.")==0 || tempStr.find("EXT.")==0 || tempStr.find("ANGLE")==0 || 
    //    tempStr.find("CUT TO")==0)
    if(tempStr.find("INT.") < 10 || tempStr.find("EXT.") < 10 || tempStr.find("ANGLE") < 10 || 
        tempStr.find("CUT TO") < 10)
    {
      return true;
    }
    else
    {
      return false;
    }
//  }
//  else
//  {
//    return false;
//  }
}

/*
 *  @brief: function to update the scene breaks in sequence of blocks of descriptions
 *
 *  @parameters:
 *    description - sequence of string belonging to the block of description
 *    sceneBreaks - the set of scenes which have been added till now
 *    blockScenes - the scene based indexing of the block of descriptions
 *
 *
 * */

void breakIntoScenes(vector <string> description, vector <string> &sceneBreaks, descriptionScenes &blockScenes, string lastSpeaker="null-1")
{
  //cout << "IN BREAKING SCENES ============================================================================" << endl;

  string lastSceneBlock;
  if (sceneBreaks.size() > 0)
  {
    lastSceneBlock = sceneBreaks[sceneBreaks.size() - 1];
  }
  else
  {
    lastSceneBlock = "";
    sceneBreaks.push_back(lastSceneBlock);
  }
  int lastSceneId = sceneBreaks.size() - 1;
  //cout << "descr-size: " << description.size();
  for(int i = 0; i < description.size(); i++)
  {
    boost::replace_all(description[i], "\n", " ");

    cout << "description -----> " << description[i] << " : " << isBreak(description[i]) << endl;
    if (isBreak(description[i]))
    {
      sceneBreaks[lastSceneId] = lastSceneBlock;
      lastSceneBlock = description[i] + "\n";
      sceneBreaks.push_back(lastSceneBlock);
      lastSceneId = sceneBreaks.size() - 1;
      //cout << "BREAK AT ********************************* " << description[i] << " ---------------- scene = " << lastSceneId << endl;

    }
    else
    {
      int beg_char = lastSceneBlock.length();
      int end_char = beg_char + description[i].length() + 1;

      if (lastSpeaker.compare("null-1") != 0 && i==0)
      {
        lastSceneBlock += " " + lastSpeaker + " speaks a dialogue. ";
      }

      lastSceneBlock += description[i] + " ";
      sceneBreaks[lastSceneId] = lastSceneBlock;

      /* ----------------------------------------------
       *        Sanity Check
       * ----------------------------------------------*/

      int beg_check_char = lastSceneBlock.find((description[i] + " "));
      if (beg_check_char !=  beg_char)
      {
        cout << "Sanity check failed: check_beg = " << beg_check_char << ", original_beg = " << beg_char << endl;
        //cout << "Description = ***" << description[i] << " " << "*** ||||||||| ***" << lastSceneBlock << endl;
      }
      else
      {
        cout << "Sanity check passed" << endl;
      }

      blockScenes.push_back(DescriptionBlocks(description[i] + " ", make_pair(beg_char, end_char), lastSceneId));
      //cout << "DESC AT ********************************* " << description[i] << " ---------------- " << beg_char << " , " << end_char << " : scene = " << lastSceneId << endl;
    }    

  }  
}


/*
 *  @brief: clear unwanted expressions out of the script
 *
 *  @parameters:
 *    line - input string
 *
 *  @output:
 *    cleaned line without unwanted expressions (including htmlentities) *
 *
 * */

string cleanTheScript(string line)
{
  boost::regex re("&\\s?[A-Za-z]+\\s?;");
  string out_line = boost::regex_replace(line, re, "-");

  vector<string> exprs;
  exprs.push_back("FADE OUT");
  exprs.push_back("FADE IN");
  exprs.push_back("CONTINUE \\d+");
  exprs.push_back("\\d+\\s+\\(?CONTINUED\\)?:?\\s+\\(?\\d+\\)?\\s+\\d+");
  exprs.push_back("\\(?CONTINUED\\)?:?\\s+\\(?\\d+\\)?\\s+\\d+");
  exprs.push_back("\\(?CONTINUED\\)?:?\\s+\\(?\\d+\\)?");
  exprs.push_back("CONTINUED\\s+\\d+");
  exprs.push_back("\\(?CONTINUED\\)?");
  exprs.push_back("CONTINUE");
  exprs.push_back("OMITTED\\s+(\\d+)");
  exprs.push_back("OMITTED\\s+\\d+");
  exprs.push_back("OMITTED");
  exprs.push_back("OMIT");
  exprs.push_back("RESUME S?C?E?N?E?");
  exprs.push_back("BACK TO SCENE");
  exprs.push_back("CUT TO");
  exprs.push_back("ANGLE");
  /*exprs.push_back("INT");
  exprs.push_back("EXT");*/


  for(size_t i = 0; i < exprs.size(); i++)
  {
    boost::regex reg(exprs[i].c_str());
    out_line = boost::regex_replace(out_line, reg, "");  
  }

  return out_line;
}

int main(int argc, char* argv[])
{

	// check for the right number of arguments
  if( argc < 6){
  	cout << "Not enough input arguments (required: 4)" << endl;
    cout << "1. Script directory, 2. srt file, 3. output alignment file, 4. output scene-breaks file, 5. output speaker alignment file" << endl;
  	exit(EXIT_FAILURE);
  }

  // the output file to store the time ends
  // and the descriptions
  string outFilename(argv[3]);
  string outFilenameScene(argv[4]);
  string outFilenameSpeaker(argv[5]);

  // parse the srt file
  SrtParser srt_parser;
  srt_parser.parseFile(argv[2]);

  HocrScriptParser script_parser;
  script_parser.parseFile(argv[1]);

  // Now tokenize each dialogue, and insert the words to a vector of words
  int numWords_srt = 0;
  vector <vector <string> > subWordTokens;
  for (int i = 0; i < srt_parser._dialogues.size(); i++)
  {
    WordTokenizer dialogueWords(srt_parser._dialogues[i]);
    subWordTokens.push_back(dialogueWords._wordTokens);
    numWords_srt += dialogueWords._wordTokens.size();
  }

  // tokenize script to word tokens
  WordTokenizer scriptWords(script_parser._dialogues, 
      script_parser._lineToBlockIndex, script_parser._dialogueToLineIndex, numWords_srt);


  // the double ended queue to store the matching index pair
  // <subtitle index, <matching begining in script, matching end in script> >
  deque < pair <int, pair<int, int> > > finalWordAlignment;
  deque <float> matchScores;


  /**************************************
   * OPTION 1 - do word to word alignment
   * *************************************
   *
     WordTokenizer subWords(srt_parser._dialogues); 
     getTimeSynchDistance(subWords._wordTokens, scriptWords._wordTokens, normalized_levenshtein_distance, finalWordAlignment, true);  
  */


  /* *******************************************
   * OPTION 2 - slower but more accurate alignment, whre substring alignment is done throug DP as well
   * ********************************************
   */ 
    getTimeSynchDistance(subWordTokens, scriptWords._wordTokens, timeSynchDistanceWithAlignment, finalWordAlignment, matchScores, true);
  
  
  // OPTION 3 - faster but slightly less accurate, uses a greedy scheme for string matchin when comparing
  // subtitle string to a sub-segment of the script
  //getTimeSynchDistance(subWordTokens, scriptWords._wordTokens, greedyTimeSynchDist, finalWordAlignment, true);
  
  
  /* 
   * 
   * UNCOMMENT to print the alignment from the script to get only action segments without dialogue
   *
   */
   // get the time-synched scripts
#ifdef DEBUG
  cout << "Starting complete line distance measurement ... " << endl;
#endif
  
  // First, pool the time-stamps for different dialogue blocks
  for(int i = 0; i < (finalWordAlignment.size()) ; i++)
  {
    //int deb;
    //cout << "stopped to debug: 1 :"; cin >> deb; cout << deb;

    if(matchScores[i] < SCORE_THRESH)
    {
      continue;
    }

    pair <int, pair <int, int> > subId = finalWordAlignment[i];
    pair <int,int> srtTime = parseTime(srt_parser._timeStamps[subId.first]);

    
    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    cout << "Matching happens at : " << srt_parser._dialogues[subId.first] << " MATCHES TO WORDS " << scriptWords._wordTokens[subId.second.first] << "  ---- TO ---- " << scriptWords._wordTokens[subId.second.second] << endl;
    cout << "\" " << script_parser._dialogues[scriptWords._wordToSentenceIndex[subId.second.first]] << " \" " << " ... to ... " << " \" ";
    cout << script_parser._dialogues[scriptWords._wordToSentenceIndex[subId.second.second]] << " \" " << endl;
    

    int dialogueBlockId_beg = script_parser._dialogueToLineIndex[scriptWords._wordToSentenceIndex[subId.second.first]];
    dialogueBlockId_beg = script_parser._lineToBlockIndex[dialogueBlockId_beg];
    
    int dialogueBlockId_end = script_parser._dialogueToLineIndex[scriptWords._wordToSentenceIndex[subId.second.second]];
    dialogueBlockId_end = script_parser._lineToBlockIndex[dialogueBlockId_end];
  
   // cout << "stopped to debug: 2 :"; cin >> deb; cout << deb;
    
    if(dialogueBlockId_beg <= dialogueBlockId_end)
    {
      cout << "dialogue Id: " << dialogueBlockId_beg << endl;
      // First do it for the block where the dialogue begins
      pair<int,int> ts = script_parser._timeStamps[dialogueBlockId_beg];
      if (ts.first == -1 || ts.first > srtTime.first )
      {
        ts.first = srtTime.first;
      }

      if (ts.second == -1 || ts.second < srtTime.second )
      {
        ts.second = srtTime.second;
      }
      script_parser._timeStamps[dialogueBlockId_beg] = ts;

      // Next do it for the block where the dialogue ends
      ts = script_parser._timeStamps[dialogueBlockId_end];
      if (ts.first == -1 || ts.first > srtTime.first )
      {
        ts.first = srtTime.first;
      }

      if (ts.second == -1 || ts.second < srtTime.second )
      {
        ts.second = srtTime.second;
      }
      script_parser._timeStamps[dialogueBlockId_end] = ts;

    }
    else
    {
      cout << "Some time-alignment error in assigning dialogue ids !" << endl;
    }  
  }


  ofstream outFile(outFilename.c_str());
  char tempTime[4];

  /* 
   | ---------------------------------------------------------------------------------|
   |                                                                                  |
   | Breaking the dialogue blocks into scenes and assigning scene ids to each block   |
   |                                                                                  |
   | ---------------------------------------------------------------------------------|
   */
  
  vector <string> sceneBreaks;
  boost::unordered_map <int, descriptionScenes> blockScenes;
  string lastSpeaker;  
  int speakerCtr = 0;

  lastSpeaker = "null-1";

  int dia_block_count = 0;
  for (int i = 0; i < script_parser._blocks.size(); i++)
    if (script_parser._blocks[i]._blockId == DIALOGUE)
      dia_block_count++;
  cout << "Num dia blocks = " << dia_block_count << ", speaker count = " << script_parser._dialogueSpeakers.size() << endl;

  for (int i = 0; i < (script_parser._blocks.size()); i++)
  {
    vector <string> description;
    descriptionScenes blockScene;

    //cout << "start-break: (" << i << "/" << script_parser._blocks.size() << ")" << endl;
    cout << speakerCtr << "/" << script_parser._dialogueSpeakers.size() << endl;
    if(script_parser._blocks[i]._blockId == DIALOGUE)
    {
      lastSpeaker = script_parser._dialogueSpeakers[speakerCtr++];    
    }

    //cout << "mid-break: (" << i << ")" << endl;

    if(script_parser._blocks[i]._blockId == DESCRIPTION || script_parser._blocks[i]._blockId == UNDECIDED)
    {
      ScriptBlock dBlock= script_parser._blocks[i];
      for(int k = 0; k < dBlock._scriptLines.size(); k++)
      {
        string clean_line = cleanTheScript(script_parser._hocrSentences[dBlock._scriptLines[k]].line);
        if (script_parser._blocks[i]._blockId != UNDECIDED || isBreak(clean_line))
          description.push_back(clean_line);
      }
      breakIntoScenes(description, sceneBreaks, blockScene, lastSpeaker);
      blockScenes[i] = blockScene;
    }

    //cout << "end-break: (" << i << ")" << endl;

  }

  cout << "Completed scene breaking" << endl;

  // Display the dialogues aligned in the script with time-stamp
  for(int i = 0; i < (script_parser._dialogueBlockIds.size() - 2); i++)
  {
    int blockId_1 = script_parser._dialogueBlockIds[i];
    int blockId_2 = script_parser._dialogueBlockIds[i+1];
    int blockId_3 = script_parser._dialogueBlockIds[i+2];

    pair <int, int> descrTimeStamp = assertTimeStamps(script_parser._timeStamps[blockId_1], script_parser._timeStamps[blockId_2], script_parser._timeStamps[blockId_3]);

    cout << "Time stamps: " << script_parser._timeStamps[blockId_1].first << "," << script_parser._timeStamps[blockId_2].first << "," <<  script_parser._timeStamps[blockId_3].first << endl;

    if (descrTimeStamp.first == -1 || descrTimeStamp.second == -1)
    {
      continue;
    }

    // cout << "B1 = " << blockId_1 << " , B2 = " << blockId_2 << " ============================ " << endl;
    //vector <string> description;
    string outDescription = "";
    descriptionScenes blockSceneOut;
    for(int j = blockId_1; j <= blockId_2; j++)
    {   
      if(script_parser._blocks[j]._blockId == DESCRIPTION)
      {
        blockSceneOut.insert(blockSceneOut.end(), blockScenes[j].begin(), blockScenes[j].end());
      }
    }

    if (blockSceneOut.size() > 0)
    {
      descriptionScenes blockSceneFinal;
      int prev_scene = blockSceneOut[0]._sceneId;
      int beg_char = blockSceneOut[0]._sceneMatchChars.first;
      int end_char = blockSceneOut[0]._sceneMatchChars.second;
      string descriptionOut = "";
      for(int j = 0; j < blockSceneOut.size(); j++)
      {
        int curr_scene = blockSceneOut[j]._sceneId;
        if (curr_scene==prev_scene)
        {
          descriptionOut += blockSceneOut[j]._sentence;
          end_char = blockSceneOut[j]._sceneMatchChars.second;        
        }
        else
        {
          blockSceneFinal.push_back(DescriptionBlocks(descriptionOut, make_pair(beg_char, end_char), prev_scene));
          beg_char = blockSceneOut[j]._sceneMatchChars.first;
          end_char = blockSceneOut[j]._sceneMatchChars.second;
          descriptionOut = blockSceneOut[j]._sentence;
          prev_scene = curr_scene;
        }
      }
      blockSceneFinal.push_back(DescriptionBlocks(descriptionOut, make_pair(beg_char, end_char), prev_scene));


      cout << "time: " << descrTimeStamp.first << " --> " << descrTimeStamp.second << endl;

      descrTimeStamp.first = descrTimeStamp.first - 20;
      descrTimeStamp.first = descrTimeStamp.first>0? descrTimeStamp.first: 0;

      int hh_beg = (descrTimeStamp.first)/3600;
      int mm_beg = (descrTimeStamp.first%3600)/60;
      int ss_beg = ((descrTimeStamp.first%3600)%60);

      descrTimeStamp.second = descrTimeStamp.second + 20 - descrTimeStamp.first;

      int hh_end = (descrTimeStamp.second)/3600;
      int mm_end = (descrTimeStamp.second % 3600) / 60;
      int ss_end = ((descrTimeStamp.second % 3600) % 60);
      

      for(int j = 0; j < blockSceneFinal.size(); j++)
      {
        string sDescription(blockSceneFinal[j]._sentence);
        //boost::replace_all(sDescription, "\n", " ");
 
        outFile << time2d(hh_beg, tempTime) << ":" << time2d(mm_beg, tempTime) << ":" << time2d(ss_beg, tempTime) << endl;
        outFile << time2d(hh_end, tempTime) << ":" << time2d(mm_end, tempTime) << ":" << time2d(ss_end, tempTime) << endl;       
        cout << sDescription;
        outFile << sDescription << endl;
        outFile << "Scene:" << blockSceneFinal[j]._sceneId << " : (" << blockSceneFinal[j]._sceneMatchChars.first << "," << blockSceneFinal[j]._sceneMatchChars.second << ")" << endl ;
      }

      outFile << endl;
      cout << endl;

    }
    
  
  }

  outFile.close();
  
  /******** Writing scene blocks to be used later for coref **********/

  ofstream outFileScene(outFilenameScene.c_str());
  
  for(int i = 0; i < sceneBreaks.size(); i++)
  {
    outFileScene << "---------------------- (" << i  << ") -----------------------------------" << endl;
    outFileScene << sceneBreaks[i];
    outFileScene << endl << endl;
  }
  outFileScene.close();

  /*** Write the dialogue speaker names along with time-stamps of dialogues ***/
  write_speaker_with_timestamps(script_parser, outFilenameSpeaker);
  

  return 0;

}
