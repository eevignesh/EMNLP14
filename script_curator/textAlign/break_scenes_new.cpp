#include <boost/algorithm/string.hpp>
#include <string>
#include <boost/regex.hpp>
#include <iostream>
#include <fstream>
#include <boost/unordered_map.hpp>
using namespace std;

string LongestCommonSubstring(const string& first, const string& second, int &match_pos)
{
  if(first.length() == 0 || second.length() == 0)
    return "";
  int i = 0, j = 0;
  string max;
  string lcsub = "";
  match_pos = -1;

  for (j=0; j < second.length(); j++)
    for (int k=1; k <= first.length() && k <= second.length(); k++)
    {
      if (first.substr(i,k) == second.substr(j,k))
      {
        lcsub = first.substr(i,k);
      }
      else
      {
        if (lcsub.length() > max.length())
        {
          max=lcsub;
          match_pos = j;
        }
        lcsub="";
      }
    }   

  if (lcsub.length() > max.length())
  {
    max=lcsub;
    match_pos = j;
  }

  lcsub="";
  cout << "Longest Common Substring: " << max << endl;
  return max;
}

string cleanTheScript(string line)
{
  boost::regex re("&\\s?[A-Za-z]+\\s?;");
  string out_line = boost::regex_replace(line, re, " ");

  vector<string> exprs;
  exprs.push_back("FADE OUT");
  exprs.push_back("FADE IN");
  exprs.push_back("CONTINUE \\d+");
  exprs.push_back("CONTINUED\\s+(\\d+)");
  exprs.push_back("CONTINUED\\s+\\d+");
  exprs.push_back("CONTINUED");
  exprs.push_back("CONTINUE");
  exprs.push_back("OMITTED\\s+(\\d+)");
  exprs.push_back("OMITTED\\s+\\d+");
  exprs.push_back("OMITTED");
  exprs.push_back("OMIT");
  exprs.push_back("RESUME S?C?E?N?E?");
  exprs.push_back("BACK TO SCENE");
  exprs.push_back("CUT TO");
  exprs.push_back("ANGLE");
  //exprs.push_back("ON:");
  //exprs.push_back("IN:");
  string exprs_X = "\\s+\\(?X\\)?\\s+";


  for(size_t i = 0; i < exprs.size(); i++)
  {
    boost::regex reg(exprs[i].c_str());
    out_line = boost::regex_replace(out_line, reg, "");
  }

  boost::regex reg(exprs_X.c_str());
  out_line = boost::regex_replace(out_line, reg, " ");  


  return out_line;

}


typedef struct _DescriptionData
{
  string _time_beg;
  string _time_duration;
  string _descriptions;
  int _scene;
  int _beg_char;
  int _end_char;

  void clear()
  {
    _time_beg = "";
    _time_duration = "";
    _descriptions = "";
    _scene = -1;
    _beg_char = -1;
    _end_char = -1;
  }

  _DescriptionData()
  {
    clear();
  }

}DescriptionData;

typedef struct _SceneData
{
  string _description;
  string _header;
  vector<int> _descriptorIds;

  _SceneData()
  {
    _description = "";
    _header = "";    
  }

  _SceneData(string header, string description)
  {
    _description = description;
    _header = header;    
  }


}SceneData;


/*
 *  @brief: parse the description file to break into parts
 *
 *  @parameters:
 *    inputFile - file containing the descriptions
 *    outputDescriptions - output containing the descriptions *
 *
 * */

void parseDescriptionFile(string inputFile, vector <DescriptionData> &outputDescriptions, boost::unordered_map <int, SceneData> &outputScenes)
{
  boost::regex re_time("\\d\\d:\\d\\d:\\d\\d");
  boost::regex re_scene_id("Scene:(\\d+)\\s+:\\s+\\((\\d+),(\\d+)\\)");
  boost::smatch res_matches;
  
  ifstream inFile(inputFile.c_str());
  DescriptionData dData;

  if (inFile.is_open())
  {
    // Read the line from file
    string line;
    int des_id=0;
    while( getline(inFile, line))
    {
      line = cleanTheScript(line);
      if(boost::regex_match(line, res_matches, re_time))
      {
        if(des_id==0)
        {
          des_id = 1;
          dData.clear();
          dData._time_beg = line;
        }
        else if (des_id==1)
        {
          des_id = 2;
          dData._time_duration = line;
        }
        else
        {
          cout << "Something wrong here ...." << endl;
        }
      }
      else if(boost::regex_match(line, res_matches, re_scene_id))
      {
          string s_scene(res_matches[1].first, res_matches[1].second);
          dData._scene = atoi(s_scene.c_str());
          
          string s_beg(res_matches[2].first, res_matches[2].second);
          dData._beg_char = atoi(s_beg.c_str());
          
          string s_end(res_matches[3].first, res_matches[3].second);
          dData._end_char = atoi(s_end.c_str());

          //cout << " *** " << dData._scene << "-" << dData._beg_char << "-" << dData._end_char << endl;

          outputDescriptions.push_back(dData);
          outputScenes[dData._scene]._descriptorIds.push_back(outputDescriptions.size() - 1);

          des_id = 0;
      }
      else if(des_id == 2)
      {
          dData._descriptions += line;
      }
      
    }
    inFile.close();
  }
}



/*
 *  @brief: parse the scene break file to break into parts
 *
 *  @parameters:
 *    inputFile - file containing the scenes
 *    outputScenes - output containing the scenes
 *
 *  @output:
 *    last_scene_num - the last scene number in the scene file (also the max. scene num)
 *
 * */

int parseSceneBreaks(string inputFile, boost::unordered_map <int, SceneData> &outputScenes)
{
  boost::regex re_scene_break("---------------------- \\((\\d+)\\) -----------------------------------");
  boost::smatch res_matches;

  ifstream inFile(inputFile.c_str());
  int max_scene_num = -1;

  if (inFile.is_open())
  {
    // Read the line from file
    string line;
    int des_id=0;
    int last_scene_num  = 0;
    bool start_add = false;
    while( getline(inFile, line))
    {
      line = cleanTheScript(line);
      if(boost::regex_match(line, res_matches, re_scene_break))
      {
        string s_scene_num(res_matches[1].first, res_matches[1].second);
        last_scene_num = atoi(s_scene_num.c_str());
        
        max_scene_num = (last_scene_num > max_scene_num)? last_scene_num : max_scene_num;
        
        outputScenes[last_scene_num]._description = "";
        outputScenes[last_scene_num]._header = "";
        start_add = true;
      }
      else
      {
        
        if ( !(start_add) || (start_add && last_scene_num==0))
        {
          outputScenes[last_scene_num]._description += line;
          
        }
        else if(start_add)
        {
          start_add = false;
          outputScenes[last_scene_num]._header += line;
        }

      }

    }
    inFile.close();
  }

  return max_scene_num;
}


vector <SceneData> getSceneBreaks(SceneData inSceneData, vector<DescriptionData> &desData , string keyWord)
{
  boost::regex re_scene_key("(.*)\\s+([^a-z]+" + keyWord + "[^a-z]+)\\s+(.*)");
  boost::regex re_scene_key_word(keyWord);

  boost::match_results <string::const_iterator> res_matches;
  boost::match_results <string::const_iterator> res_word_matches;

  vector <SceneData> samples;
  vector <SceneData> samples_old;
  vector <SceneData> all_samples;

  samples_old.push_back(inSceneData);

  bool breakDetected = true;
  while(breakDetected)
  {
    breakDetected = false;
    samples.clear();
    for (int i = 0; i < samples_old.size(); i++)
    {
      SceneData test = samples_old[i];

      if (boost::regex_search( test._description, res_word_matches, re_scene_key_word))
      {
        cout << "The keyword " << keyWord << " has matched " << endl;
        cout << test._description << endl;
      }

      if(boost::regex_search( test._description, res_matches, re_scene_key))
      {
        breakDetected = true;
        
        samples.push_back(SceneData(test._header, res_matches[1]));
        samples.push_back(SceneData(res_matches[2], res_matches[3]));
        
        breakDetected = true;
      }
      else
      {
        all_samples.push_back(test);

      }
    }

    samples_old.clear();
    samples_old.swap(samples);
 
  }

  int num_breaks = all_samples.size();
  // Now assign the descriptions to each of the broken up scene segments
  bool noBreak = false;
  vector <pair<int, pair <int, int> > > char_pairs;
  for (int i = 0; i < inSceneData._descriptorIds.size(); i++)
  {
    int curr_id = inSceneData._descriptorIds[i];

    string desDataValue = desData[curr_id]._descriptions;
    
    bool matchFound = false;
    for(int j = 0; j < all_samples.size(); j++)
    {
      string fullValue = all_samples[j]._header + "\n" + all_samples[j]._description;
      size_t match_pos = fullValue.find(desDataValue);
      if(match_pos != string::npos)
      {
        all_samples[j]._descriptorIds.push_back(curr_id);
        char_pairs.push_back(make_pair(j, make_pair(match_pos, match_pos + desDataValue.size())));
        matchFound = true;
        break;
      }
    }

    if (!matchFound)
    {
      cout << " cannot break the scene, although breaks found, so using LCS approach " << endl;
      int maxlength = 0;
      int curr_push = -1;
      int match_pos = -1;
      int curr_match_pos = -1;
      string best_match_string = "";

      for(int j = 0; j < all_samples.size(); j++)
      {
        string fullValue = all_samples[j]._header + "\n" + all_samples[j]._description;
        string match_string = LongestCommonSubstring(desDataValue, fullValue, match_pos);
        
        if(maxlength <= match_string.length() && match_pos >= 0)
        {
          best_match_string = match_string;
          maxlength = match_string.length();
          curr_push = j;
          curr_match_pos = match_pos;
        }
      }
      cout << "The maximum lenght = " << maxlength << endl;
      if(maxlength >= 5)
      {
        cout << "retaining only max-match string" << endl;
        all_samples[curr_push]._descriptorIds.push_back(curr_id);
        char_pairs.push_back(make_pair(curr_push, make_pair(curr_match_pos, curr_match_pos + maxlength)));
        desData[curr_id]._descriptions = best_match_string;
        matchFound = true;        
      }
      else
      {
        noBreak = true;
      }
    }

  }

  if (noBreak)
  {
    all_samples.clear();
    all_samples.push_back(inSceneData);
    for (int i = 0; i < inSceneData._descriptorIds.size(); i++)
    {
      int curr_id = inSceneData._descriptorIds[i];
      desData[curr_id]._scene = 0;
    }

  }
  else
  {
    cout << "Scene broken into " << num_breaks << " breaks" << endl;
    for (int i = 0; i < inSceneData._descriptorIds.size(); i++)
    {
      int curr_id = inSceneData._descriptorIds[i];

      desData[curr_id]._scene = char_pairs[i].first;
      desData[curr_id]._beg_char = char_pairs[i].second.first;
      desData[curr_id]._end_char = char_pairs[i].second.second;
    }
   
  }

  return all_samples; 
}

int main(int argc, char **argv)
{

  /*
  SceneData test_scene(string("RAM"), string("ad BLAH DAY (209) advice 323 DAY IN FADE blah"));
  cout << "ORIGINAL STRING: " << test_scene._description << endl;

  for (int i = 0; i < all_samples.size(); i++)
  {
    cout << "header: " << all_samples[i]._header << " ____ descr: " << all_samples[i]._description << endl;
  }
 
  return 0;
  */

  if (argc < 6)
  {
    cout << "Minimum 2 inputs: " << "1. input description file, 2. input scene file, 3. keywords to look for in caps please" << endl;
  }

  ofstream outSceneFile(argv[4]);
  ofstream outAlignFile(argv[3]);


  string keyWordFile = argv[5];
  vector <string> keyWordsIn;

  ifstream inFile(keyWordFile.c_str());

  if (inFile.is_open())
  {
    // Read the line from file
    string line;
    while( getline(inFile, line))
    {
      keyWordsIn.push_back(line); 
    }
  }
  inFile.close();
   

  boost::unordered_map<int, SceneData>  outputScenes;
  int last_scene_num =  parseSceneBreaks(argv[2], outputScenes);

  vector <DescriptionData> outputDescriptions;
  parseDescriptionFile(argv[1], outputDescriptions, outputScenes);

  //last_scene_num = sceneBreaker(outputScenes, outputDescriptions, last_scene_num);

  vector <SceneData> finalSceneBreaks;
  

  for (int j = 0; j < keyWordsIn.size(); j++)
  {
    int ctr = 0;
    int scene_counter = 0;
    finalSceneBreaks.clear();

    string keyWord = keyWordsIn[j];
    for (boost::unordered_map<int, SceneData>::iterator it = outputScenes.begin(); it != outputScenes.end(); ++it)
    {
      cout << ctr++ << " , " ;
      vector <SceneData> newSceneData = getSceneBreaks(it->second, outputDescriptions, keyWord);

      for (int i = 0; i < it->second._descriptorIds.size(); i++)
      {
        int curr_id = it->second._descriptorIds[i];
        outputDescriptions[curr_id]._scene +=scene_counter;
      }
      
      finalSceneBreaks.insert(finalSceneBreaks.end(), newSceneData.begin(), newSceneData.end());
      scene_counter += newSceneData.size();

      //cout << it->second._description << endl << endl;
    }

    outputScenes.clear();
    for (int i = 0; i < finalSceneBreaks.size(); i++)
    {
      outputScenes[i] = finalSceneBreaks[i];
      for(int k = 0; k < outputScenes[i]._descriptorIds.size(); k++)
      {
        int curr_id = outputScenes[i]._descriptorIds[k];
        outputDescriptions[curr_id]._scene = i;
      }
    }
  }


  for (int i = 0; i<finalSceneBreaks.size(); i++)
  {
    outSceneFile << "---------------------- (" << i << ") -----------------------------------" << endl;
    outSceneFile << finalSceneBreaks[i]._header << "\n";
    outSceneFile << finalSceneBreaks[i]._description << endl;
    outSceneFile << endl;
  }
  outSceneFile.close();

  for (int i = 0; i < outputDescriptions.size(); i++)
  {
    outAlignFile << outputDescriptions[i]._time_beg << endl;
    outAlignFile << outputDescriptions[i]._time_duration << endl;
    outAlignFile << outputDescriptions[i]._descriptions << endl;
    outAlignFile << "Scene:" << outputDescriptions[i]._scene << " : (" << outputDescriptions[i]._beg_char << "," << outputDescriptions[i]._end_char  << ")" << endl << endl;

    cout << "Scene:" << outputDescriptions[i]._scene << " : (" << outputDescriptions[i]._beg_char << "," << outputDescriptions[i]._end_char  << ")" << endl << endl;

    //outAlignFile << endl;
  }

  outAlignFile.close();

  return 0;
}
