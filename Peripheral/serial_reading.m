if ~isempty(instrfind)
fclose(instrfind);
delete(instrfind);
end

close all;
clearvars;

SerialPort='COM5';   %serial port


%%Set up the serial port object
s = serial(SerialPort,'BaudRate',9600);
fopen(s);

flushinput(s);
% scan the distance
while ~KbCheck
currentPosition = fscanf(s,'%g',4);
fprintf("Current position = %g\n", currentPosition);
flushinput(s);
end

%% Clean up the serial port
fclose(s);
delete(s);
clear s;