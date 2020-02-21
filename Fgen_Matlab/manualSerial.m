coms = instrfind;

if ~isempty(coms)
    fclose(coms);
end

SerialID = serial('COM13', 'Baudrate',9600);
fopen(SerialID);
%fread(SerialID);

amp = 3;


teststr = '<^1,2,100,80,30!>';
for char = teststr
    fwrite(SerialID,char);
end