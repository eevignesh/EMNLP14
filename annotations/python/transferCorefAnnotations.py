#!/usr/bin/python

import re
import sys
from HTMLParser import HTMLParser
from htmlentitydefs import name2codepoint
from keyboard_util import keyboard

# Parse the html snippet from the annotation file
class AnnotationParser(HTMLParser):
    def __init__(self):
    	self.action_ids = [-1]
        self.action_data = {}
        self.current_status = 'action'
        HTMLParser.__init__(self)
    def read(self, data):
    	self.reset()
    	self.feed(data)
    def handle_starttag(self, tag, attrs):
      self.current_status = tag;
      #print attrs
      if (tag=='action' or tag=='subject' or tag=='coref-subject'): 
        self.attributes 	= [int(atr[1]) for atr in attrs];	
      if self.current_status == 'action':
        self.action_ids = self.attributes
  
    def handle_endtag(self, tag):
      if tag == 'coref-subject' or tag == 'subject':
        self.current_status = 'action'

    def handle_data(self, data):
      if self.current_status == 'subject':
        self.action_data[self.attributes[0]] =[self.attributes, data, '']
      elif self.current_status == 'coref-subject':
          coref_subj_data = self.action_data[self.attributes[0]]
          coref_subj_data[-1] = data
          self.action_data[self.attributes[0]] = coref_subj_data
          #print 'current_tag:', self.current_status, 'coref-data:', data
          #print coref_subj_data

class Splits:
    Scene, Sent, Html = range(3)

# Parse the coref file along with paragraph to get scenes, and coref data
class TextParser():
    def __init__(self):
        self.scene_splitter     = '_____________________\n';
        self.sent_splitter	= '---------------------\n';
        self.html_splitter 	= '#####################\n';
        self.scene_data = {}
        self.action_data = {}
        self.sent_data	 = {}
        self.current_status = Splits.Scene

    def insert_data(self, html_parser, current_block, current_sent):
        #print html_parser.action_ids
        if not html_parser.action_ids[0] == -1:
            self.scene_data[html_parser.action_ids[0]] = current_block
            self.sent_data[html_parser.action_ids[1]]  = current_sent
            self.action_data[html_parser.action_ids[2]] = [html_parser.action_ids[0], html_parser.action_data]

    def parse_file(self, coref_textfile):
            html_parser = AnnotationParser()
            current_block = ''
            current_sent = ''
            for line in coref_textfile:                
                # identify the split between scene/sentence/html-part
                if line == self.scene_splitter:                    
                    self.current_status = Splits.Scene
                    self.insert_data(html_parser, current_block, current_sent)	
                    current_block = ''
                    continue
                elif line == self.sent_splitter:
                    self.current_status = Splits.Sent
                    continue
                elif line == self.html_splitter:
                    self.current_status = Splits.Html
                    html_parser = AnnotationParser()
                    html_parser.reset()
                    continue

                # read the data	
                if self.current_status == Splits.Scene:
                    current_block = current_block + line
                elif self.current_status == Splits.Sent:
                    current_sent = current_sent + line
                elif self.current_status == Splits.Html:                    
                    #print 'feed-line:', line
                    html_parser.feed(line)

            self.insert_data(html_parser, current_block, current_sent)

class CorefHtmlParser:
    '''
      Parse the coref html file with the annotations.
    '''
    def __init__(self):
        self.action_data = {}
        self.status = 0
    def insert_data(self, html_parser):
        if not html_parser.action_ids[0] == -1:
            self.action_data[html_parser.action_ids[2]] = [html_parser.action_ids[0], html_parser.action_data]

    def parse_file(self, coref_textfile):
            html_parser = AnnotationParser()
            for line in coref_textfile:                
                # identify the end of html-section for one annotation
                if line.startswith('</action>'):
                    html_parser.feed(line)
                    #print 'Inserting data', html_parser.action_data
                    self.insert_data(html_parser)
                    html_parser = AnnotationParser()
                    html_parser.reset()
                    self.status = 0
                elif line.startswith('<action'):
                    html_parser.feed(line)
                    self.status = 1
                elif self.status == 1:
                    html_parser.feed(line)

            #self.insert_data(html_parser)

class SceneBreakParser():
    '''
      Parse the scene breaks file to get the different scene descriptions (old format).
    '''
    def __init__(self):
        self.scene_regex = r'--- \((\d*)\) ---';
        self.scene_data   = {}
        self.scene_header = {}
        

    def parse_file(self, coref_textfile):
        current_scene = ''
        current_header = ''
        scene_id = -1         
        current_status = 0
        for line in coref_textfile:                
            # identify the split between scenes
            split_value = re.search(self.scene_regex, line)
            if split_value:
                if scene_id >= 0:
                  self.scene_data[scene_id] = current_scene
                  self.scene_header[scene_id] = current_header
                scene_id = int(split_value.group(1))
                current_status = 1
                current_header = ''
                current_scene = ''
                continue
            elif current_status == 1:
                current_header = line
                current_status = 2
                continue
            elif current_status == 2:
                current_scene = current_scene + line
                continue

        # read the data	
        if current_status == 2 and scene_id >= 0:
            self.scene_data[scene_id] = current_scene
            self.scene_header[scene_id] = current_header


def sanity_check(coref_html_parser, text_parser):
    ''' Checking for sanity while parsing both files '''
    for idx in coref_html_parser.action_data:
      for subid in coref_html_parser.action_data[idx][1]:
        #keyboard()
        begin_id = coref_html_parser.action_data[idx][1][subid][0][1]
        #keyboard()
        end_id   = coref_html_parser.action_data[idx][1][subid][0][2]
        scene_id = coref_html_parser.action_data[idx][0]
        word     = coref_html_parser.action_data[idx][1][subid][1]
        #keyboard()
        try:
          if not word == text_parser.scene_data[scene_id][begin_id:end_id] and not word == 'null':
            print 'idx:', word, 'does not match ', text_parser.scene_data[scene_id][begin_id:end_id]
        except:
          print 'idx:', word, ' not found!'
          keyboard()

def transfer_to_new_scene(coref_html_parser, text_parser_old, text_parser_new):
  ''' transferring to the new cleaner scene breaks '''
  search_half_width = 10
  for idx in coref_html_parser.action_data:
      for subid in coref_html_parser.action_data[idx][1]:
        begin_id = coref_html_parser.action_data[idx][1][subid][0][1]
        end_id   = coref_html_parser.action_data[idx][1][subid][0][2]
        scene_id = coref_html_parser.action_data[idx][0]
        word     = coref_html_parser.action_data[idx][1][subid][1]
        try:
          if not word == text_parser_old.scene_data[scene_id][begin_id:end_id] and not word == 'null':
            print 'idx:', word, 'does not match ', text_parser_old.scene_data[scene_id][begin_id:end_id]
            continue
          else:
            begin_id_big = max(begin_id, 0)
            end_id_big = min(end_id + search_half_width, len(text_parser_old.scene_data[scene_id]))
            search_phrase = text_parser_old.scene_data[scene_id][begin_id_big:end_id_big]
            found_flag = False
            for scene_id_new in text_parser_new.scene_data:
              full_text = text_parser_new.scene_data[scene_id_new]
              if full_text.find(search_phrase) > 0:
                #print 'Found the phrase: %s'%(search_phrase)
                found_flag = True
                break
            if not found_flag:
              print 'Could not find: <%s>'%(search_phrase)
              #keyboard()
        except:
          print 'idx:', word, ' not found!'
          keyboard()


if __name__=='__main__':
    if (len(sys.argv) < 4):
      print "Usage: transferCorefAnnotations <coref_anno_2.xml> <scene_breaks_clean.txt (old)> <scene_breaks.txt (new)"
      exit(-1)
    
    # Parsing the Html coref annotation file
    coref_annofile = open(sys.argv[1], 'r')
    coref_html_parser = CorefHtmlParser()    
    coref_html_parser.parse_file(coref_annofile)
    coref_annofile.close()

    # Parsing the old scene break data
    scene_file_old = open(sys.argv[2], 'r')
    text_parser_old = SceneBreakParser()
    text_parser_old.parse_file(scene_file_old)
    scene_file_old.close()
    
    # Parsing the coref text data
    scene_file_new = open(sys.argv[3], 'r')
    text_parser_new = SceneBreakParser()
    text_parser_new.parse_file(scene_file_new)
    scene_file_new.close()

    transfer_to_new_scene(coref_html_parser, text_parser_old, text_parser_new);

