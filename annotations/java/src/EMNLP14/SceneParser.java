package EMNLP14;

import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.util.HashMap;

import java.io.File;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.FileReader;


class SceneParser{
  public static HashMap <Integer, String> sceneData;
  public static HashMap <Integer, String> sceneHeader;

  public SceneParser( String file ) throws IOException {
      sceneData               = new HashMap <Integer, String>();
      sceneHeader             = new HashMap <Integer, String>();
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
          if (matcher.find()) {
            if (status >= 0 && scene_id > 0) {
              sceneData.put(scene_id, scene_data);
              sceneHeader.put(scene_id, scene_head);
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
          sceneData.put(scene_id, scene_data);
          sceneHeader.put(scene_id, scene_head);
      }
	}

  /*public static void main(String[] args){
    try{      
      SceneParser sceneParser = new SceneParser(args[0]);
      System.out.println(sceneParser.sceneData.get(Integer.parseInt(args[1])));
    } catch (IOException e) {
      e.printStackTrace();
    }

  }*/

}
