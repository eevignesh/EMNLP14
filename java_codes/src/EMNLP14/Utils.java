
package EMNLP14;

import java.util.List;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.util.StringTokenizer;


import edu.stanford.nlp.dcoref.*;
import edu.stanford.nlp.util.*;
import edu.stanford.nlp.ling.CoreLabel;
import edu.stanford.nlp.ling.IndexedWord;
import edu.stanford.nlp.ling.CoreAnnotations.*;
import edu.stanford.nlp.dcoref.CorefChain.CorefMention;
import edu.stanford.nlp.trees.tregex.*;
import edu.stanford.nlp.trees.Tree;

import java.io.File;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.FileReader;


class Utils {

  /* deprecate (code from Armand) */
  private static String sentence2string(CoreMap sentence){
    List<CoreLabel> tokens = sentence.get(TokensAnnotation.class);
    String stc ="";
    for( CoreLabel token : tokens)
    stc+= token.word() + " ";
    return stc;
  }

  /* Getting the levenshtein distance between strings */
  public static class LevenshteinDistance {
    
    private static int minimum(int a, int b, int c) {
      return Math.min(Math.min(a, b), c);
    }
   
    public static int computeLevenshteinDistance(String str1,String str2) {
      int[][] distance = new int[str1.length() + 1][str2.length() + 1];
   
      for (int i = 0; i <= str1.length(); i++)
        distance[i][0] = i;
      for (int j = 1; j <= str2.length(); j++)
        distance[0][j] = j;
   
      for (int i = 1; i <= str1.length(); i++)
        for (int j = 1; j <= str2.length(); j++)
          distance[i][j] = minimum(
              distance[i - 1][j] + 1,
              distance[i][j - 1] + 1,
              distance[i - 1][j - 1]+ ((str1.charAt(i - 1) == str2.charAt(j - 1)) ? 0 : 1));
   
      return distance[str1.length()][str2.length()];    
    }
  }

  /* Check if a mentions is human and not possesive */
  public static boolean isHumanNotPossesive(Mention m1, List <String> castList) {

    boolean isHuman;
    boolean isPossesive = false;

    // Check if the mention is animate, else neglect
    if (m1.animacy == Dictionaries.Animacy.ANIMATE) {
      isHuman =  true;
    } else {
      isHuman = false;
    }

    if (m1.headIndexedWord.tag().equals("PRP$")) {
      isPossesive = true;
    } else {
      for (IndexedWord iw : m1.dependency.descendants(m1.headIndexedWord)) {
        if (iw.tag().equals("POS")) {
          isPossesive = true;
          System.out.println("mention poss: " + m1.toString());
        }
          
      } 
    }

    // Check if the mention is possesive (Jon's bottle or his bottle)
    String overlap = getFromCastList(m1.headWord.originalText(), castList);
    if (!overlap.toLowerCase().equals("other")) {
      isHuman = true;
    }

    return (isHuman & !(isPossesive));

  }

  /* Check if an String is human and not possesive */
  public static boolean isWordHuman(String word, List <String> castList, Dictionaries dict) {

    // Check if the mention is animate, else neglect
    boolean isHuman = dict.animateWords.contains(word) 
      | dict.animatePronouns.contains(word);
    

    // Check if the mention is possesive (Jon's bottle or his bottle)
    String overlap = getFromCastList(word, castList);
    boolean inCastList = false;
    if (!overlap.toLowerCase().equals("other")) {
      inCastList = true;
    }

    return (isHuman | inCastList);

  }


  /* Get the best matching cast name form a list of strings */
  public static String getFromCastList(String mentionString, List <String> castList) {
    
    String personName = "Other";
    mentionString = mentionString.toLowerCase();

    for ( String castName : castList) {
      
      int minDist = LevenshteinDistance.computeLevenshteinDistance(castName, mentionString);
      
      StringTokenizer st = new StringTokenizer(castName.toLowerCase());
      while (st.hasMoreTokens()) {
        minDist = Math.min(minDist,
          LevenshteinDistance.computeLevenshteinDistance(st.nextToken(), mentionString));
      }

      if ((minDist <= 1 && castName.length() > 4) || (minDist==0) ) {
        personName = castName.toLowerCase();
        personName = personName.replace(' ', '_');
        break;
      }
    }
    
    return personName;
  }

  /* Parse a text file to get the list of castnames */
  public static ArrayList<String> parseCastList(String castListFile) throws Exception{
    
    BufferedReader reader   = new BufferedReader( new FileReader (castListFile));
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

  /* Parse a text file to extract the complte text as it is from the file */
  public static String getFullTextFromFile( String file ) throws IOException {
      BufferedReader reader   = new BufferedReader( new FileReader (file));
      String         line     = null;
      String         fullText = "";

      while( ( line = reader.readLine() ) != null ) {
        fullText += line + "\n";                            
      }

      return fullText;
	}

  /* Get the set of good-overlapping mentions with certain spans in a sentence */
  public static class GoodOverlaps {

    public int bestMatch;
    float bestOverlap;
    int begCharOffset;
    float overlapThreshold;
    List <Mention> orderedMentions;
    ArrayList <String> castList;

    public GoodOverlaps(int bco, float ot, List <Mention> om, ArrayList<String> cl) {
      begCharOffset = bco;
      overlapThreshold = ot;
      orderedMentions = om;
      castList = cl;
    }

    public void setOverlapThreshold(float ot) {
      overlapThreshold = ot;
    }

    public ArrayList<Integer> getGoodOverlapMentions(int begChar, int endChar, 
                        boolean checkIfHuman) {
        this.bestOverlap = 0;
        this.bestMatch = -1;

        ArrayList <Integer> goodOverlaps = new ArrayList <Integer> ();

        for (int mentionI = 0; mentionI < this.orderedMentions.size(); mentionI++) {
          
          Mention m1 = this.orderedMentions.get(mentionI);
          int begCharMention = m1.headWord.beginPosition() - this.begCharOffset;
          int endCharMention = m1.headWord.endPosition() - this.begCharOffset;
          float overlap = -1;

          if (endCharMention > begChar && begCharMention <= begChar) {
            overlap = (float)(Math.min(endCharMention, endChar) 
                      - begChar)/
                      (float)(Math.max(endCharMention, endChar) 
                          - begCharMention);
          } else if (endChar > begCharMention && begChar <= begCharMention) {
            overlap = (float)(Math.min(endCharMention, endChar) - begCharMention)/
                      (float)(Math.max(endCharMention, endChar) - begChar);
          }

          boolean isHuman = true;

          if (checkIfHuman)
            isHuman = isHumanNotPossesive(m1, this.castList);

          if (overlap > this.bestOverlap && isHuman) {
            this.bestMatch = mentionI;
            this.bestOverlap = overlap;
          }

          if (overlap > this.overlapThreshold && isHuman) {
            goodOverlaps.add(mentionI);
          }

        }
        return goodOverlaps; 
    }
  }

  /* Transfer coref-labels based on the coref-chain */
  public static HashMap<String, String> getCorefOfMentions (Map<Integer, CorefChain> corefGraph,
          List<CoreMap> sentences) {
    HashMap<String, String> mentionCorefValues = new HashMap<String, String> ();

    for (CorefChain corefValue : corefGraph.values()) {
      List<CorefMention> mentions = corefValue.getMentionsInTextualOrder();
      for( CorefMention cc : mentions) {

          CorefMention mentionRepresentative = corefValue.getRepresentativeMention();
          CoreLabel reprWordOfMention = sentences.
              get(mentionRepresentative.sentNum-1).get(TokensAnnotation.class).
              get(mentionRepresentative.headIndex - 1);
          CoreLabel headWordOfMention = sentences.
              get(cc.sentNum-1).get(TokensAnnotation.class).get(cc.headIndex-1);

          int begCharCoref = headWordOfMention.beginPosition();
          int endCharCoref = headWordOfMention.endPosition();                 
          
          String mentionKey = String.format("%d_%d", begCharCoref, endCharCoref);
          System.out.println("coref (" + headWordOfMention.originalText() + 
                ") --- key: " + mentionKey);
          mentionCorefValues.put(mentionKey, reprWordOfMention.originalText());
      }         
    }
    return mentionCorefValues;
  }

  /* Parse  a .ann file to get the verbs and the arguments */

  public static class BratAnnotationTokens {

    int tokenId;
    String tokenType;
    int begChar;
    int endChar;
    String originalText;
    boolean isPassive;

    public BratAnnotationTokens(int ti, String tt, int bc, int ec, String ot) {
      tokenId = ti;
      tokenType = tt;
      begChar = bc;
      endChar = ec;
      originalText = ot;
      isPassive = false;
    }

  };

  public static class BratAnnotationRelations {


  };

  public static ArrayList<BratAnnotationTokens> parseAnnotationFile 
    (String annotationFileName) throws IOException{

    BufferedReader reader = new BufferedReader( new FileReader (annotationFileName));
    ArrayList <BratAnnotationTokens> bratTokens  = 
      new ArrayList <BratAnnotationTokens>();
    String line;
    while ( ( line = reader.readLine()) != null) {
    
      // parse the token line
      if (line.startsWith("T")) {
        //System.out.println(line);
        String[] splits = line.split("\\s+", 5);
        int tokenId = Integer.parseInt(splits[0].substring(1));
        String tokenType = splits[1];
        int begChar = Integer.parseInt(splits[2]);
        int endChar = Integer.parseInt(splits[3]);
        String originalText = splits[4];

        BratAnnotationTokens bt = new BratAnnotationTokens(tokenId, tokenType,
            begChar, endChar, originalText);
        bratTokens.add(bt);
      }
    }

    reader.close();
    return bratTokens;

  }

  public static HashMap <String, BratAnnotationTokens>
    voiceExtractor (Tree root, HashMap <String, BratAnnotationTokens> bratVerbs) {

    boolean value = false;
    String tgrepPatternStr = "VP < VBN|VBD > (VP|SQ < (/^(VB|AUX)/ < /be|was|is|are|were|been|being|'s|'re|'m|am|Been|Being|WAS|IS|get|got|getting|gets|Get|gotten|become|became|felt|feels|feel|seems|seem|seemed|remains|remained|remain/))";
    TregexPattern tgrepPattern = TregexPattern.compile(tgrepPatternStr);
    System.out.println(" in voice extractor ----------------------------------->  " + root.size());
    for (Tree node : root.getLeaves()) {
      //System.out.println("***");
      String bratkey;
      CoreLabel cl = (CoreLabel) node.label();
      bratkey = String.format("%d_%d", cl.beginPosition(), cl.endPosition());
      //System.out.println("brat key (" + node.value() + ") - " + bratkey);
      if (bratVerbs.containsKey(bratkey)) {
          BratAnnotationTokens bt = bratVerbs.get(bratkey);   
          Tree predNode = node;
          System.out.println("found the verb : " + bt.originalText);

          try {
          
            value = false;
            Tree parentTree = predNode.parent(root);
            Tree greatParentTree = parentTree.parent(root);
            TregexMatcher m = tgrepPattern.matcher(greatParentTree);  // tgrepPattern.matcher(root);
            while (m.find()) {
              if (m.getMatch().equals(parentTree)) {
                value = true;
                System.out.println("(" + node.value() + ") is passive");
              }
            }
            bt.isPassive = value;
            bratVerbs.put(bratkey, bt);          
          } catch (Exception e) {
            System.out.println("Error With Tree:");
            root.pennPrint();
            System.out.println("PredNode:");
            predNode.pennPrint();
            e.printStackTrace();
          }
      }
    
    }

    return bratVerbs;
  }

};
