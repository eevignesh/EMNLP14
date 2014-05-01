#ifndef __SPEAKER_WRITER_HPP
#define __SPEAKER_WRITER_HPP

#include "textAlign.hpp"
#include "tvParser.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>

using namespace std;



namespace alignment
{

  int write_speaker_with_timestamps(HocrScriptParser script_parser, string outFilename)
  {
    ofstream outFile(outFilename.c_str());

    // Display the dialogues aligned in the script with time-stamp
    for(int i = 0; i < (script_parser._dialogueBlockIds.size() - 2); i++)
    {
      int blockId_1 = script_parser._dialogueBlockIds[i];

      pair <int, int> descrTimeStamp = script_parser._timeStamps[blockId_1];
       
      if (descrTimeStamp.first == -1 || descrTimeStamp.second == -1)
      {
        continue;
      }

      if (script_parser._dialogueSpeakers[i].compare("null-1") == 0)
      {
        continue;
      }


      cout << "time: " << descrTimeStamp.first << " --> " << descrTimeStamp.second << endl;

      descrTimeStamp.first = descrTimeStamp.first - 2;
      descrTimeStamp.first = descrTimeStamp.first>0? descrTimeStamp.first: 0;

      int hh_beg = (descrTimeStamp.first)/3600;
      int mm_beg = (descrTimeStamp.first%3600)/60;
      int ss_beg = ((descrTimeStamp.first%3600)%60);

      descrTimeStamp.second = descrTimeStamp.second + 2 - descrTimeStamp.first;

      int hh_end = (descrTimeStamp.second)/3600;
      int mm_end = (descrTimeStamp.second % 3600) / 60;
      int ss_end = ((descrTimeStamp.second % 3600) % 60);
      
      char tempTime[4];
      outFile << time2d(hh_beg, tempTime) << ":" << time2d(mm_beg, tempTime) << ":" << time2d(ss_beg, tempTime) << endl;
      outFile << time2d(hh_end, tempTime) << ":" << time2d(mm_end, tempTime) << ":" << time2d(ss_end, tempTime) << endl;
      outFile << script_parser._dialogueSpeakers[i] << endl;

      outFile << endl << endl;
      cout << endl;
    
    }

    outFile.close();
  }

}

#endif
