%% parse the brat annotation file to get the different arguments and relations


%  params:
%   anno_file - the .ann file
%   text_file - the corresoponding .txt file

%  output:
%   brat_annotation
%       - lines     : the full text
%       - tokens    : the tokens, where tokens(i) corresponds to T<i> in the anno file
%           - beg_char
%           - end_char
%           - name
%           - original_text
%       - relations : the relations, where relations(i) corresponds to R<i> in the anno file
%           - source_token
%           - target_token
%           - name

function brat_annotation = parseBratAnnoFile(anno_file, text_file)

all_lines = textread(text_file, '%s', 'delimiter', '\n');
brat_annotation.lines = '';

for i = 1:numel(all_lines)
  brat_annotation.lines = [brat_annotation.lines all_lines{i} ' '];
end

anno_lines = textread(anno_file, '%s', 'delimiter', '\n');

brat_annotation.tokens = repmat(struct('beg_char', [], 'end_char', [], ...
      'name', [], 'original_text', []), [2*numel(anno_lines) 1]);
brat_annotation.relations = repmat(struct('source_token', [], 'target_token', [], ...
      'name', []), [numel(anno_lines), 1]);

max_token_id = 0;
max_relation_id = 0;

for i = 1:numel(anno_lines)
  splits = strsplit(anno_lines{i});
  
  % parse the token if first char is 'T'
  if (splits{1}(1) == 'T')
    %NOTE: There is +1 due to 0 indexing in the brat file
    token_id = str2num(splits{1}(2:end)) + 1;
    max_token_id = max(token_id, max_token_id);
    brat_annotation.tokens(token_id).name = splits{2};
    brat_annotation.tokens(token_id).beg_char = str2num(splits{3});
    brat_annotation.tokens(token_id).end_char = str2num(splits{4});
    brat_annotation.tokens(token_id).original_text = splits{5};  
    continue;
  end

  % parse the relation if first char is 'R'
  if (splits{1}(1) == 'R')
    relation_id = str2num(splits{1}(2:end)) + 1;
    max_relation_id = max(relation_id, max_relation_id);
    brat_annotation.relations(relation_id).name = splits{2};
    brat_annotation.relations(relation_id).source_token = str2num(splits{3}(7:end)) + 1;
    brat_annotation.relations(relation_id).target_token = str2num(splits{4}(7:end)) + 1;
    continue;
  end
  
  fprintf('WARNING: Annotation file probably corrupt!\n');
  
end


% prune to the maximum token id
if max_token_id > 0
  brat_annotation.tokens = brat_annotation.tokens(1:max_token_id);
else
  brat_annotation.tokens = [];
  fprintf('WARNING <%s>: There are no tokens!\n', anno_file);
end

% prune to the maximum relation id
if max_relation_id > 0
  brat_annotation.relations = brat_annotation.relations(1:max_relation_id);
else
  brat_annotation.relations = [];
  fprintf('WARNING <%s>: There are no relations!\n', anno_file);
end
