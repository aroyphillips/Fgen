% Serial Read debug
clear;


coms = instrfind;

if ~isempty(coms)
    fclose(coms);
end

SerialID = serial('COM4', 'BaudRate', 9600);
fopen(SerialID);

% SerialID.Terminator = 'LF';
% for i = 1:10
t0 = tic;
dt = 0;

N= 10^6;
tests = NaN(2,N);
ii = 1;
while (dt<30)
     dt = toc(t0);
     flushinput(SerialID);
     test = fread(SerialID,1, 'uint8') == 255;
     if test         
         fprintf("Timing = %g \n", dt);
         fprintf("Reading: %g \n", test);
         fprintf("__________\n");
     end
     tests(:,ii) = [test;dt];
     ii = ii+1;
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