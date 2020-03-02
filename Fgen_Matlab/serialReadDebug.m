% Serial Read debug
clear;


coms = instrfind;

if ~isempty(coms)
    fclose(coms);
end

SerialID = serial('COM13', 'BaudRate', 9600);
fopen(SerialID);

test = fread(SerialID,1);
tic
pause(2);
toc

flushinput(SerialID);
test_new = fread(SerialID,1);

pause(2);

flushinput(SerialID);
test_old = fread(SerialID,1);

pause(2)

flushinput(SerialID);
test_new_new = fread(SerialID,1);

%fclose(SerialID);

fprintf("Values on each pass: %g, %g, %g, %g", mean(test), ...
    mean(test_new), mean(test_old), mean(test_new_new));