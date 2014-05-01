/*
Tokens extracted from a Brat style text file.
Written by Vignesh, 2014
*/

package EMNLP14;

import java.util.Properties;
import java.util.List;


class BratToken {

  public static class SRLToken {
    
    int begCharBrat; // begining character in the brat text
    int endCharBrat; // ending character in the brat text
    String originalText; // original text of the token
    int tokenId; // a unique token id
    int sentenceId; // sentence number

    public SRLToken(int bcb, int ecb, String ot, int ti) {
      begCharBrat = bcb;
      endCharBrat = ecb;
      originalText = ot;
      tokenId = ti;
      sentenceId = -1;
    }

    public SRLToken() {
      begCharBrat = -1;
      endCharBrat = -1;
      originalText = null;
      tokenId = -1;
      sentenceId = -1;      
    }

    public void setSentenceId(int si) {
      this.sentenceId = si;
    }

  }

  /* Human tokens (for argument candidates) */
  public static class HumanToken extends SRLToken{

    int animacyValue;

    String corefOriginalText;
    String corefCastName;

    public HumanToken() {super();}
    public HumanToken(int bcb, int ecb, String ot, int ti) {
      super(bcb, ecb, ot, ti);
    }
  };  

  /* Verb tokens */
  public static class VerbToken extends SRLToken{

    
    public VerbToken() {super();}
    public VerbToken(int bcb, int ecb, String ot, int ti) {
      super(bcb, ecb, ot, ti);
    }


  };

  /* Nominal tokens */
  public static class NomToken extends SRLToken{

    public NomToken() {super();}
    public NomToken(int bcb, int ecb, String ot, int ti) {
      super(bcb, ecb, ot, ti);
    }

  };

}
