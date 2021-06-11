clear;
load('C:\Users\aroyp\Documents\Bpod Local\Data\FakeSubject\TrainFish\Session Data\FakeSubject_TrainFish_20201027_153729.mat')
trials = SessionData.RawEvents.Trial;
numTrials = length(trials);
rewardLocs = NaN(1,numTrials);
badTrials = NaN(1,numTrials);

for trial = 1:21
    events = trials{1,trial}.Events;
    if isfield(events,"RotaryEncoder1_1")
        if length(events.RotaryEncoder1_1)== 1
            rewardLocs(trial) = events.RotaryEncoder1_1;
        else
            badTrials(1,trial) = length(events.RotaryEncoder1_1);
        end
    else
         badTrials(1,trial) = 0;
    end
end

% 
% Trial#: 1
% [Lap_Length_Factor = 0.275]
% [Location goal: 58cm * length_factor = 16cm]
% [End lap location: 49 cm]
% __________
% Trial#: 2
% [Lap_Length_Factor = 0.28]
% [Location goal: 111cm * length_factor = 31cm]
% [End lap location: 50 cm]
% __________
% Trial#: 3
% [Lap_Length_Factor = 0.285]
% [Location goal: 32cm * length_factor = 9cm]
% [End lap location: 51 cm]
% __________
% Trial#: 4
% [Lap_Length_Factor = 0.29]
% [Location goal: 98cm * length_factor = 28cm]
% [End lap location: 52 cm]
% __________
% Trial#: 5
% [Lap_Length_Factor = 0.295]
% [Location goal: 143cm * length_factor = 42cm]
% [End lap location: 53 cm]
% __________
% Trial#: 6
% [Lap_Length_Factor = 0.3]
% [Location goal: 114cm * length_factor = 34cm]
% [End lap location: 54 cm]
% __________
% Trial#: 7
% [Lap_Length_Factor = 0.305]
% [Location goal: 23cm * length_factor = 7cm]
% [End lap location: 55 cm]
% __________
% Trial#: 8
% [Lap_Length_Factor = 0.31]
% [Location goal: 130cm * length_factor = 40cm]
% [End lap location: 55 cm]
% __________
% Trial#: 9
% [Lap_Length_Factor = 0.315]
% [Location goal: 153cm * length_factor = 48cm]
% [End lap location: 56 cm]
% __________
% Trial#: 10
% [Lap_Length_Factor = 0.32]
% [Location goal: 80cm * length_factor = 26cm]
% [End lap location: 57 cm]
% __________
% Trial#: 11
% [Lap_Length_Factor = 0.325]
% [Location goal: 80cm * length_factor = 26cm]
% [End lap location: 58 cm]
% __________
% Trial#: 12
% [Lap_Length_Factor = 0.33]
% [Location goal: 132cm * length_factor = 44cm]
% [End lap location: 59 cm]
% __________
% Trial#: 13
% [Lap_Length_Factor = 0.335]
% [Location goal: 38cm * length_factor = 13cm]
% [End lap location: 60 cm]
% __________
% Trial#: 14
% [Lap_Length_Factor = 0.34]
% [Location goal: 126cm * length_factor = 43cm]
% [End lap location: 61 cm]
% __________
% Trial#: 15
% [Lap_Length_Factor = 0.345]
% [Location goal: 10cm * length_factor = 3cm]
% [End lap location: 62 cm]
% __________
% Trial#: 16
% [Lap_Length_Factor = 0.35]
% [Location goal: 150cm * length_factor = 53cm]
% [End lap location: 63 cm]
% __________
% Trial#: 17
% [Lap_Length_Factor = 0.355]
% [Location goal: 34cm * length_factor = 12cm]
% [End lap location: 64 cm]
% __________
% Trial#: 18
% [Lap_Length_Factor = 0.36]
% [Location goal: 100cm * length_factor = 36cm]
% [End lap location: 64 cm]
% __________
% Trial#: 19
% [Lap_Length_Factor = 0.365]
% [Location goal: 23cm * length_factor = 8cm]
% [End lap location: 65 cm]
% __________
% Trial#: 20
% [Lap_Length_Factor = 0.37]
% [Location goal: 98cm * length_factor = 36cm]
% [End lap location: 66 cm]
% __________
% Trial#: 21
% [Lap_Length_Factor = 0.375]
% [Location goal: 38cm * length_factor = 14cm]
% [End lap location: 67 cm]
% __________
% Trial#: 22
% [Lap_Length_Factor = 0.38]
% [Location goal: 115cm * length_factor = 44cm]
% [End lap location: 68 cm]