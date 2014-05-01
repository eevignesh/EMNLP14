%%% deprecate
function annotated_text = markAnnotationsInHtml(verb_data, scene_text)

ctr_e = 1;
extra_text = [];

for j = 1:numel(verb_data{i})
  try
    extra_text(ctr_e).beg_char  = verb_data{i}(j).verb.span(2)+1;
  catch
    fprintf('bug here\n'); keyboard;
  end
  extra_text(ctr_e).word_span = [verb_data{i}(j).verb.span]; 
  extra_text(ctr_e).text      = '<font color="blue"> ( </font>';
  for k = 1:numel(verb_data{i}(j).args)
    if ~isempty(strfind(verb_data{i}(j).args(k).type, 'MIS'))
      font_color = 'red';
    else
      font_color = 'blue';
    end

    extra_text(ctr_e).text = [extra_text(ctr_e).text ...
                              sprintf('<font color="%s">', font_color) ...
                              verb_data{i}(j).args(k).type ':T' ...
                              num2str(verb_data{i}(j).args(k).id) '</font> '];
  end
  extra_text(ctr_e).text     = [extra_text(ctr_e).text '<font color="blue"> ) </font>'];

  ctr_e = ctr_e + 1;
  
  % now find extra text for arguments

  for k = 1:numel(verb_data{i}(j).args)
    arg_beg_char = verb_data{i}(j).args(k).span(2)+1;
    all_beg_chars = cat(2, extra_text.beg_char);
    if (~any(all_beg_chars == arg_beg_char))
      extra_text(ctr_e).beg_char = arg_beg_char;
      token_id = verb_data{i}(j).args(k).id;
      
      if (~isempty(strfind(brat_data.tokens(token_id).name, 'WRONG'))  || ...
          ~isempty(strfind(brat_data.tokens(token_id).name, 'MIS'))) 
        font_color = 'red';
      else
        font_color = 'green';
      end      
      extra_text(ctr_e).text = sprintf('<font color="%s"> (T%d:%s) </font>', ...
                  font_color, token_id, brat_data.tokens(token_id).name);
      extra_text(ctr_e).word_span = [verb_data{i}(j).args(k).span]; 

      ctr_e = ctr_e + 1;
    end
  end

end

[~, sort_ids] = sort(cat(2, extra_text.beg_char), 'ascend');
extra_text = extra_text(sort_ids);
annotated_text = [];

start_char = 1;
for j = 1:numel(extra_text)
  annotated_text = [annotated_text scene_txt{i}(start_char:extra_text(j).beg_char-1) ...
     extra_text(j).text];
  start_char = extra_text(j).beg_char;
end
annotated_text =[annotated_text scene_txt{i}(start_char:end)];

