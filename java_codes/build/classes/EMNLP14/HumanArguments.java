/*
Parse a scene file, extract the set of possible human
arguments from each sentence, the verbs/nominals.
Apply Stanford CoreNLP coref to get coreference values
for the arguments, where applicable (used in baselines only).
Written by Vignesh, 2014
*/

package EMNLP14;

import java.util.Properties;
import java.util.List;
import java.util.Collection;
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

import java.io.File;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.FileInputStream;
import java.io.InputStream;

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

import EMNLP14.BratToken.*;
import EMNLP14.Utils.*;

class HumanArguments{

  /* Class to store the mention to be written for Brat format */
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

  /* Class to store the arguments which are humans */
  public static class CandidateHumanArguments {
    
    String relationToGovernor;
    int begChar;
    int endChar;
    String originalText;
    boolean isPossesive; 
    List <String> parents;
    int tokenId;
    int sentenceId;

    public CandidateHumanArguments(String rtg, int bc, int ec, String ot, boolean ip) {
      relationToGovernor = rtg;
      begChar = bc;
      endChar = ec;
      originalText = ot;
      isPossesive = ip;
      parents = new ArrayList<String> ();
      sentenceId = -1;
    }     

  };
 
  public static class DependencyHumanArgumentExtractor {

    Dictionaries dict;
    List <String> castList;

    // Hacky reprsentation    
    HashMap <String, BratAnnotationTokens> bratVerbs;
    HashMap <String, BratAnnotationTokens> bratNoms;

    public void setVerbAndNominalList(ArrayList<BratAnnotationTokens> bv,
        ArrayList<BratAnnotationTokens> bn) {
      bratVerbs = new HashMap <String, BratAnnotationTokens> ();
      bratNoms = new HashMap <String, BratAnnotationTokens> ();

      // Hash based on the begining and end point
      for (BratAnnotationTokens b : bv) {
        String bratkey = String.format("%d_%d", b.begChar, b.endChar);
        bratVerbs.put(bratkey, b);
      }

      // Hash based on the begining and end point
      for (BratAnnotationTokens b : bn) {
        String bratkey = String.format("%d_%d", b.begChar, b.endChar);
        bratNoms.put(bratkey, b);
      }
    }    

    public DependencyHumanArgumentExtractor(Dictionaries d, List <String> cl) {
      dict = d;
      castList = cl;
      bratVerbs = new HashMap <String, BratAnnotationTokens> ();
      bratNoms = new HashMap <String, BratAnnotationTokens> ();
    }

    public List<CandidateHumanArguments> getHumanArguments(SemanticGraph dep, 
                  IndexedWord rootWord, Set <IndexedWord> alreadySeen, 
                  List <CandidateHumanArguments> argCandidates) {
      
        String wordTillNow;
        List <CandidateHumanArguments> resArgCandidates = 
          new ArrayList<CandidateHumanArguments>();

        // Check if the word is a verb/nominal and if so
        // get the voice of the verb
        String bratkey = String.format("%d_%d", rootWord.beginPosition(),
            rootWord.endPosition());
        
        if(!alreadySeen.contains(rootWord)) {
          alreadySeen.add(rootWord);
          if (rootWord.tag().equals("NN") | 
              rootWord.tag().equals("NNP") | 
              rootWord.tag().equals("NNS") ) {
            
            System.out.println("NER (" + rootWord.originalText() + ") : " + rootWord.ner());
            boolean isPerson = rootWord.ner().equals("PERSON");
            boolean personFlag = isPerson;
            int endPt = rootWord.endPosition();
            int begPt = rootWord.beginPosition();
            IndexedWord startWord = rootWord;
            wordTillNow = rootWord.originalText();

            while (isPerson) {
              isPerson = false;
              for (IndexedWord child : dep.getChildList(startWord)) {
                System.out.println("INDEX : " + startWord.index() + " --> " + child.index());
                if (child.ner().equals("PERSON") && 
                    dep.getEdge(startWord, child).toString().toLowerCase().equals("nn") &&
                    startWord.index() == (child.index() + 1)) {                  
                  alreadySeen.add(child);
                  isPerson = true;
                  startWord = child;
                  wordTillNow = child.originalText() + " " + wordTillNow;
                  begPt = child.beginPosition();
                  break;
                }
              }
            }

            // check if the word is human, then add it to list 
            // and reset the current trailing words
            if (Utils.isWordHuman(wordTillNow.toLowerCase(), this.castList, this.dict) | personFlag) {


              List<SemanticGraphEdge> inEdges = dep.getIncomingEdgesSorted(rootWord);
              String relation = "";
              if (inEdges.size() > 0)
                relation = inEdges.get(0).toString();

              // Stupid hacky check to ensure that "X speaks dialogue are removed"
              boolean isSpeaker = false;
              if (relation.toLowerCase().equals("nsubj")) {
                try {
                  IndexedWord syntaticParent = dep.getParent(rootWord);
                  if (syntaticParent.originalText().equals("speaks")) {
                    // Now check if sibling is dialogue
                    Collection <IndexedWord> siblings = dep.getSiblings(rootWord);
                    for (IndexedWord s : siblings) {
                      if (s.originalText().equals("dialogue")) {
                        isSpeaker = true;
                        break;
                      }
                    }
                  } 
                } catch (Exception e) {
                  e.printStackTrace();
                  System.out.println("Could not figure out if " + wordTillNow + 
                      "(" + relation + ") a speaker");
                }  
              }

              // Add to the set of arguments, if it is not a speaker
              if (!isSpeaker) {
                boolean isPossesive = relation.equals("poss");
                CandidateHumanArguments c = new CandidateHumanArguments(relation, 
                    begPt, endPt, wordTillNow, isPossesive);
                try {
                  for (IndexedWord argParent : dep.getParentList(rootWord)) {
                    c.parents.add(String.format("%d_%d", argParent.beginPosition(), 
                          argParent.endPosition()));     
                  }
                } catch (Exception e) {
                  e.printStackTrace();
                  System.out.println("Trouble getting the parents from the dep-tree.");
                }

                resArgCandidates.add(c);
                System.out.println("Adding: " + wordTillNow + "(" + relation + ")");
              }

            }

          } else {
           
            int endPt = rootWord.endPosition();
            int begPt = rootWord.beginPosition();
            wordTillNow = rootWord.originalText();

            if (Utils.isWordHuman(wordTillNow.toLowerCase(), 
                this.castList, this.dict) && !rootWord.tag().startsWith("V")) {

              boolean isPossesive = this.dict.possessivePronouns.contains(wordTillNow);
              List<SemanticGraphEdge> inEdges = dep.getIncomingEdgesSorted(rootWord);
              String relation = "";
              if (inEdges.size() > 0)
                relation = inEdges.get(0).toString();
              CandidateHumanArguments c = new CandidateHumanArguments(relation, 
                  begPt, endPt, wordTillNow, isPossesive);
              try {
                for (IndexedWord argParent : dep.getParentList(rootWord)) {
                    c.parents.add(String.format("%d_%d", argParent.beginPosition(), 
                          argParent.endPosition()));     
                }
              } catch (Exception e) {
                e.printStackTrace();
                System.out.println("Trouble getting the parents from the dep-tree.");
              }
        
              resArgCandidates.add(c);
              System.out.println("Adding: " + c.originalText + "(" + relation + ")");
              System.out.println(rootWord);
            }

          }

          List <IndexedWord> childList = new ArrayList<IndexedWord> ();
          try {
            childList = dep.getChildList(rootWord);
          } catch (Exception e) {
            e.printStackTrace();
            System.out.println("The childlist failed");
          }
          for (IndexedWord child : childList) {
            List<CandidateHumanArguments> res = getHumanArguments(dep, child, alreadySeen,
                argCandidates);
            resArgCandidates.addAll(res);
          }
          //System.out.println("Leaf: " + rootWord);
        }
        return resArgCandidates;
    }
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
    String inputDir     = args[0];
    String castListFile = args[1];
    String outDir    = args[2];

    // overlap threshold between the mention and SRL arguments
    float overlapThreshold = (float)0.1;

    try
    {
      //ArrayList<String> castList = Utils.parseCastList(castListFile);
      // castList = new ArrayList<String>();
      
      // parse properties file
      Properties prop = new Properties();
      InputStream is = new FileInputStream("properties.xml");
      prop.loadFromXML(is);
      
      // minimum number of times a name should appear as a speaker in order to be 
      // considered as a valid cast-member
      int castThresh = Integer.parseInt(prop.getProperty("castListThreshold"));

      ArrayList<String> castList = CastListParser.getCastFromSpeakerFile(castListFile, castThresh);

      for (int sceneId=1; sceneId <= 100; sceneId++) {
      //for (int sceneId : sceneParser.sceneData.keySet()) {
          if (sceneId == 0) {
            System.out.println("Skip scene 0\n");
            continue;
          }
          
          String sceneText = "";
          try {
            sceneText = Utils.getFullTextFromFile(inputDir + 
                String.format("/scene_%04d.txt", sceneId));
          } catch (Exception e) {
            System.out.println("Ending scene processing");
            break;
          }
      
          /* Get the verb and nominals from the brat annotation file 
           * to be consistent */
          ArrayList<BratAnnotationTokens> bratTokens = 
            Utils.parseAnnotationFile(inputDir + 
              String.format("/scene_%04d.ann", sceneId));
          ArrayList <BratAnnotationTokens> bratVerbs = 
            new ArrayList<BratAnnotationTokens>();
          ArrayList <BratAnnotationTokens> bratNoms = 
            new ArrayList<BratAnnotationTokens>();
          for (BratAnnotationTokens b : bratTokens) {
            if (b.tokenType.equals("Verb")){
                bratVerbs.add(b);
            } else if (b.tokenType.equals("Nom")) {
                bratNoms.add(b);
            }
          }
  

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
            
            // Intialize the human arg extractor
            DependencyHumanArgumentExtractor depArgExtractor = 
              new DependencyHumanArgumentExtractor(dict, castList);
            depArgExtractor.setVerbAndNominalList(bratVerbs, bratNoms);
              

            // Intialize mention extractor (for coref only)
            MentionExtractor menext = new MentionExtractor(dict, null);
            List <List <Mention>> newMentionList = menext.arrange(document, words, 
                      trees, mentionList, false);

            Document doc_sieve = new Document(document, newMentionList, null, dict);
            List <List <Mention> > orderedMentionsBySentence = doc_sieve.getOrderedMentions();
            int numSentences = orderedMentionsBySentence.size();

            // Write an annotation file as well (easy to visualize and is used as ref for features)
            File annoFile = new File(outDir + String.format("/scene_%04d.ann", sceneId));
            BufferedWriter writerAnno = new BufferedWriter(new FileWriter(annoFile));

            // Feature writting files
            File featFile = new File(outDir + String.format("/scene_%04d.verb.feat", sceneId));
            BufferedWriter writerFeat = new BufferedWriter(new FileWriter(featFile));
            File featFileNom = new File(outDir + String.format("/scene_%04d.nom.feat", sceneId));
            BufferedWriter writerFeatNom = new BufferedWriter(new FileWriter(featFileNom));


            HashMap <String, MentionForBrat> argumentMentions = new HashMap <String, MentionForBrat> ();

            int charOffset = 0, begChar = 0, endChar = 0;
            int tokenCtr = 0;
            
            HashMap <String, Pair <CoreLabel, Integer> > sceneTokens = 
              new HashMap<String, Pair<CoreLabel, Integer> >();
            List<CandidateHumanArguments> argCandidateListPerScene =
                 new ArrayList<CandidateHumanArguments>();

            for (int sentI = 0; sentI < numSentences; sentI++) {

                /* Tree pretty print 
               System.out.println("Tree pretty print -------------------> ");
               trees.get(sentI).pennPrint(); */

               List<Mention> orderedMentions = orderedMentionsBySentence.get(sentI);

               String sentenceString = sentences.get(sentI).get(TextAnnotation.class);
               List<CoreLabel> sentenceTokens = sentences.get(sentI).get(TokensAnnotation.class);

               for (CoreLabel st : sentenceTokens) {
                sceneTokens.put(String.format("%d_%d", st.beginPosition(), st.endPosition()),
                    new Pair<CoreLabel, Integer> (st, sentI));
               }
           
               SemanticGraph dep = sentences.get(sentI)
                              .get(CollapsedCCProcessedDependenciesAnnotation.class);

               // Traverse (depth-first) the dependency graph to get the nouns
               System.out.println(" ---------------- DEPTH - FIRST ------------------\n");
               System.out.println(dep);
               Set <IndexedWord> alreadySeen = new HashSet <IndexedWord> ();
               List<CandidateHumanArguments> argCandidateList = 
                 new ArrayList<CandidateHumanArguments>();
               List<CandidateHumanArguments> argCandidateListUpdated =
                 new ArrayList<CandidateHumanArguments>();
               
               depArgExtractor.bratVerbs = Utils.voiceExtractor(trees.get(sentI), 
                                            depArgExtractor.bratVerbs);
               
               for (IndexedWord rootWord : dep.getRoots()) {                  
                   argCandidateListUpdated.addAll(depArgExtractor.getHumanArguments(dep, 
                         rootWord, alreadySeen, argCandidateList));                    
               }
               /*for (CandidateHumanArguments c : argCandidateList) {
                  System.out.println("Candidate: " + c.originalText);              
               }*/
               System.out.println("Candidate size: " + argCandidateListUpdated.size());
               System.out.println(" ---------------- DEPTH - FIRST - END ------------------\n");

               // account for multiple spaces at sentence begining
               int begCharOffset = 0;
               if (sentenceTokens.size() > 0)
                 begCharOffset = sentenceTokens.get(0).beginPosition() - charOffset;

               GoodOverlaps mentionOverlap = new GoodOverlaps(begCharOffset, 
                   overlapThreshold, orderedMentions, castList);
               
               // Go through each word and decide if human or not
               List <CoreLabel> sentenceWords = words.get(sentI);
               

               // Get the coref-value for each of the human arguments, by 
               // identifying the mentions which overlap with each argument
               for (int argId = 0; argId < argCandidateListUpdated.size(); argId++) {
                 int bestMatch = -1;
                 CandidateHumanArguments h = argCandidateListUpdated.get(argId);

                 ArrayList <Integer> goodOverlaps = mentionOverlap.getGoodOverlapMentions(
                          h.begChar, h.endChar, false);
                 bestMatch = mentionOverlap.bestMatch;

                 // set the mention-key to failed, if no mention overlaps
                 String mentionKey = String.format("FAIL_%d", tokenCtr++);
                 int argumentTokenId = -1;

                 if (bestMatch >= 0) {
                   Mention m1 = orderedMentions.get(bestMatch);
                   begChar = m1.headWord.beginPosition();
                   endChar = m1.headWord.endPosition();
                   mentionKey = String.format("%d_%d", begChar, endChar);
                 }

                 // map to the mention and enter into hashmap
                 if (!argumentMentions.containsKey(mentionKey)) {
                  argumentTokenId = tokenCtr++;
                  MentionForBrat m1ForBrat =  new MentionForBrat(h.begChar, 
                      h.endChar, h.begChar - begCharOffset, 
                      h.endChar - begCharOffset, 
                      h.originalText, argumentTokenId);
                  m1ForBrat.corefTokenName = "Other";
                  argumentMentions.put(mentionKey, m1ForBrat);                                 
                 } else {
                  argumentTokenId = argumentMentions.get(mentionKey).tokenId;
                 }
                 h.tokenId = argumentTokenId;
                 h.sentenceId = sentI;
                 argCandidateListUpdated.set(argId, h);
               }

               charOffset += sentenceString.length()+1;
               argCandidateListPerScene.addAll(argCandidateListUpdated);
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
                  Utils.getFromCastList(corefMentionValues.get(mentionKey), castList);
                argumentMentions.put(mentionKey, argumentBratMention);

              }
            }            


            for (MentionForBrat m1ForBrat : argumentMentions.values() ) {
             
              writerAnno.write(String.format("T%d\t%s %d %d\t%s\n", 
              m1ForBrat.tokenId, m1ForBrat.corefTokenName, m1ForBrat.begCharBrat, 
              m1ForBrat.endCharBrat, m1ForBrat.headWord));
            }

            
            // Write the brat verbs as passive and active (separately)
            for (String bratkey : depArgExtractor.bratVerbs.keySet()) {
              BratAnnotationTokens bv = depArgExtractor.bratVerbs.get(bratkey);
              bv.tokenId = tokenCtr++; // Update to the new token ctr
              if (bv.isPassive) {
                writerAnno.write(String.format("T%d\t%s %d %d\t%s\n",
                      bv.tokenId, "Verb_passive", bv.begChar, bv.endChar,
                      bv.originalText));
              } else {
                writerAnno.write(String.format("T%d\t%s %d %d\t%s\n",
                      bv.tokenId, "Verb_active", bv.begChar, bv.endChar,
                      bv.originalText));
              }

              // Re-insert the updated brat token
              depArgExtractor.bratVerbs.put(bratkey, bv);

            }

            // Write the brat noms as passive and active (separately)
            for (String bratkey : depArgExtractor.bratNoms.keySet()) {
              BratAnnotationTokens bn = depArgExtractor.bratNoms.get(bratkey);
              bn.tokenId = tokenCtr++; // Update to the new token ctr

              writerAnno.write(String.format("T%d\t%s %d %d\t%s\n",
                    bn.tokenId, "Nom", bn.begChar, bn.endChar,
                    bn.originalText));

              depArgExtractor.bratNoms.put(bratkey, bn);
            }
            
            writerAnno.close();

            // Writing the features
            writerFeat.write("#verb_token_id, argument_token_id,");
            writerFeat.write("pred_lemma, arg_sentence_id, pred_sentence_id");
            writerFeat.write("pred_voice, is_pred_parent, is_arg_to_left, rel_of_arg_to_gov\n");
            for (CandidateHumanArguments h : argCandidateListPerScene) {
              for (BratAnnotationTokens bv : depArgExtractor.bratVerbs.values()) {
                // Check if the peredicate is a parent of the argument                
                boolean isParent = h.parents.contains(String.format("%d_%d", 
                      bv.begChar, bv.endChar));

                boolean isLeft   = (h.endChar <= bv.begChar);
                String argumentRelation = h.relationToGovernor;
                if (argumentRelation.equals(""))
                  argumentRelation = "root";

                String predicateLemma = bv.originalText;
                String predicateKey = String.format("%d_%d", bv.begChar, bv.endChar);
                int predicateSentId = -1;
                if (sceneTokens.containsKey(predicateKey)) {
                  CoreLabel predicateToken = sceneTokens.get(predicateKey).first;
                  predicateLemma = predicateToken.get(LemmaAnnotation.class);
                  predicateSentId = sceneTokens.get(predicateKey).second;
                }
                boolean predicateVoice = bv.isPassive;
                writerFeat.write(bv.tokenId + "," + h.tokenId + "," + predicateLemma + 
                    "," + h.sentenceId + "," + predicateSentId + "," +
                    predicateVoice + "," + isParent + "," + isLeft + "," + argumentRelation
                    + "," + h.isPossesive + "\n");               
              }
            }
            writerFeat.close();

            // Writing the features
            writerFeatNom.write("#verb_token_id, argument_token_id,");
            writerFeatNom.write("pred_lemma, arg_sentence_id, pred_sentence_id");
            writerFeatNom.write("pred_voice, is_pred_parent, is_arg_to_left, rel_of_arg_to_gov, is_arg_possesive\n");
            for (CandidateHumanArguments h : argCandidateListPerScene) {
              for (BratAnnotationTokens bn : depArgExtractor.bratNoms.values()) {
                // Check if the peredicate is a parent of the argument                
                boolean isParent = h.parents.contains(String.format("%d_%d", 
                      bn.begChar, bn.endChar));

                boolean isLeft   = (h.endChar <= bn.begChar);
                String argumentRelation = h.relationToGovernor;
                if (argumentRelation.equals(""))
                  argumentRelation = "root";

                String predicateLemma = bn.originalText;
                String predicateKey = String.format("%d_%d", bn.begChar, bn.endChar);
                int predicateSentId = -1;
                if (sceneTokens.containsKey(predicateKey)) {
                  CoreLabel predicateToken = sceneTokens.get(predicateKey).first;
                  predicateLemma = predicateToken.get(LemmaAnnotation.class);
                  predicateSentId = sceneTokens.get(predicateKey).second;
                }
                boolean predicateVoice = false;
                writerFeatNom.write(bn.tokenId + "," + h.tokenId + "," + predicateLemma + 
                    "," + h.sentenceId + "," + predicateSentId + "," +
                    predicateVoice + "," + isParent + "," + isLeft + "," + argumentRelation
                    + "," + h.isPossesive + "\n");               
              }
            }
            writerFeatNom.close();


            
         } catch (Exception e) {
           e.printStackTrace();
         }// end inner try-catch
      }// end loop through scenes
    } catch(Exception e) {
      e.printStackTrace();
    }// end outter try-catch

  }// end main

}

