/*
	Created by Vignesh Ramanathan and Armand Joulin, 2013.
	Compute the alignement between two texts
	Partialy use code from Sten Hjelmqvist (through wikipedia)
*/

#ifndef __TEXTALIGN_HPP__
#define __TEXTALIGN_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <algorithm> 
#include <boost/tokenizer.hpp>
#include <cmath>
#include "utils.hpp"

#define DEBUG
#define DRIFT_ALLOWED 2
#define LAMBDA 0.9
#define GAMMA 3.0
#define MAX_DRIFT(X) (X/2)
#define MIN_SUBLENGTH_FOR_DRIFT 10
#define SCORE_THRESH 0.7

using namespace std; 


namespace alignment
{
    
  /*
   * @brief: return 1 if two characters are equal
   *
   * @paramters:
   * char a - first character
   * char b - second character
   *
   * @output:
   * is a==b
   * */
  inline int isCharEqual(char a, char b)
  {
    return (a==b)? 1 : 0;
  }


  /*
   * @brief: returns the levenshtein distance between two strings (not commuatative)
   *
   * @parameters:
   * s - first string
   * t - secong string
   *
   * @output:
   * the integer distance giving number of changes to go from s to t
   * */
  static int levenshtein_distance(string s, string t)
  {
    // inspired by wikipedia (orginal by Hjelmqvist, Sten)

    // degenerate cases
    if (s == t) return 0;
    if (s.length() == 0) return t.length();
    if (t.length() == 0) return s.length();
    
    // create two work vectors of integer distances
    int v0[t.length() + 1];
    int v1[t.length() + 1];
    
    // initialize v0 (the previous row of distances)
    // this row is A[0][i]: edit distance for an empty s
    // the distance is just the number of characters to delete from t
    for (int i = 0; i < t.length() + 1; i++)
          v0[i] = i;
    
    for (int i = 0; i < s.length(); i++)
    {
        // calculate v1 (current row distances) from the previous row v0
    
        // first element of v1 is A[i+1][0]
        //   edit distance is delete (i+1) chars from s to match empty t
        v1[0] = i + 1;
    
        // use formula to fill in the rest of the row
        for (int j = 0; j < t.length(); j++)
        {
            int cost = (s[i] == t[j]) ? 0 : 1;
            v1[j + 1] = min(min(v1[j] + 1, v0[j + 1] + 1), v0[j] + cost);
        }
    
        // copy v1 (current row) to v0 (previous row) for next iteration
        for (int j = 0; j < t.length() + 1; j++)
            v0[j] = v1[j];
    }
    
    return v1[t.length()];
  }


  /*
   * @brief: return the normalized levenshtein by dividing by maximum string length
   *
   * @parameters:
   * s - first string
   * t - second string
   *
   * @output:
   * normalized Levenshtein distance
   *
   * */
  inline float normalized_levenshtein_distance(string s, string t)
  {
    return (max(s.length(), t.length()) != 0) ? ((float)levenshtein_distance(s, t)) / max(s.length(), t.length()) : 1;
  }

  /*
   * @brief: function to compute the time synchronized distance between two set of sequential strings
   *
   * @paramters:
   * sub - the subscript string vector which is to be matched to the script
   * script - the script contains the entries of the substirng "sub" in the same sequential order
   *          , but could be morphed are disturbed
   * distFunction - the distance function
   * subchain - (int, int) pair queue, where the final alignment will be store
   * isVerbose - shold the matching scores and final mathces be printed
   *
   * @output:
   * the integer distance between the sub and the script upon alignment
   * */

  static float getTimeSynchDistance(vector<string> sub, vector<string> script, float (*distFunction)(string, string), 
      deque< pair<int,int> > &subchain, bool isVerbose = true)
  {
    // compute the distances (not required)
    int N2 = sub.size();
    int N1 = script.size();

    if(N2==0 || N1==0)
      return 0;

    if(N2 > N1)
    {
      subchain.push_back(make_pair(0,0));
      return 1;
    }

    // Allocate arrays used by the method
    float **cumdists = new float*[N2];
    int **ancestors = new int*[N2];
    for(int i = 0; i<N2; i++)
    {
      cumdists[i] = new float[N1];
      ancestors[i] = new int[N1];
    }
    

#ifdef DEBUG
    if(isVerbose)
    {
      cout << "Starting forward pass ... " << endl;
    }
#endif

    for(int n2 = 0; n2 < N2; n2++)
    {
#ifdef DEBUG
      if (isVerbose)
      {  
        cout << "(" << n2 << "," << N2 << ")" << endl;
      }
#endif

      for(int n1 = n2; n1 < N1 - N2 + n2 + 1; n1++)
      {
        float maxcumdist = 0;
        int ancestor = -1;
        if( n2 > 0)
        {
          for(int n11 = n2 - 1; n11 < n1; n11++ )
            if(maxcumdist <= cumdists[n2-1][n11])
            {
              maxcumdist = cumdists[n2-1][n11];
              ancestor = n11;
            }
        }
        cumdists[n2][n1] = 1 - (*distFunction)(sub[n2], script[n1]) + maxcumdist;
        ancestors[n2][n1] = ancestor;
      }	
    }

#ifdef DEBUG    
    if (isVerbose)
    {
      cout << "Starting backward pass" << endl;
    }
#endif
    // backward pass: find best sub chain
    int n2 = N2-1, nb1 = -1;
    float maxcumdist = 0;
    for(int n1 = n2; n1 < N1 - N2 + n2 + 1; n1++)
      if(maxcumdist <= cumdists[n2][n1])
      {
        nb1 = n1;
        maxcumdist = cumdists[n2][n1];
      }
    // printing score

    if (isVerbose)
    {
      cout << "best score: "<< maxcumdist << endl;
    }

    // compute best pair of matches
    while( nb1 != -1)
    {
      //if( (1 - (*distFunction)(sub[n2], script[nb1])) != 0)
      //{
        subchain.push_front(make_pair(n2,nb1));
      //}
      
     
      nb1 = ancestors[n2][nb1];
      n2 = n2 - 1;
    }

    // print results:
    if (isVerbose)
    {
      for (deque<pair<int,int> >::iterator it = subchain.begin(); it != subchain.end(); ++it)
      {
        cout << it->first << " "<< it->second << " ( \"" << sub[it->first] <<"\" matched to \""<< script[it->second] <<"\" )"<< endl;
      }
    }

    // release the memory allocated
    for(int i = 0; i<N2; i++)
    {
      //*dists[i] = new float[N1];
      delete[] cumdists[i];
      delete[] ancestors[i];
    }
    delete[] cumdists;
    delete[] ancestors;

    // return the -(maxcumdist) as the distance measure    
    return ((sub.size() - maxcumdist)/sub.size());
  }


  /*
   * @brief: return the time synchronized distance between two vecotr of strings
   *
   * @paramters:
   * sub - the string which is to be found in the script
   * script - the string in which "sub" is to be identified
   * alignment - in addition to distance, it also gives the indices of the begining and end indices
   *        corresponding to the match of sub in script
   * */
  inline float timeSynchDistanceWithAlignment(vector <string> tokens_sub, vector <string> tokens_script, pair <int, int> &alignment)
  {

    if (tokens_sub.size() == 0 || tokens_script.size() == 0)
    {
      alignment.first = 0;
      alignment.second = 0;
      return 1;
    }

    deque <pair<int, int> > dummyQueue;

    /*
    cout << "\" ";
    for (int i = 0; i<tokens_sub.size(); i++)
    {
      cout << tokens_sub[i] << " ";
    }

    cout << "\" compared to \" " ;

    for (int i = 0; i < tokens_script.size(); i++)
    {
      cout << tokens_script[i] << " ";
    }
    cout << "\"" << endl;*/

    // time synch. distance which gives the levenstein distance based quality measure of matching
    float tsd = getTimeSynchDistance(tokens_sub, tokens_script, normalized_levenshtein_distance, dummyQueue, false);

    // beigning and ending indices of the match on the script
    int beginMatch, endMatch;

    // now compute a distance for the misalignment of the second string
    float space_separation = 0;
    if (tokens_sub.size() > 1)
    {
      int N2 = dummyQueue.size();
      endMatch = dummyQueue[N2-1].second;
      beginMatch = dummyQueue[0].second;
      float matchedExtent = (endMatch - beginMatch + 1);
      space_separation =  matchedExtent - tokens_sub.size();
      //cout << "me = " << space_separation << endl;
    }
    else
    {
      endMatch = dummyQueue[0].second;
      beginMatch = endMatch;
      space_separation = 0;
    }
    alignment.first = beginMatch;
    alignment.second = endMatch;

    // NOTE: lambda decides the tradeoff between space-misalignment and quality misalignment
    // gamma decides the extent to which you want to penalize the space misalignment, set gamma high to penalize heavily
    float space_distance = 1 - exp(-GAMMA*space_separation);
    return (LAMBDA*tsd + (1 - LAMBDA)*space_distance);

  }

  /*
   * @brief: return the time synchronized distance between two strings by tokenizing them,
   *         without providing the alignment information
   *
   * @paramters:
   * sub - the string which is to be found in the script
   * script - the string in which "sub" is to be identified
   * */
  inline float timeSynchDistance(string sub, string script)
  {
    // initialize tokenizers
    boost::tokenizer<> tokenizer_sub(sub);
    boost::tokenizer<> tokenizer_script(script);

    // tokenize the sub and the script into individual words
    vector<std::string> tokens_sub(tokenizer_sub.begin(), tokenizer_sub.end());
    vector<std::string> tokens_script(tokenizer_script.begin(), tokenizer_script.end());


    pair <int, int> dummy;
    return (timeSynchDistanceWithAlignment(tokens_sub, tokens_script, dummy));
  }

  /*
   * @brief: return the greedy time synchronized distance alignment between two vecotr of strings
   *
   * @paramters:
   * sub - the string which is to be found in the script
   * script - the string in which "sub" is to be identified
   * alignment - in addition to distance, it also gives the indices of the begining and end indices
   *        corresponding to the match of sub in script
   * */
  inline float greedyTimeSynchDist(vector <string> tokens_sub, vector <string> tokens_script, pair <int, int> &alignment)
  {

    if (tokens_sub.size() == 0 || tokens_script.size() == 0)
    {
      alignment.first = 0;
      alignment.second = 0;
      return 1;
    }

    deque <pair<int, int> > dummyQueue;

    // time synch. distance which gives the levenstein distance based quality measure of matching
    int N2 = tokens_sub.size();
    int N1 = tokens_script.size();

    if (N2 > N1)
    {
      alignment.first = 0;
      alignment.second = 0;
      return 1;
    }

    // Match the strings such that the last two words are aligned, then move in a greedy fashion 
    // matching the best available word, with a maximum allowance of N1-N2. This gap keeps reducing
    // as we move ahead and misalign some segment
    int allowed_drift = N1 - N2;
    float tsd = normalized_levenshtein_distance(tokens_script[N1-1], tokens_sub[N2-1]);
    
    int last_match = N1-1;
    for(int i = N2-2; i >= 0 ; i--)
    {
      int drift = 0;
      float minDist = 1;
      float matchId = (last_match - 1);
      // match with a mis alignment within the allowed limit
      for(int j = (last_match-1); j >= (last_match - allowed_drift -1); j--)
      {
        float dist_lev = normalized_levenshtein_distance(tokens_script[j], tokens_sub[i]);
        if (dist_lev < minDist)
        {
          // if matched with a misalignment, record the drift as well
          minDist = dist_lev;
          drift = (last_match - 1 - j);
          matchId = j;
        }
      }

      // reduce the allowed drift
      allowed_drift -= drift;
      tsd = tsd + minDist;
      last_match = (int)(matchId);
    }
    tsd = tsd/(N2); // normalize so that value is in [0,1]
    alignment.first = last_match;
    alignment.second = N1 - 1;
    
   
    // now compute a distance for the misalignment of the second string
    float space_separation = N1 - last_match;
  
    // NOTE: lambda decides the tradeoff between space-misalignment and quality misalignment
    // gamma decides the extent to which you want to penalize the space misalignment,  set gamma high to penalize heavily
    float space_distance = 1 - exp(-GAMMA*space_separation);
    return (LAMBDA*tsd + (1 - LAMBDA)*space_distance);

  }

   /*
   * @brief: function to compute the time synchronized distance between a set of sequential STRINGS a set of sequential WORDS
   *
   * @paramters:
   * sub - the subscript string vector which is to be matched to the script
   * scriptWords - the script contains the words of the substirng "sub" in the same sequential order
   *          , but could be morphed are disturbed
   * distFunction - the distance function
   * subchain - <i, pair<b_i, e_i> > pair queue, where the final alignment will be store
   *            sentence i in substring is matched to words between <b_i and e_i>
   * isVerbose - should the matching scores and final mathces be printed
   *
   * @output:
   * the distance between the sub and the script upon alignment
   * */

  static float getTimeSynchDistance(vector< vector<string> > sub, vector<string> scriptWords, 
      float (*distFunction)(vector <string>, vector <string>, pair <int, int>&), 
      deque< pair<int, pair<int,int> > > &subchain, deque <float> &matchScores, bool isVerbose = true)
  {

    // compute the distances (not required)
    int N2 = sub.size();
    int N1 = scriptWords.size();

    if(N2==0 || N1==0)
      return 0;

    float **cumdists = new float*[N2];
    int **ancestors = new int*[N2];
    float **scores = new float*[N2];
    pair <int, int> **matchIds = new pair <int, int> *[N2];
    for(int i = 0; i<N2; i++)
    {
      //*dists[i] = new float[N1];
      cumdists[i] = new float[N1];
      ancestors[i] = new int[N1];
      matchIds[i] = new pair <int, int> [N1];
      scores[i] = new float[N1];
    }
    
#ifdef DEBUG
    cout << "computing mathcing limits for each sentence ..." << endl;
#endif
    // Count total number of words in the subtitle
    int totalSubWordcount = 0;
    for(int i =0; i < N2; i++)
    {
      totalSubWordcount += sub[i].size();
    }

    // find the matching limits for each subtitle sentence
    vector < pair<int, int> > subMatchLimits; 
    
    // each  element is the minimum begining and maximum ending index allowed for the sentence
    int begCtr = 0;
    int endCtr = scriptWords.size() - totalSubWordcount - 1;
    for(int i = 0; i < N2; i++)
    {
      endCtr += sub[i].size();
      subMatchLimits.push_back(make_pair(begCtr, endCtr));
      begCtr += sub[i].size();
    }


#ifdef DEBUG
    cout << "Starting forward pass ... " << endl;
#endif

    /*
     * Forward pass to compute the energies
     * */
    for(int n2 = 0; n2 < N2; n2++)
    {
#ifdef DEBUG
      cout << "(" << n2 << "," << N2 << "): " << subMatchLimits[n2].first << "," << subMatchLimits[n2].second << " *********************************** " << endl;
#endif

      // for all elements below the lower limit, set the distance matrix values to zero, and ancestors to -1
      for(int n1 = 0; n1 < subMatchLimits[n2].first; n1++)
      {
        scores[n2][n1] = 0;
        cumdists[n2][n1] = 0;
        ancestors[n2][n1] = -1;
        matchIds[n2][n1] = make_pair(-1,-1);
      }

      float maxScore = 0;
      float maxcumdist = 0;
      int ancestor = -1;
      pair <int,int> matchIdLocal = make_pair(-1,-1);

      // compute the score for each element in the matrix using DP
      for(int n1 = subMatchLimits[n2].first; n1 <= subMatchLimits[n2].second; n1++)
      {

        float tempDist;
        pair <int, int> matchId;
        
        // try to match the subtitle[n2] to the script such that the last word of the subtitle matches before n1, 
        // and store the maximum score of this match in cumdists [n2][n1]
        for(int n11 = (int)(n1 - sub[n2].size() + 1 - DRIFT_ALLOWED); n11 <= (int)(n1); n11++)
        {
          if(n11<0)
            continue;
          
          // extract a subsegment of the script fromm starting from n11 to n1                   
          vector <string> tempVector;
          for(int k = n11; k <= n1; k++)
          {
            tempVector.push_back(scriptWords[k]);
            //cout << scriptWords[k] << " ";
          }
          
          /*cout << " \" compared to the sub: \" ";

          for(int k = 0; k < sub[n2].size(); k++)
          {
            cout << sub[n2][k] << " ";
          }
          cout << endl;*/
         
          // compute the matching score between the subtitle and the sub-segment of the script
          if(sub[n2].size() <= tempVector.size())
          {
            tempDist = 1 - (*distFunction)(sub[n2], tempVector, matchId);
          }
          else if(sub[n2].size() <= (tempVector.size() + MAX_DRIFT(sub[n2].size())) && sub[n2].size() > MIN_SUBLENGTH_FOR_DRIFT) 
            // allowing for the fact that the subtitle string can sometime miss few words
          {
            tempDist = 1 - (*distFunction)(tempVector, sub[n2], matchId) - 0.1; // the 0.1 is a slight penalty for this mismatch
            matchId.first = 0; matchId.second = (tempVector.size() - 1);         
          }
          else // if subtitle is really big compared to the script sub-string, then set tempDist to be samll
          {
            tempDist = 0;
            matchId.first = 0; matchId.second = 0;
          }
          //cout << tempDist;
          

          
          // the score at a match is the tempDist + score of matching the previous subtitle before n11

          if (n2 > 0)
          {
            if(n11 > 0)
            {
              if ((tempDist + cumdists[n2-1][n11-1]) >= maxcumdist)
              {
                maxcumdist = (tempDist + cumdists[n2-1][n11-1]);
                ancestor = n11-1;
                // matchIds keep track of the poisitions of matching the script statement to the subtitle
                matchIdLocal = make_pair(matchId.first + n11, matchId.second + n11);
                maxScore = tempDist;
              }
            }
          }
          else
          {
            // for the first statement in subtitle, intialize
            if(tempDist >= maxcumdist)
            {
              maxcumdist = tempDist;
              ancestor = -1;
              matchIdLocal = make_pair(matchId.first + n11, matchId.second + n11);
              maxScore = tempDist;
            }
          }

        }
        // set the maximum value to cumulative distance
        cumdists[n2][n1] = maxcumdist;
        ancestors[n2][n1] = ancestor;
        matchIds[n2][n1] = matchIdLocal;
        scores[n2][n1] = maxScore;

      }

      // for all the elements outside the range, set the value to the maxvalue till now
      for(int n1 = (subMatchLimits[n2].second + 1); n1 < scriptWords.size(); n1++)
      {
        cumdists[n2][n1] = maxcumdist;
        ancestors[n2][n1] = ancestor;
        matchIds[n2][n1] = matchIdLocal;
        scores[n2][n1] = maxScore;
      }
      
      /*
      for(int verb_ct = 0; verb_ct < scriptWords.size(); verb_ct++)
        cout << "(" << matchIds[n2][verb_ct].first << "," << matchIds[n2][verb_ct].second << ") , ";
      cout << endl;
      */
    }

    if (isVerbose)
    {
      cout << "Starting backward pass" << endl;
    }

    // backward pass: find best sub chain
    int n2 = N2-1, nb1 = -1;
    float maxcumdist = 0;
    for(int n1 = subMatchLimits[n2].first; n1 <= subMatchLimits[n2].second; n1++)
      if(maxcumdist < cumdists[n2][n1])
      {
        nb1 = n1;
        maxcumdist = cumdists[n2][n1];
      }
    // printing score

    if (isVerbose)
    {
      cout << "Best Score: "<< maxcumdist << endl;
    }

    // Push the score of the best match for each sentence
    matchScores.push_front(scores[n2][nb1]);


    // compute best pair of matches
    while( nb1 != -1)
    {
      cout << "(" << n2 << "," << nb1 << ")" << endl;
      pair <int, int> temp = make_pair(matchIds[n2][nb1].first, matchIds[n2][nb1].second);
      subchain.push_front(make_pair(n2,temp));
      matchScores.push_front(scores[n2][nb1]); //score for best match
      nb1 = ancestors[n2][nb1];
      n2 = n2 - 1;
    }
    
    /*
    n2++;
    while(n2 >= 0)
    {
      pair <int, int> temp = make_pair(0,0);
      subchain.push_front(make_pair(n2, temp));
      n2--;
    }*/

    // print results:
    
    if (isVerbose)
    {
      int scoreCtr = 0;
      for (deque< pair<int, pair<int, int> > >::iterator it = subchain.begin(); it != subchain.end(); ++it)
      {
        cout << it->first << ", "<< (it->second).first << " ... " << (it -> second).second << " score = " 
          << matchScores[scoreCtr++] << " " <<  endl;
        cout << "\" ";
        for (vector <string>::iterator it2 = sub[it -> first].begin(); it2 != sub[it -> first].end(); it2++)
        {
          cout << *it2 << " ";
        }
        cout << "\" matched to \" ";
        for (int i = (it -> second).first; i <= (it -> second).second; i++)
        {
          cout << scriptWords[i] << " ";
        }
        cout << "\"" << endl;
      }
    }

    // release the memory allocated
    for(int i = 0; i < N2; i++)
    {      
      delete[] cumdists[i];
      delete[] ancestors[i];
      delete[] matchIds[i];
      delete[] scores[i];
    }
    delete[] cumdists;
    delete[] ancestors;
    delete[] matchIds;
    delete[] scores;

    // note that this is the matching distance, hence -score
    return ((N2 - maxcumdist)/N2);
  }

}

#endif
