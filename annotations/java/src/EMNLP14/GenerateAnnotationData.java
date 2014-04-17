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
import java.util.Arrays;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicReference;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

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

class GenerateAnnotationData{
// printing options

  private static String sentence2string(CoreMap sentence){
    List<CoreLabel> tokens = sentence.get(TokensAnnotation.class);
    String stc ="";
    for( CoreLabel token : tokens)
    stc+= token.word() + " ";
    return stc;
  }

  public static class MentionForBrat {
    //Mention m;
    int begCharDocument;
    int endCharDocument;
    int begCharBrat;
    int endCharBrat;
    String headWord;
    int tokenId; 
    
    public MentionForBrat(int bcd, int ecd, int bcb, int ecb, String hw, int ti) {
      begCharDocument = bcd;
      endCharDocument = ecd;
      begCharBrat     = bcb;
      endCharBrat     = ecb;
      headWord        = hw;
      tokenId         = ti;
    }
  };  

/**********************************************************
*                       MAIN
**********************************************************/


  public static void main(String[] args){
    System.out.println("Starting scene parser");

    // coreNLP stuff
    Properties props = new Properties();
    props.put("annotators", "tokenize, ssplit, pos, lemma, ner, parse, dcoref");
    StanfordCoreNLP pipeline = new StanfordCoreNLP(props);

    // handling input:
    String namefile  = args[0];
    String outDir    = args[1];
    String verbOrNom = args[2];

    try
    {
      SceneParser sceneParser = new SceneParser(namefile);
      String configFile       = "srl-config.properties";
      CuratorSRLClassifier curatorSRL = new CuratorSRLClassifier(configFile, verbOrNom);
      curatorSRL.setupClient("jackson.stanford.edu", "9010");

      for (int sceneId : sceneParser.sceneData.keySet()) {
          if (sceneId == 0) {
            System.out.println("Skip scene 0\n");
            continue;
          }

          /*if (sceneId > 1) {
            break;
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


               PredicateArgumentView predArgs = null;
               
               boolean failFlag = false;               
               int sourceTokenId;

               try {
                 predArgs = curatorSRL.getPredicateArguments(sentenceString);
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
                      if (relName == "A0" || relName == "A1" || relName == "A2" || relName == "A3") {

                        // Iterate through mentions to find the best match 
                        // and annotate if it is animate or in the cast list
                        int bestMatch = -1;
                        float bestOverlap = 0;

                        for (int mentionI = 0; mentionI < orderedMentions.size(); mentionI++) {
                          Mention m1 = orderedMentions.get(mentionI);
                          // get the character span of the head string in the mention
                          int begCharMention = m1.headWord.beginPosition();
                          int endCharMention = m1.headWord.endPosition();
                          float overlap = -1;
                          if (endCharMention > begChar && begCharMention <= begChar) {
                            overlap = (float)(Math.min(endCharMention, endChar) - begChar)/
                                      (float)(Math.max(endCharMention, endChar) - begCharMention);
                          } else if (endChar > begCharMention && begChar <= begCharMention) {
                            overlap = (float)(Math.min(endCharMention, endChar) - begCharMention)/
                                      (float)(Math.max(endCharMention, endChar) - begChar);
                          }
                          if (overlap > bestOverlap) {
                            bestMatch = mentionI;
                            bestOverlap = overlap;
                          }                          
                        }
                        
                        if (bestMatch >= 0) {
                          Mention m1 = orderedMentions.get(bestMatch);
                          begChar = m1.headWord.beginPosition();
                          endChar = m1.headWord.endPosition();
                          System.out.println(m1.animacy);
                          System.out.println(m1.headWord + ", beg --- " + m1.headWord.beginPosition() +
                              " , end --- " + m1.headWord.endPosition());
                          int begCharMod = begChar - begCharOffset;
                          int endCharMod = endChar - begCharOffset;

                          // Check if the mention is animate, else neglect
                          if (m1.animacy != Dictionaries.Animacy.ANIMATE) {
                            continue;
                          }

                          String mentionKey = String.format("%d_%d", begCharMod, endCharMod);
                          int argumentTokenId = -1;
                          if (!argumentMentions.containsKey(mentionKey)) {
                            MentionForBrat m1ForBrat =  new MentionForBrat(begChar, endChar, begCharMod, endCharMod, m1.headWord.originalText(), tokenCtr++);
                            argumentMentions.put(mentionKey, m1ForBrat);             
                            writerAnno.write(String.format("T%d\tArgument %d %d\t%s\n", 
                             m1ForBrat.tokenId, m1ForBrat.begCharBrat, m1ForBrat.endCharBrat, m1ForBrat.headWord));                                
                            argumentTokenId = m1ForBrat.tokenId;
                          }
                          else {
                            MentionForBrat m1ForBrat = argumentMentions.get(mentionKey);
                            argumentTokenId = m1ForBrat.tokenId;
                          }

                          writerAnno.write(String.format("R%d\t%s Arg1:T%d Arg2:T%d\n", relationCtr++, relName, sourceTokenId, argumentTokenId));

                        } // end of if bestMatch
                      } // end of if argumentCheck
                   } // end of for relationIteration
                 } // end of for predicateGroup 
               }// end of checking if predGroupEmpty              

               charOffset += sentenceString.length()+1;
            }


            // Transfer coref labels to the arguments
            Collection <MentionForBrat> argumentMentionCollection = argumentMentions.values();
            
            for (CorefChain corefValue : corefGraph.values()) {
              List<CorefMention> mentions = value.getMentionsInTextualOrder();
              for( CorefMention cc : mentions ){
                //if(cc.sentNum == is + 1)
                //{
                  //curIndWord.add ( findRefIndexWord( dep, mention2label(sentences, cc)) );
                  SemanticGraph refDep  = mention2SemanticGraph( sentences, corefValue.getRepresentativeMention());  
                  IndexedWord rep   = findRefIndexWord( refDep, mention2label(sentences, value.getRepresentativeMention()));
                  refIndWord.add(new Pair< SemanticGraph, IndexedWord> (refDep, rep));
                //} 
              }         
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
