coms = instrfind;

if ~isempty(coms)
    fclose(coms);
end

SerialID = serial('COM13','BaudRate',9600);
fopen(SerialID);
fwrite(SerialID,'<','char');
temp = sprintf('%2.2f',5);
fwrite(SerialID,temp,'char');
fread(SerialID,1,'double');
temp = sprintf('%2.2f',1);
fwrite(SerialID,temp,'char');
fread(SerialID,1,'double');
fwrite(SerialID,'>','char');

figure(1)
c = 0;
while(1)
    c=c+1;
    data(c) = fread(SerialID,1,'double');
   
    plot(c,data(c),'.b')
    if c == 1
        hold on
    end
    drawnow;
end
   
% fclose(SerialID);