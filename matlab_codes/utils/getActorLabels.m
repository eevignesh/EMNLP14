function actor_bags = getActorLabels(script_srt_align_file)

fid = fopen(script_srt_align_file);

speaker_bags = [];

sp_ctr = 0;
flag = 0;

tline = fgetl(fid);
while ischar(tline)
  o = regexp(tline, '(\d+):(\d+):(\d+)', 'tokens');
  sc_r = regexp(tline, 'Scene:(\d+)\s+:\s+\((\d+),(\d+)\)', 'tokens');
  if ~isempty(o)
    if(flag==0)
      sp_ctr = sp_ctr + 1;
      time_beg = str2num(o{1}{1})*3600 + str2num(o{1}{2})*60 + str2num(o{1}{3});
      speaker_bags(sp_ctr).time_beg = time_beg;
      flag = 1;
    else
      if(flag==1)
        time_end = str2num(o{1}{1})*3600 + str2num(o{1}{2})*60 + str2num(o{1}{3});
        speaker_bags(sp_ctr).time_end = speaker_bags(sp_ctr).time_beg + time_end - 1;

        speaker_bags(sp_ctr).time_beg = speaker_bags(sp_ctr).time_beg + 15;
        speaker_bags(sp_ctr).time_end = speaker_bags(sp_ctr).time_end - 15;
        flag = 2;
      else
        fprintf('Something wrong in file parsing\n');
        keyboard;
        return;
      end
    end
  else
    flag = 0;
    %keyboard;
    if ~isempty(sc_r)
      %keyboard;
      speaker_bags(sp_ctr).scene = str2num(sc_r{1}{1});
      speaker_bags(sp_ctr).beg_ctr = str2num(sc_r{1}{2});
      speaker_bags(sp_ctr).end_ctr = str2num(sc_r{1}{3});
    else
      if(numel(tline) >= 2)
        if ~isfield(speaker_bags(sp_ctr), 'description')
          speaker_bags(sp_ctr).description = {};
        end
        %try
          speaker_bags(sp_ctr).description{end+1} = tline;      
        %catch
        %  keyboard;
        %end
      end
    end
  end
  tline = fgetl(fid);
end
actor_bags = speaker_bags;

fclose(fid);
