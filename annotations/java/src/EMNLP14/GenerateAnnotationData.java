/*
 	Transform scripts into action triplets
	Written by Armand Joulin, 2013.	
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


import java.io.File;
import java.io.IOException;
import java.io.BufferedReader;
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
	private static final boolean DEBUG 	= false;
	private static final boolean DEBUG_MAIN	= true | DEBUG;
	private static final boolean DEBUG_PS 	= false | DEBUG;
	private static final boolean DEBUG_SG 	= false | DEBUG;
  private static final boolean EXT_OUT = true;
  private static final boolean ANN_OUT = true;

	private static final boolean PARSE_TREE	= false;
	private static final boolean SEM_GRAPH 	= true;
	private static final boolean COREF 	= true;

	private static final List<String> VERBS  = Arrays.asList(new String[]{"VBD", "VBZ", "VBP","VB"});
	private static final List<String> SUBJECTS = Arrays.asList(new String[]{"nsubj", "agent", "attr"});
	private static final List<String> SUBJ_FOLLOW_LIST = Arrays.asList(new String[]{"xcomp","aux","auxpass"});

  private static String basename(String path) {
    int slash = path.lastIndexOf('/');
    String base = (slash == -1) ? path : path.substring(slash+1);
    return base;
  }


	private static String sentence2string(CoreMap  sentence){
			List<CoreLabel> tokens = sentence.get(TokensAnnotation.class);
			String stc ="";
			for( CoreLabel token : tokens)
				stc+= token.word() + " ";
			return stc;
	}


	/******************************************************************************************
	*
	*
	*					MAIN
	* 
	*
	******************************************************************************************/


	public static void main(String[] args){
		System.out.println("Starting scene parser");
					
    // coreNLP stuff
		Properties 		props 		    = new Properties();
		props.put("annotators", "tokenize, ssplit, pos, lemma, ner, parse, dcoref");
 		StanfordCoreNLP 	pipeline 	= new StanfordCoreNLP(props);

		// handling input:
		String namefile = args[1];
    try
    {
      SceneParser sceneParser = new SceneParser(namefile);
      for (int sceneId : sceneParser.sceneData.keySet()) {

          String        sceneText = sceneParser.sceneData.get(sceneId);
          Annotation 		document 	= new Annotation(sceneText);
          //Annotation    document_sieve  = new Annotation(text);
          // run all Annotators on this text
          pipeline.annotate(document);

          // these are all the sentences in this document
          // a CoreMap is essentially a Map that uses 
          // class objects as keys and has values with custom types
          List<CoreMap> 		sentences 	= document.get(SentencesAnnotation.class);
          List<List<CoreLabel>> words = new ArrayList< List <CoreLabel>>();
          List<Tree> trees = new ArrayList<Tree>();

          for (CoreMap s : sentences)
          {
            words.add(s.get(TokensAnnotation.class));
            trees.add(s.get(TreeAnnotation.class));
          }

          //Map<Integer,CorefChain> graph 		= document.get(CorefChainAnnotation.class);
          try
          {

            Dictionaries dict = new Dictionaries();
            RuleBasedCorefMentionFinder ruleMentionFinder = new RuleBasedCorefMentionFinder();
            List <List <Mention> > mentionList = ruleMentionFinder.extractPredictedMentions(document, -1, dict);
            
            //Semantics semantic = new Semantics();
            MentionExtractor menext = new MentionExtractor(dict, null);
            List <List <Mention>> newMentionList = menext.arrange(document, words, trees, mentionList, false);
            /*for (List<Mention> lm : newMentionList)
            {
              for (Mention m : lm)
              {
                //m.process(dict, semantic, menext);
                System.out.println(m.toString());
                System.out.println(m.animacy);
                System.out.println(m.gender);
                System.out.println(m.number);
                System.out.println(m.nerString);
                System.out.println(m.headString);
                System.out.println(" ----------------------- ");

              }
            }*/

             Document doc_sieve = new Document(document, newMentionList, null, dict);
             List <List <Mention> > orderedMentionsBySentence = doc_sieve.getOrderedMentions();
            
             for (int sentI = 0; sentI <= 1; sentI++) {                    
               List<Mention> orderedMentions = orderedMentionsBySentence.get(sentI);              
                for (int mentionI = 0; mentionI < orderedMentions.size(); mentionI++) {
                  Mention m1 = orderedMentions.get(mentionI);
                  System.out.println(m1.toString());
                  System.out.println(m1.animacy);
                }
             }
          } catch (Exception e) {
            e.printStackTrace();
          }// end inner try-catch
      }// end loop through scenes
    } catch(IOException e) {
      e.printStackTrace();
    }// end outter try-catch

  }// end main

}

