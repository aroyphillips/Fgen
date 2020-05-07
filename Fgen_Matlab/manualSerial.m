coms = instrfind;

if ~isempty(coms)
    fclose(coms);
end

SerialID = serial('COM4', 'Baudrate',9600);
fopen(SerialID);

N = 1000;
tests = NaN(2,N);
t0 = tic;
for ii = 1:N
flushinput(SerialID);
this = fread(SerialID,1, 'uint8')==255;
dt = toc(t0);
fprintf("Time: %3.5f, Val: %g\n", dt, this);
tests(:,ii) = [this;dt];

end


%teststr = '<^1,2,100,80,30!>';
%{
for char = teststr
    fwrite(SerialID,char);
end
%}


fclose(coms);