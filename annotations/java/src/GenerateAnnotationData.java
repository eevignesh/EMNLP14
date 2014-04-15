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

class SceneParser{
  HashMap <Integer, String> sceneData;
  HashMap <Integer, String> sceneHeader;

  private static SceneParser( String file ) throws IOException {
      BufferedReader reader   = new BufferedReader( new FileReader (file));
      String         line     = null;
      String         ls       = System.getProperty("line.separator");
      Pattern        pattern  = Pattern.compile("--- \\((\\d*)\\) ---");
      
      int     status      = -1;
      int     scene_id    = -1;
      String  scene_data  = "";
      String  scene_head  = "";

      while( ( line = reader.readLine() ) != null ) {
          Matcher matcher = pattern.matcher(line);
          if (matcher.found()) {
            if (status >= 0 && scene_id > 0) {
              sceneData[scene_id] = scene_data;
              sceneHeader[scene_id] = scene_head;
            }
            scene_id = Integer.parseInt(matcher.group(1));
            status = 1;
            scene_data = "";
            scene_head = "";
          } else if (status == 1) {
            scene_head = line;
            status     = 2;      
          } else if (status == 2) {
            scene_data += line;          
          }
                   
      }

      if (status >= 0 && scene_id > 0) {
          sceneData[scene_id] = scene_data;
          sceneHeader[scene_id] = scene_head;  
      }
	}

}

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

  public static String readText(String filename){
    String everything = "";
    try 
    {
      BufferedReader br = new BufferedReader(new FileReader(filename));
      StringBuilder sb = new StringBuilder();
      String line = br.readLine();

      while (line != null) 
      {
        sb.append(line);
        sb.append('\n');
        line = br.readLine();
      }
      everything = sb.toString();
      br.close();
    } 
    catch (Exception e) 
    {
      e.printStackTrace();
    }
    return everything;    
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
		String namefile = "scene_breaks.txt";
    try
    {
      SceneParser sceneParser = new SceneParser(namefile);
      while( (namefile = br_fileList.readLine()) != null )
      {
        parseXml xmlParser = new parseXml();
        String alignDir = namefile; //"/home/vignesh/Desktop/CVPR2014/java/sample_alignments/alignments/";     

        FileWriter 		outFilePair	  = new FileWriter(alignDir + "/pairCorefFeature.txt");
        FileWriter 		outFileUnary	= new FileWriter(alignDir + "/unaryCorefFeature.txt");


        xmlParser.getMentionList(alignDir);
        Map <String, UnaryCorefFeature> unaryAdded = new HashMap<String, UnaryCorefFeature>();
        for(int d : xmlParser.dscr2Mentions.keySet())
        {

          String text = xmlParser.descriptionList.get(d);
    
          //try{text = readFile(namefile);}
          //catch(IOException e){	e.printStackTrace();}
          
          // create an empty Annotation just with the given text
          Annotation 		document 	= new Annotation(text);
          //Annotation    document_sieve  = new Annotation(text);
          // run all Annotators on this text
          pipeline.annotate(document);

          // these are all the sentences in this document
              // a CoreMap is essentially a Map that uses 
          // class objects as keys and has values with custom types
          List<CoreMap> 		sentences 	= document.get(SentencesAnnotation.class);
          List<Tree> trees = new ArrayList<Tree>();
          List<List<CoreLabel>> words = new ArrayList< List <CoreLabel>>();
          
          for (CoreMap s : sentences)
          {
            words.add(s.get(TokensAnnotation.class));
            trees.add(s.get(TreeAnnotation.class));
          }

          PairInt minMaxSent = getMinMaxSent(words, xmlParser.minMaxDscrIds.get(d));
          System.out.println("Min-max sent " + "(" + d + ") : " + minMaxSent.first + "," + minMaxSent.second);
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
            
             for (int sentI = minMaxSent.first; sentI <= minMaxSent.second; sentI++) {
                    
              List<Mention> orderedMentions = orderedMentionsBySentence.get(sentI);
              
              int minSent = Math.max(sentI-2, minMaxSent.first);

              for (int mentionI = 0; mentionI < orderedMentions.size(); mentionI++) {

                Mention m1 = orderedMentions.get(mentionI);
                List<Mention> mentListForFeat = new ArrayList<Mention>();
                List<Mention> orderedAntecedents = new ArrayList<Mention>();                 
                //List<Integer> orderedDistances    = new ArrayList<Integer>();
                
                //int max_sent_beg = max(0, sentI-2);
                for (int sentJ = sentI; sentJ >= minSent; sentJ--) {
                  if (sentJ == sentI) {
                    orderedAntecedents.addAll(orderedMentions.subList(0, mentionI)); 
                    //orderedDistances = distMentionsForPronoun(orderedAntecedents, m1);
                    mentListForFeat.addAll(orderedAntecedents);                      
                  }
                  else {
                    mentListForFeat.addAll(orderedMentionsBySentence.get(sentJ));
                  }
                }
                List<PairCorefFeature> pcf = new ArrayList<PairCorefFeature>();
                pcf = getPairFeature(mentListForFeat, m1, dict, words);

                // first get the mention-id for m1
                
                MentData mdt = new MentData(m1.headWord.beginPosition(),
                                            m1.headWord.endPosition(),
                                            d,
                                            m1.headString);
                int ment_id = -1;
                if (xmlParser.mentHash.containsKey(mdt._hashValue))
                {
                  ment_id = xmlParser.mentHash.get(mdt._hashValue);
                }
                else
                {
                  ment_id = xmlParser.mentList.size();
                  xmlParser.mentHash.put(mdt._hashValue, ment_id);
                  xmlParser.mentList.add(mdt);
                }
                
                // add stuff to the subj-predicate list, for printing
                if (!xmlParser.coveredMentIds.contains(ment_id))
                {
                  SubjPredicate sp = new SubjPredicate();
                  sp._mentionId    = ment_id;
                  sp._subjValue    = m1.headString;
                  sp._dscrid       = d;
                  xmlParser.spList.add(sp);
                  xmlParser.coveredMentIds.add(ment_id);
                }

                if (!unaryAdded.containsKey(mdt._hashValue))
                {
                  UnaryCorefFeature uf = new UnaryCorefFeature();
                  uf.getUnaryFeature(m1);
                  unaryAdded.put(mdt._hashValue, uf);           
                  uf.writeFeature(ment_id, outFileUnary);     
                }

                for (int pid = 0; pid < mentListForFeat.size(); pid++)
                {
                  Mention ante_mention = mentListForFeat.get(pid);
                  int ante_ment_id = -1;
                  MentData amdt = new MentData(ante_mention.headWord.beginPosition(),
                                               ante_mention.headWord.endPosition(),
                                               d,
                                               ante_mention.headString);

                  if (xmlParser.mentHash.containsKey(amdt._hashValue))
                  {
                    ante_ment_id = xmlParser.mentHash.get(amdt._hashValue);
                  }
                  else
                  {
                    ante_ment_id = xmlParser.mentList.size();
                    xmlParser.mentHash.put(amdt._hashValue, ante_ment_id);
                    xmlParser.mentList.add(amdt);
                  }

                  // add stuff to the subj-predicate list, for printing
                  if (!xmlParser.coveredMentIds.contains(ante_ment_id))
                  {
                    SubjPredicate sp = new SubjPredicate();
                    sp._mentionId    = ante_ment_id;
                    sp._subjValue    = ante_mention.headString;
                    sp._dscrid       = d;
                    xmlParser.spList.add(sp);
                    xmlParser.coveredMentIds.add(ante_ment_id);
                  }

                  if (!unaryAdded.containsKey(amdt._hashValue))
                  {
                    UnaryCorefFeature uf = new UnaryCorefFeature();
                    uf.getUnaryFeature(ante_mention);
                    unaryAdded.put(amdt._hashValue, uf);
                    uf.writeFeature(ante_ment_id, outFileUnary);
                  }


                  System.out.println(m1.toString() + " ===> " + ante_mention.toString());
                  pcf.get(pid).writeFeature(ment_id, ante_ment_id, outFilePair);
                }
                
                // next get the mention-id for the antecedent list

             }
          }

          /*Properties propsSieveCoref = new Properties();
          SieveCoreferenceSystem testSieveCoref = new SieveCoreferenceSystem(propsSieveCoref);
          Map <Integer, CorefChain> graphSieve = testSieveCoref.coref(doc_sieve);   */
          }
          catch(Exception e)
          {
            e.printStackTrace();
          }

        }

        // write the mention list data
        FileWriter 		outFileMent	= new FileWriter(alignDir + "/mentionList.txt");
        for (SubjPredicate sp : xmlParser.spList)
        {
          sp.write(outFileMent, xmlParser.mentList.get(sp._mentionId));
        }
        outFileMent.close();
        outFilePair.close();
        outFileUnary.close();
      }
      
      br_fileList.close();
    }
    catch(IOException e)
    {
      e.printStackTrace();
    }

  }


}

