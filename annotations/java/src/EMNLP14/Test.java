package EMNLP14;

import edu.illinois.cs.cogcomp.srl.SRLProperties;
import edu.illinois.cs.cogcomp.srl.Main;


import java.util.Arrays;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.illinois.cs.cogcomp.core.datastructures.trees.TreeParserFactory;
import edu.illinois.cs.cogcomp.edison.annotators.HeadFinderDependencyViewGenerator;
import edu.illinois.cs.cogcomp.edison.data.curator.CuratorClient;
import edu.illinois.cs.cogcomp.edison.data.curator.CuratorDataStructureInterface;
import edu.illinois.cs.cogcomp.edison.sentences.Constituent;
import edu.illinois.cs.cogcomp.edison.sentences.PredicateArgumentView;
import edu.illinois.cs.cogcomp.edison.sentences.SpanLabelView;
import edu.illinois.cs.cogcomp.edison.sentences.TextAnnotation;
import edu.illinois.cs.cogcomp.edison.sentences.TokenLabelView;
import edu.illinois.cs.cogcomp.edison.sentences.TreeView;
import edu.illinois.cs.cogcomp.edison.sentences.ViewNames;
import edu.illinois.cs.cogcomp.edison.utilities.WordNetManager;

import edu.illinois.cs.cogcomp.indsup.learning.DenseVector;
import edu.illinois.cs.cogcomp.indsup.learning.WeightVector;

import edu.illinois.cs.cogcomp.srl.core.Models;
import edu.illinois.cs.cogcomp.srl.core.SRLManager;
import edu.illinois.cs.cogcomp.srl.core.VerbNom;
import edu.illinois.cs.cogcomp.srl.experiment.TextPreProcessor;
import edu.illinois.cs.cogcomp.srl.inference.ISRLInference;
import edu.illinois.cs.cogcomp.srl.inference.SRLLagrangeInference;
import edu.illinois.cs.cogcomp.thrift.base.Forest;
import edu.illinois.cs.cogcomp.thrift.curator.Record;


public class Test {
	public static void main(String[] arguments) throws Exception {

		String configFile = arguments[0];

		String verbNom = arguments[1];

		String host = arguments[2];
		int port = Integer.parseInt(arguments[3]);

		String input = null;
		Test srl = new Test(configFile, verbNom);

		CuratorClient client = new CuratorClient(host, port);
		boolean forceUpdate = false;

		System.out.print("Enter text (underscore to quit): ");
		input = "Alice weds Bob in the prison";

    TextAnnotation ta = client.getTextAnnotation("", "", input,
        forceUpdate);

    TextPreProcessor.addViewsFromCurator(ta, client,
        srl.manager.defaultParser);

    PredicateArgumentView p = srl.getSRL(ta);

    for (Constituent c : p.getPredicates()){
      System.out.println("Predicate: " + c.getSurfaceString() + " start: " + c.getStartCharOffset() + " End: " + c.getEndCharOffset());  
    }

    System.out.println(p);
    System.out.println();

	}

	private final static Logger log = LoggerFactory
			.getLogger(Test.class);

	public final SRLManager manager;

	public Test(String configFile, String verbNom)
			throws Exception {

		//DenseVector.BLOCK_SIZE = 100000;

		WordNetManager.loadConfigAsClasspathResource(true);

		log.info("Initializing config");
		SRLProperties.initialize(configFile);

		log.info("Creating {} manager", verbNom);
		manager = Main.getManager(VerbNom.valueOf(verbNom), false, true, false,
				"Charniak");

		log.info("Loading models");
		loadModels();

		TextAnnotation ta;
		if (manager.getVerbNom() == VerbNom.Verb)
			ta = initializeDummySentenceVerb();
		else
			ta = initializeDummySentenceNom();

		log.info("Running {} SRL on sentence {}", verbNom, ta.getText());
		PredicateArgumentView srl = getSRL(ta);

		log.info("Output: {}", srl.toString());

	}

	public String getSRLCuratorName() {
		return manager.getSRLSystemIdentifier();
	}

	protected TextAnnotation initializeDummySentenceVerb() {
		TextAnnotation ta = new TextAnnotation("", "", Arrays.asList("I do ."));

		TokenLabelView tlv = new TokenLabelView(ViewNames.POS, "Test", ta, 1.0);
		tlv.addTokenLabel(0, "PRP", 1d);
		tlv.addTokenLabel(1, "VBP", 1d);
		ta.addView(ViewNames.POS, tlv);

		ta.addView(ViewNames.NER, new SpanLabelView(ViewNames.NER, "test", ta,
				1d));

		SpanLabelView chunks = new SpanLabelView(ViewNames.SHALLOW_PARSE,
				"test", ta, 1d);
		chunks.addSpanLabel(0, 1, "NP", 1d);
		chunks.addSpanLabel(1, 2, "VP", 1d);
		ta.addView(ViewNames.SHALLOW_PARSE, chunks);

		TreeView parse = new TreeView(ViewNames.PARSE_CHARNIAK, "Charniak", ta,
				1.0);
		parse.setParseTree(
				0,
				TreeParserFactory
						.getStringTreeParser()
						.parse("(S1 (S (NP (PRP I))       (VP (VPB do))        (. .)))"));
		ta.addView(parse.getViewName(), parse);

		return ta;
	}

	protected TextAnnotation initializeDummySentenceNom() {
		TextAnnotation ta = new TextAnnotation("", "",
				Arrays.asList("The construction of the library is complete ."));

		TokenLabelView tlv = new TokenLabelView(ViewNames.POS, "Test", ta, 1.0);
		tlv.addTokenLabel(0, "DT", 1d);
		tlv.addTokenLabel(1, "NN", 1d);
		tlv.addTokenLabel(2, "IN", 1d);
		tlv.addTokenLabel(3, "DT", 1d);
		tlv.addTokenLabel(4, "NN", 1d);
		tlv.addTokenLabel(5, "VB", 1d);
		tlv.addTokenLabel(6, "JJ", 1d);
		tlv.addTokenLabel(7, ". ", 1d);

		ta.addView(ViewNames.POS, tlv);

		ta.addView(ViewNames.NER, new SpanLabelView(ViewNames.NER, "test", ta,
				1d));

		SpanLabelView chunks = new SpanLabelView(ViewNames.SHALLOW_PARSE,
				"test", ta, 1d);

		chunks.addSpanLabel(0, 2, "NP", 1d);
		chunks.addSpanLabel(2, 3, "PP", 1d);
		chunks.addSpanLabel(3, 5, "NP", 1d);
		chunks.addSpanLabel(5, 6, "VP", 1d);
		chunks.addSpanLabel(6, 7, "ADJP", 1d);

		ta.addView(ViewNames.SHALLOW_PARSE, chunks);

		TreeView parse = new TreeView(ViewNames.PARSE_CHARNIAK, "Charniak", ta,
				1.0);

		String treeString = "(S1 (S (NP (NP (DT The) (NN construction)) (PP (IN of) (NP (DT the) (NN library)))) (VP (AUX is) (ADJP (JJ complete))) (. .)))";
		parse.setParseTree(0,
				TreeParserFactory.getStringTreeParser().parse(treeString));
		ta.addView(parse.getViewName(), parse);

		return ta;

	}

	public String getVersion() {
		return SRLProperties.getInstance().getSRLVersion();
	}

	public String getCuratorName() {
		return "illinoisSRL";
	}

	private void loadModels() throws Exception {

		for (Models m : Models.values()) {
			if (manager.getVerbNom() == VerbNom.Verb && m == Models.Predicate)
				continue;

			log.info("Loading model {}", m);
			manager.getModelInfo(m).loadWeightVector();
		}

		log.info("Finished loading all models");
	}

	public PredicateArgumentView getSRL(TextAnnotation ta) throws Exception {

		log.info("Input: {}", ta.getText());
		TextPreProcessor.addHelperViews(ta);

		// adding this right now helps to remove a concurrency related bug in
		// the dependency view
		// generator.
		ta.addView(new HeadFinderDependencyViewGenerator(manager.defaultParser));

		List<Constituent> predicates;
		if (manager.getVerbNom() == VerbNom.Verb)
			predicates = manager.getHeuristicPredicateDetector().getPredicates(
					ta);
		else
			predicates = manager.getLearnedPredicateDetector()
					.getPredicates(ta);

		ISRLInference inference = new SRLLagrangeInference(manager, ta,
				predicates, true, 100);

		return inference.getOutputView();
	}

	public Forest getSRLForest(Record record) throws Exception {
		TextAnnotation ta = CuratorDataStructureInterface
				.getTextAnnotationViewsFromRecord("", "", record);

		PredicateArgumentView pav = getSRL(ta);

		return CuratorDataStructureInterface
				.convertPredicateArgumentViewToForest(pav);
	}

}
