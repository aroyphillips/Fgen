clear

trainPrd = 60;
trainDur = 100;
trainWid = 120;
trainDly = 50;
trainAmp = 2;
A2V = 0.5;

y =zeros(1,181);

gausAmp = 3;
gausCent =70;
gausLeftWid = 10;
gausRightWid = 10;

gausInds = cumsum(ones(1,181));
% up gaussian
upInds = 1:(gausCent+1);
y(upInds) = gausAmp*exp(-log(2)*((gausInds(upInds)-gausCent)/gausLeftWid).^2);

% down gaussian
downInds = (gausCent+1):170;
y(downInds) = gausAmp*exp(-log(2)*((gausInds(downInds)-gausCent)/gausRightWid).^2);

plot(y)

% prdTs = round(1/trainPrd*1000);
% width = round(1/trainWid*1000);
% numIn1s = ceil(1000/prdTs);
% single_spike = zeros(1,prdTs+1);
% single_spike(1:width+1) = trainAmp/A2V;
% all_spikes = repmat(single_spike, 1, numIn1s);
% spike_y = all_spikes(1:1001);
% 
% sample_spikes = zeros(1,trainDur+1);
% sample_spikes(1:2:end) = trainAmp;
% y((trainDly+1):(trainDly+trainDur+1)) = y((trainDly+1):(trainDly+trainDur+1)) + sample_spikes;
% plot(y);
% 
% 
% 
% train_y = zeros(1,181);
% single_pulse = zeros(1,trainPrd+1); 
% single_pulse(1:trainWid+1) = trainAmp/A2V;
% all_pulses = repmat(single_pulse, 1, ceil(trainDur/trainPrd));
% all_pulses = all_pulses(1:trainDur+1);
% 
% train_inds = [(trainDly+1):(trainDly+trainDur+1)];
% train_y(train_inds) = all_pulses;    
% 
% plot(train_y);
% 
% triAmp = 3;
% triDly = 50;
% triPk = 100;
% triDur = 70;
% triSlopeUp = triAmp/(triPk-triDly);
% triSlopeDown = triAmp/(triPk-triDly-triDur);
% 
% triPulseUp = ([triDly:triPk]-triDly)*triSlopeUp;
% triPulseDown = (0:(triDur+triDly-triPk))*triSlopeDown+triAmp;
% triPulse = [triPulseUp, triPulseDown];
% 
% plot(triPulse);