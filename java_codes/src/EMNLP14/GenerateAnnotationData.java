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
import java.io.InputStream;
import java.io.FileInputStream;

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

  static float overlapThreshold;
  static ArrayList <String> castList;


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
  
  public static void writeAnnotationConfFile(String annotationConfFile, 
      ArrayList<String> castList) throws IOException{
    
    BufferedWriter writerAnno = new BufferedWriter(new FileWriter(annotationConfFile));
    
    writerAnno.write("[entities]\n\n");
    writerAnno.write("Verb\n");
    writerAnno.write("Nom\n");
    writerAnno.write("FAIL\n");
    writerAnno.write("Other\n");

    String arg1 = "Verb|Nom";
    String arg2 = "Other";

    for (String castName : castList) {

      castName = castName.toLowerCase().replace(" " , "_");
      writerAnno.write(castName + "\n");
      arg2 = arg2 + "|" + castName;
    }

    writerAnno.write("\n[events]\n\n[relations]\n");

    writerAnno.write("A0 Arg1:" + arg1 + ", Arg2:" + arg2 + "\n");
    writerAnno.write("A1 Arg1:" + arg1 + ", Arg2:" + arg2 + "\n");
    writerAnno.write("A2 Arg1:" + arg1 + ", Arg2:" + arg2 + "\n");
    writerAnno.write("A3 Arg1:" + arg1 + ", Arg2:" + arg2 + "\n");
    writerAnno.write("A4 Arg1:" + arg1 + ", Arg2:" + arg2 + "\n");
    writerAnno.write("A5 Arg1:" + arg1 + ", Arg2:" + arg2 + "\n");
    writerAnno.write("A-Other Arg1:" + arg1 + ", Arg2:" + arg2 + "\n");

    writerAnno.write("\n[attributes]\n");

    writerAnno.close();
  }


  public static class PredicateWriter {
    
    HashMap <String, MentionForBrat> argumentMentions;
    int tokenCtr;
    Set <String> relationSet;
    int relationCtr;

    public void addPredicateArguments(PredicateArgumentView predArgs, 
      List<Mention> orderedMentions, int charOffset, 
      String verbOrNom, BufferedWriter writerAnno, 
      int begCharOffset, GoodOverlaps mentionOverlap) throws IOException{

      if (predArgs != null) {
       for (Constituent c : predArgs.getPredicates()) {
          
         // Skip the speaking instances
         if (c.getSurfaceString().equals("speaks") || 
             c.getSurfaceString().equals("dialogue"))
           continue;

         // Writing Predicate annotation
         int begChar = c.getStartCharOffset() + charOffset;
         int endChar = c.getEndCharOffset() + charOffset;
         writerAnno.write(String.format("T%d\t%s %d %d\t%s\n", 
               this.tokenCtr++, verbOrNom, begChar, endChar, c.getSurfaceString()));
         int sourceTokenId = this.tokenCtr-1;

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
                  if (!this.argumentMentions.containsKey(mentionKey)) {
                    MentionForBrat m1ForBrat =  new MentionForBrat(begChar, 
                        endChar, begCharMod, endCharMod, 
                        m1.headWord.originalText(), this.tokenCtr++);
                    m1ForBrat.corefTokenName = "Other";
                    this.argumentMentions.put(mentionKey, m1ForBrat);             

                    argumentTokenId = m1ForBrat.tokenId;
                  } else {
                    MentionForBrat m1ForBrat = this.argumentMentions.get(mentionKey);
                    argumentTokenId = m1ForBrat.tokenId;
                  }

                  String relationBratString = 
                    String.format("R%s_T%d_T%d",relName, sourceTokenId,
                      argumentTokenId);
                  if (!this.relationSet.contains(relationBratString)) {

                    writerAnno.write(String.format("R%d\t%s Arg1:T%d Arg2:T%d\n", 
                        this.relationCtr++, relName, sourceTokenId, argumentTokenId));
                    this.relationSet.add(relationBratString);
                  }
                }
              } // end of if bestMatch
            } // end of if argumentCheck
         } // end of for relationIteration
       } // end of for predicateGroup 
      }// end of checking if predGroupEmpty              

      //return tokenCtr;
    }
  };
 
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

    //float overlapThreshold = (float)0.1;

    try
    {
      // parse properties file
      Properties prop = new Properties();
      InputStream is = new FileInputStream("properties.xml");
      prop.loadFromXML(is);
      
      // minimum number of times a name should appear as a speaker in order to be 
      // considered as a valid cast-member
      int castThresh = Integer.parseInt(prop.getProperty("castListThreshold"));
      
      // overlap threshold between the mention and SRL arguments
      overlapThreshold = Float.parseFloat(prop.getProperty("overlapThreshold"));

      System.out.println("Overlap threshold = " + overlapThreshold);

      castList = new ArrayList<String>();
      castList = CastListParser.getCastFromSpeakerFile(castListFile, castThresh);
      String annotationConfFile = outDir + "/annotation.conf";
      writeAnnotationConfFile(annotationConfFile, castList);
      
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
            
            // To store the argument mentions which have been covered till now
            PredicateWriter predWriter = new PredicateWriter();
            predWriter.tokenCtr = 0;
            predWriter.relationCtr = 0;           
            predWriter.argumentMentions = new HashMap <String, MentionForBrat> ();
            predWriter.relationSet = new HashSet <String> ();

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

               // Setup the overlap identification class
               GoodOverlaps mentionOverlap = new GoodOverlaps(begCharOffset, 
                   overlapThreshold, orderedMentions, castList);


               PredicateArgumentView predArgs = null;
               PredicateArgumentView predArgsNom = null;

               boolean failFlag = false;               
               //int sourceTokenId;

               try {
                 
                 predArgs = curatorSRL.getPredicateArgumentsVerb(sentenceString);

               }catch (Exception e) {
                 System.out.println("SRL failed for sentence: " + sentenceString);
                 e.printStackTrace();

                 writerOut.write(sentenceString + "\n");
                 begChar = charOffset;
                 endChar = charOffset + sentenceString.length();
                 writerAnno.write(String.format("T%d\tFAIL %d %d\t%s\n", 
                      predWriter.tokenCtr++, begChar, endChar, sentenceString));  
                 charOffset += sentenceString.length() + 1;
                 System.out.println("Continuing ...");
                 failFlag = true;                 
               }
                
               if (failFlag)
                 continue;
            
              try {
                 
                 predArgsNom = curatorSRL.getPredicateArgumentsNom(sentenceString);

               }catch (Exception e) {
                 System.out.println("SRL failed for sentence: " + sentenceString);
                 e.printStackTrace();
                 failFlag = true;                 
               }

               writerOut.write(sentenceString + "\n");                
               
               // Enter things here
               predWriter.addPredicateArguments(predArgs, orderedMentions, 
                   charOffset, "Verb", writerAnno, 
                   begCharOffset, mentionOverlap);

               predWriter.addPredicateArguments(predArgsNom, orderedMentions, 
                   charOffset, "Nom", writerAnno, 
                   begCharOffset, mentionOverlap);

               charOffset += sentenceString.length()+1;
            }


            // Transfer coref labels to the arguments           
            HashMap<String, String> corefMentionValues = Utils.getCorefOfMentions(corefGraph, sentences);
            for (String mentionKey : corefMentionValues.keySet()) {
              if (predWriter.argumentMentions.containsKey(mentionKey)) {
                MentionForBrat argumentBratMention = predWriter.argumentMentions.get(mentionKey);
                System.out.println("COREF --------------");
                System.out.println("mention: " + argumentBratMention.headWord + 
                    ", coref: " + corefMentionValues.get(mentionKey));

                argumentBratMention.corefTokenName = 
                  Utils.getFromCastList(corefMentionValues.get(mentionKey), castList);
                predWriter.argumentMentions.put(mentionKey, argumentBratMention);

              }
            }

            for (MentionForBrat m1ForBrat : predWriter.argumentMentions.values() ) {
             
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
