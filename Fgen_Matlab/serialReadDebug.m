% Serial Read debug
clear;


coms = instrfind;

if ~isempty(coms)
    fclose(coms);
end

SerialID = serial('COM4', 'BaudRate', 9600);
fopen(SerialID);
test2= 0;

SerialID.Terminator = 'LF';
% for i = 1:10
t0 = tic;
dt = 0;
while (dt<60)
     dt = toc(t0);
     flushinput(SerialID);
     test = fread(SerialID,1,'char');
     test2 = fscanf(SerialID,'%s', 1) == 49;
%      fprintf("Reading: %g \n", test);
     if test2         
         fprintf("Timing = %g \n", dt);
         fprintf("Scaning: %g \n", test2);
         fprintf("__________\n");
     end
end
% end
% 
% test = fread(SerialID,1);
% tic
% pause(2);
% toc
% 
% flushinput(SerialID);
% test_new = fread(SerialID,1);
% 
% pause(2);
% 
% flushinput(SerialID);
% test_old = fread(SerialID,1);
% 
% pause(2)
% 
% flushinput(SerialID);
% test_new_new = fread(SerialID,1);

fclose(SerialID);

% fprintf("Values on each pass: %g, %g, %g, %g", mean(test), ...
%     mean(test_new), mean(test_old), mean(test_new_new));