/*
Parse scene data to obtain mentions, coreNLP coreference,
transfer ground-truth coreference and use SRL from 
Illinois curator
Written by Vignesh, 2014
*/

package EMNLP14;

import java.util.Properties;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.HashSet;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicReference;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.util.StringTokenizer;

import java.lang.Math;

import edu.illinois.cs.cogcomp.edison.sentences.PredicateArgumentView;
import edu.illinois.cs.cogcomp.edison.sentences.Constituent;
import edu.illinois.cs.cogcomp.edison.sentences.Relation;


import java.io.File;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.FileWriter;

import edu.stanford.nlp.pipeline.*;
import edu.stanford.nlp.dcoref.*;
import edu.stanford.nlp.util.*;
import edu.stanford.nlp.ling.CoreLabel;
import edu.stanford.nlp.ling.CoreAnnotations.*;
import edu.stanford.nlp.trees.*;
import edu.stanford.nlp.trees.TreeCoreAnnotations.*;
import edu.stanford.nlp.trees.TreeCoreAnnotations.*;
import edu.stanford.nlp.dcoref.CorefCoreAnnotations.CorefChainAnnotation;
import edu.stanford.nlp.dcoref.CorefChain.CorefMention;

import edu.stanford.nlp.semgraph.SemanticGraph;
import edu.stanford.nlp.semgraph.SemanticGraphCoreAnnotations.CollapsedCCProcessedDependenciesAnnotation;
import edu.stanford.nlp.semgraph.SemanticGraphEdge;
import edu.stanford.nlp.ling.IndexedWord;
import edu.stanford.nlp.trees.GrammaticalRelation;

import edu.stanford.nlp.util.Pair;

import EMNLP14.Utils.*;

class GenerateAnnotationData{
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

  private static String sentence2string(CoreMap sentence){
    List<CoreLabel> tokens = sentence.get(TokensAnnotation.class);
    String stc ="";
    for( CoreLabel token : tokens)
    stc+= token.word() + " ";
    return stc;
  }

  private static boolean isHumanNotPossesive(Mention m1, List <String> castList) {

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

    /*System.out.println("mention: " + m1.headWord + " from list: " +  overlap);
    System.out.println(isHuman);
    System.out.println(!(isPossesive));*/

    return (isHuman & !(isPossesive));

  }

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

      if (minDist <= 1) {
        personName = castName.toLowerCase();
        personName = personName.replace(' ', '_');
        break;
      }
    }
    
    return personName;
  }


  public static class MentionForBrat {
    //Mention m;
    int begCharDocument;
    int endCharDocument;
    int begCharBrat;
    int endCharBrat;
    String headWord;
    int tokenId; 
    
    String corefTokenName;

    public MentionForBrat(int bcd, int ecd, int bcb, int ecb, String hw, int ti) {
      begCharDocument = bcd;
      endCharDocument = ecd;
      begCharBrat     = bcb;
      endCharBrat     = ecb;
      headWord        = hw;
      tokenId         = ti;
    }
  };  

 
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

/**********************************************************
*                       MAIN
**********************************************************/


  public static void main(String[] args){
    System.out.println("Starting scene parser");

    // coreNLP stuff
    Properties props = new Properties();
    props.put("annotators", "tokenize, ssplit, pos, lemma, truecase, ner, parse, dcoref");
    StanfordCoreNLP pipeline = new StanfordCoreNLP(props);

    // handling input:
    String namefile  = args[0];
    String outDir    = args[1];
    String verbOrNom = args[2];
    String castListFile = args[3];

    // overlap threshold between the mention and SRL arguments
    float overlapThreshold = (float)0.1;

    try
    {
      ArrayList<String> castList = parseCastList(castListFile);
      SceneParser sceneParser = new SceneParser(namefile);
      //String configFile       = "srl-config.properties";
      CuratorSRLClassifier curatorSRL = new CuratorSRLClassifier();
      curatorSRL.setupClient("jackson.stanford.edu", "9010");

      for (int sceneId : sceneParser.sceneData.keySet()) {
          if (sceneId == 0) {
            System.out.println("Skip scene 0\n");
            continue;
          }

          /*if (sceneId != 11) {
            continue;
          }*/

          String sceneText = sceneParser.sceneData.get(sceneId);
          Annotation document = new Annotation(sceneText);
          //Annotation document_sieve = new Annotation(text);
          // run all Annotators on this text
          pipeline.annotate(document);

          // these are all the sentences in this document
          // a CoreMap is essentially a Map that uses
          // class objects as keys and has values with custom types
          List<CoreMap> sentences = document.get(SentencesAnnotation.class);
          Map<Integer,CorefChain> corefGraph     = document.get(CorefChainAnnotation.class);


          List<List<CoreLabel>> words = new ArrayList< List <CoreLabel>>();
          List<Tree> trees = new ArrayList<Tree>();

          for (CoreMap s : sentences)
          {
            words.add(s.get(TokensAnnotation.class));
            trees.add(s.get(TreeAnnotation.class));
          }

          try
          {
            Dictionaries dict = new Dictionaries();
            RuleBasedCorefMentionFinder ruleMentionFinder = new RuleBasedCorefMentionFinder();
            List <List <Mention> > mentionList = ruleMentionFinder.extractPredictedMentions(document,
                                                 -1, dict);
                     
            //Semantics semantic = new Semantics();
            MentionExtractor menext = new MentionExtractor(dict, null);
            List <List <Mention>> newMentionList = menext.arrange(document, words, 
                      trees, mentionList, false);

            Document doc_sieve = new Document(document, newMentionList, null, dict);
            List <List <Mention> > orderedMentionsBySentence = doc_sieve.getOrderedMentions();
            int numSentences = orderedMentionsBySentence.size();
            int charOffset = 0, begChar = 0, endChar = 0;

            // Write the output file in Brat format
            File outFile  = new File(outDir + String.format("/scene_%04d.txt", sceneId));
            File annoFile = new File(outDir + String.format("/scene_%04d.ann", sceneId));

            BufferedWriter writerOut  = new BufferedWriter(new FileWriter(outFile));
            BufferedWriter writerAnno = new BufferedWriter(new FileWriter(annoFile));
            int tokenCtr = 0, relationCtr = 0;
            
            // To store the argument mentions which have been covered till now
            HashMap <String, MentionForBrat> argumentMentions = new HashMap <String, MentionForBrat> ();
            Set <String> relationSet = new HashSet <String> ();

            for (int sentI = 0; sentI < numSentences; sentI++) {

               List<Mention> orderedMentions = orderedMentionsBySentence.get(sentI);
               // Run the Curator SRL to get the predicate argument list
               //String sentenceString = sentence2string(sentences.get(sentI));
               String sentenceString = sentences.get(sentI).get(TextAnnotation.class);
               List<CoreLabel> sentenceTokens = sentences.get(sentI).get(TokensAnnotation.class);

               // account for multiple spaces at sentence begining
               int begCharOffset = 0;
               if (sentenceTokens.size() > 0)
                 begCharOffset = sentenceTokens.get(0).beginPosition() - charOffset;

               GoodOverlaps mentionOverlap = new GoodOverlaps(begCharOffset, 
                   overlapThreshold, orderedMentions, castList);

               PredicateArgumentView predArgs = null;
               
               boolean failFlag = false;               
               int sourceTokenId;

               try {
                 predArgs = curatorSRL.getPredicateArgumentsVerb(sentenceString);
               }catch (Exception e) {
                 System.out.println("SRL failed for sentence: " + sentenceString);
                 writerOut.write(sentenceString + "\n");
                 begChar = charOffset;
                 endChar = charOffset + sentenceString.length();
                 writerAnno.write(String.format("T%d\tFAIL %d %d\t%s\n", 
                      tokenCtr++, begChar, endChar, sentenceString));  
                 charOffset += sentenceString.length() + 1;
                 System.out.println("Continuing ...");
                 failFlag = true;                 
               }
                
               if (failFlag)
                 continue;
            
               writerOut.write(sentenceString + "\n");                
               if (predArgs != null) {
                 for (Constituent c : predArgs.getPredicates()) {
                   
                   // Writing Predicate annotation
                   begChar = c.getStartCharOffset() + charOffset;
                   endChar = c.getEndCharOffset() + charOffset;
                   writerAnno.write(String.format("T%d\t%s %d %d\t%s\n", 
                         tokenCtr++, verbOrNom, begChar, endChar, c.getSurfaceString()));
                   sourceTokenId = tokenCtr-1;

                   // Writing argument annotation
                   for (Relation r : predArgs.getArguments(c)) {
                      begChar = r.getTarget().getStartCharOffset() + charOffset;
                      endChar = r.getTarget().getEndCharOffset() + charOffset;
                      String relName = r.getRelationName();
                      
                      if (relName.equals("A0") || 
                          relName.equals("A1") || 
                          relName.equals("A2") || 
                          relName.equals("A3") ) {

                        // Iterate through mentions to find the best match 
                        // and annotate if it is animate or in the cast list
                        int bestMatch = -1;
                        ArrayList <Integer> goodOverlaps = mentionOverlap.getGoodOverlapMentions(
                            begChar, endChar, true);
                        bestMatch = mentionOverlap.bestMatch;


                        System.out.println( "Pred: " + c.getSurfaceString() + 
                                            " Arg: " + r.getTarget().toString() + 
                                            " best-match: " + bestMatch);

                        if (bestMatch >= 0) {

                          //Mention m1 = orderedMentions.get(bestMatch);

                          //int mentionId = -1;

                          for (int mentionId : goodOverlaps) {
                            Mention m1 = orderedMentions.get(mentionId);

                            begChar = m1.headWord.beginPosition();
                            endChar = m1.headWord.endPosition();
                            System.out.println(m1.animacy);
                            System.out.println(m1.headWord + 
                                ", beg --- " + m1.headWord.beginPosition() +
                                " , end --- " + m1.headWord.endPosition());
                            int begCharMod = begChar - begCharOffset;
                            int endCharMod = endChar - begCharOffset;
                            

                            String mentionKey = String.format("%d_%d", begChar, endChar);
                            int argumentTokenId = -1;
                            if (!argumentMentions.containsKey(mentionKey)) {
                              MentionForBrat m1ForBrat =  new MentionForBrat(begChar, 
                                  endChar, begCharMod, endCharMod, 
                                  m1.headWord.originalText(), tokenCtr++);
                              m1ForBrat.corefTokenName = "Other";
                              argumentMentions.put(mentionKey, m1ForBrat);             
                              /*writerAnno.write(String.format("T%d\tArgument %d %d\t%s\n", 
                               m1ForBrat.tokenId, m1ForBrat.begCharBrat, 
                               m1ForBrat.endCharBrat, m1ForBrat.headWord));*/
                              argumentTokenId = m1ForBrat.tokenId;
                            }
                            else {
                              MentionForBrat m1ForBrat = argumentMentions.get(mentionKey);
                              argumentTokenId = m1ForBrat.tokenId;
                            }

                            String relationBratString = 
                              String.format("R%s_T%d_T%d",relName, sourceTokenId,
                                argumentTokenId);
                            if (!relationSet.contains(relationBratString)) {

                              writerAnno.write(String.format("R%d\t%s Arg1:T%d Arg2:T%d\n", 
                                  relationCtr++, relName, sourceTokenId, argumentTokenId));
                              relationSet.add(relationBratString);
                            }
                          }
                        } // end of if bestMatch
                      } // end of if argumentCheck
                   } // end of for relationIteration
                 } // end of for predicateGroup 
               }// end of checking if predGroupEmpty              

               charOffset += sentenceString.length()+1;
            }


            // Transfer coref labels to the arguments          
            HashMap<String, String> corefMentionValues = Utils.getCorefOfMentions(corefGraph, sentences);
            for (String mentionKey : corefMentionValues.keySet()) {
              if (argumentMentions.containsKey(mentionKey)) {
                MentionForBrat argumentBratMention = argumentMentions.get(mentionKey);
                System.out.println("COREF --------------");
                System.out.println("mention: " + argumentBratMention.headWord + 
                    ", coref: " + corefMentionValues.get(mentionKey));

                argumentBratMention.corefTokenName = 
                  getFromCastList(corefMentionValues.get(mentionKey), castList);
                argumentMentions.put(mentionKey, argumentBratMention);

              }
            }            


            for (MentionForBrat m1ForBrat : argumentMentions.values() ) {
             
              writerAnno.write(String.format("T%d\t%s %d %d\t%s\n", 
              m1ForBrat.tokenId, m1ForBrat.corefTokenName, m1ForBrat.begCharBrat, 
              m1ForBrat.endCharBrat, m1ForBrat.headWord));
            }

            writerOut.close();
            writerAnno.close();

          } catch (Exception e) {
            e.printStackTrace();
          }// end inner try-catch
      }// end loop through scenes
    } catch(Exception e) {
      e.printStackTrace();
    }// end outter try-catch

  }// end main

}
