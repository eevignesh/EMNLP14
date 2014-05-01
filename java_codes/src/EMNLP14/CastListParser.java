/*
Parse scene data to obtain mentions, coreNLP coreference,
transfer ground-truth coreference and use SRL from 
Illinois curator
Written by Vignesh, 2014
*/

package EMNLP14;

import java.util.Properties;
import java.util.List;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

import java.io.File;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.InputStream;
import java.io.FileInputStream;


class CastListParser{

  public static ArrayList <String> getCastFromSpeakerFile(String castListFile, int castThresh) throws Exception {
    
    BufferedReader reader        = new BufferedReader( new FileReader (castListFile));
    ArrayList <String> castList  = new ArrayList <String>();
    HashMap <String, Integer> castCount = new HashMap <String, Integer>();
    String line;
    
    Pattern pattern_caps = Pattern.compile("([A-Z\\.]+)");
    Pattern pattern_brac = Pattern.compile("(\\(.*\\))");

    while ( ( line = reader.readLine()) != null) {
      
      Matcher matcher_brac = pattern_brac.matcher(line);
      String newLine;
      if (matcher_brac.find()) {
        newLine = matcher_brac.replaceAll("");
      } else {
        newLine = line;
      }
      
      Matcher matcher_caps = pattern_caps.matcher(newLine);
      String name = "";
      int group_id = 0;
      while (matcher_caps.find()) {   
        name += matcher_caps.group() + " ";
      }

      if (name.length() > 1) {
        name = name.substring(0, name.length()-1);
        if (castCount.containsKey(name)) {
          castCount.put(name, castCount.get(name)+1);
        } else {
          castCount.put(name, 1);
        }
      }

    }

    System.out.println("The cast list:");

    for (String castName : castCount.keySet()) {
      //System.out.println("Person (" + castName + ") : " + castCount.get(castName));
      if (castCount.get(castName) > castThresh) {
        castList.add(castName);
      }
    }

    reader.close();
    return castList;
  }

  // Parse a simple text file containing the entire list
  public static ArrayList <String> parseCastList(String castListFile) throws Exception{
    
    BufferedReader reader        = new BufferedReader( new FileReader (castListFile));
    ArrayList <String> castList  = new ArrayList <String>();
    String line;
    while ( ( line = reader.readLine()) != null) {
      
      if (line.endsWith(".")) {
        line = line.substring(0, line.length()-1);
      }
      castList.add(line);

    }

    reader.close();
    return castList;
  }

}
