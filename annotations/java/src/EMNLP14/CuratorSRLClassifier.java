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


public class CuratorSRLClassifier {

  private CuratorClient _client;

  public PredicateArgumentView getPredicateArgumentsVerb(String input) throws Exception {
    boolean forceUpdate = false;
    TextAnnotation ta = _client.getTextAnnotation("", "", input,
          forceUpdate);

    _client.addSRLView(ta, forceUpdate);
    PredicateArgumentView v = (PredicateArgumentView) (ta.getView(ViewNames.SRL));
    System.out.println(v);
    return v;

  }

  public PredicateArgumentView getPredicateArgumentsNom(String input) throws Exception {
    boolean forceUpdate = false;
    TextAnnotation ta = _client.getTextAnnotation("", "", input,
          forceUpdate);

     _client.addNOMView(ta, forceUpdate);
    PredicateArgumentView n = (PredicateArgumentView) ta.getView(ViewNames.NOM);

    return n;

  }


  public void setupClient(String host, String portNumber) {
    int port = Integer.parseInt(portNumber);
    _client = new CuratorClient(host, port);

  }

}


