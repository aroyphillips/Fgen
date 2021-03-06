function FlexibleRewardFish

%%% This protocol was originally written by Roy Phillips/ Randy Chitwood / Sophie
%%% Clayton. SV has just modified and adapted it to use BPOD Waveplayer and
%%% generate random block of trials based on the inherent structure. 

global BpodSystem

%% Setup (runs once before the first trial)
MaxTrials = 1000; % set to number of trials wanted (30 laps for blocked, 1000 for random)

R = RotaryEncoderModule(BpodSystem.ModuleUSB.RotaryEncoder1); % Make sure you pair the encoder with its USB port from the Bpod console
% if Rotary Encoder 1 is not showing up on Bpod, unplug the port and
% replug, refresh until Serial 1 --> Rotary Encoder 1
%% Resolve WavePlayer USB port
if (isfield(BpodSystem.ModuleUSB, 'WavePlayer1'))
    WavePlayerUSB = BpodSystem.ModuleUSB.WavePlayer1;
else
    error('Error: To run this protocol, you must first pair the WavePlayer1 module with its USB port. Click the USB config button on the Bpod console.')
end
%% Create an instance of the audioPlayer module
W = BpodWavePlayer(WavePlayerUSB);


%% Define stimuli and send to analog module
SF = 50000;% A.Info.maxSamplingRate; % Use max supported sampling rate
Sound1 = GenerateChirp(SF, 2500, 1)*3; % Sampling freq (hz), ExponentialBase, duration (s): SF,5000,1 goes exponentially from 2000(hardcoded) to ~4500 Hz;
%Sound1 = GenerateSineWave(SF, 400, 1)*.9; % Sampling freq (hz), Sine frequency (hz), duration (s)
Sound2 = GenerateInvChirp(SF, 2500, 1)*3; % Sampling freq (hz), ExponentialBase, duration (s): SF,5000,1 goes exponentially from ~4500 to 2000(hardcoded) Hz;
%Sound2 = GenerateSineWave(SF, 4000, 1)*.9; % Sampling freq (hz), Sine frequency (hz), duration (s)
Sound3 = GeneratePulses(SF, 50,5, 1)*3; %Sampling freq(hz),PulseDur(ms),Pulse Freq(hz),duration(s)
PunishSound = (rand(1,SF*.5)*2) - 1;
W.SamplingRate = SF;
W.TriggerMode = 'Master';
W.loadWaveform(1, Sound1);
W.loadWaveform(2, Sound2);
W.loadWaveform(3, Sound3);

% Set Bpod serial message library with correct codes to trigger sounds 1-4 on analog output channels 1-2
analogPortIndex = find(strcmp(BpodSystem.Modules.Name, 'WavePlayer1'));
if isempty(analogPortIndex)
    error('Error: Bpod WavePlayer module not found. If you just plugged it in, please restart Bpod.')
end

PlayCh1Light_L = ['P' 1 2]; % Play, Channel in Bits, Sound(0,1,2)
PlayCh2Light_R = ['P' 2 2];
PlaySound1 = ['P' 4 0];
PlaySound2 = ['P' 8 1];
%LoadSerialMessages('AudioPlayer1', {['P' 0], ['P' 1], ['P' 2], ['P' 3],['X']});
LoadSerialMessages('WavePlayer1',{PlayCh1Light_L,PlayCh2Light_R,PlaySound1,PlaySound2});


R.wrapMode = 'Unipolar';
R.wrapPoint = 2160; %6 rotations
R.sendThresholdEvents = 'on';
% Thresholds in degrees, correspond to 15, 35, 55, 75, 112, 119, 172, 179 in cm
% R.thresholds = [169 338 507 676 1263 1342 1940 2019]; 
% Fish: First column assigns the locationn of reward, 339 means 30cm. 1119
% meaans 90cm after the lapReset.
Dist2Deg = 360/(pi*2.54*4);
Thresholds = [30, 90];
R.thresholds = Thresholds * Dist2Deg; % Calibration dist(cm) * 11.3 = thresh in deg

%Thresh 1 = Reward Location 1
%Thresh 2 =  Reward Location 2


% random number coefficient to degrees corresponding to soundout

%R.streamUI;


%--- Define parameters and trial structure
S = BpodSystem.ProtocolSettings; % Loads settings file chosen in launch manager into current workspace as a struct called 'S'
if isempty(fieldnames(S))  % If chosen settings file was an empty struct, populate struct with default settings
    % Define default settings here as fields of S (i.e S.InitialDelay = 3.2)
    % Note: Any parameters in S.GUI will be shown in UI edit boxes. 
    % See ParameterGUI plugin documentation to show parameters as other UI types (listboxes, checkboxes, buttons, text)
    %S.GUI.WaterValveAmt = 10; %s
    S.GUI.WaterValveAmt = 0; %  micro liters
%     S.GUI.BlockMin =10; to set blocked random rewards, change lines 10 and 144
%     S.GUI.BlockMax =15;
    S.GUI.Location2Probability = 0; % = 1 for Location 2 only, = 0 for Location 1 only, and 0.5 for random
    S.GUI.RewardLocation1 = 30; %cm
    S.GUI.RewardLocation2 = 90; %cm
end


%--- Initialize plots and start USB connections to any modules
 BpodParameterGUI('init', S); % Initialize parameter GUI plugin
 TotalRewardDisplay('init');
 RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [1]); % valve open duration from calibration curve, 2-10 ul range
 BpodNotebook('init');

%**** define events (Locations)
    for i=1:2
       locations{1,i} = strcat('RotaryEncoder1_', num2str(i));
    end

% Set message# 1 to a 2-byte message: ['#' 1] (For rotary encoder: timestamp an incoming byte: 1)
% Set message# 2 to a 2-byte message: ['Z' 'E'] (For rotary encoder: Zero position, Enable thresholds)
LoadSerialMessages('RotaryEncoder1', {['#' 1], ['Z' 'E']});

%**** define outputs
    io.reward = {'ValveState', 2^1};            % Valve on port 2
    io.markTrialStart = {'RotaryEncoder1', 1};  % Send message#1 (see above) to the encoder: byte 1, to indicate trial-start
    io.reset = {'RotaryEncoder1', 2, 'BNC1',1};           % reset encoder position and re-enable thresholds
    io.led1 = {'PWM3', 255};                    % LED on output port3
    io.led2 = {'PWM4',255};                     % LED on output port4
BpodSystem.Data.PositionData = cell(1,1);
BpodSystem.PluginObjects.currentTrial = 1;
%R.streamUI
%R.startUSBStream('useTimer'); % Start streaming position data from the rotary encoder   

%% Wait for belt reset
% sma = NewStateMachine(); 
% sma = AddState(sma, 'Name', 'WaitForReset',... % waits for start position to be reached
%     'Timer', 0,...
%     'StateChangeConditions', {'Port2Out', 'ResetPosition'},...
%     'OutputActions', {}); 
% 
% sma = AddState(sma, 'Name', 'ResetPosition',... % resets location of track
%     'Timer', 0.1,...
%     'StateChangeConditions', {'Tup', 'exit'},...
%     'OutputActions', io.reset); 
% 
% 
% SendStateMachine(sma);
% RawEvents = RunStateMachine;
% 
Trial_No = 1;
for i = 1:200
    odd_i = 1:2:200;                                % modified SV 10/16/19
    %if i == 1 | i == 3 | i == 5 | i == 7 | i == 9
    if(ismember(i,odd_i))                           % modified SV 10/16/19
        S.GUI.Location2Probability = 1;
    else
        S.GUI.Location2Probability = 0; 
    end
    
    
    block=1000; %randi([S.GUI.BlockMin,S.GUI.BlockMax],1,1); % modified SV 10/16/19
%% Main loop (runs once per trial)
for currentTrial = 1:block
    disp('__________')
    disp(['Trial#: ' num2str(Trial_No)])
    disp(['Block Trial#: ' num2str(currentTrial)])
    BpodSystem.PluginObjects.currentTrial = currentTrial;
    S = BpodParameterGUI('sync', S); % Sync parameters with BpodParameterGUI plugin
    
%% Update the GUI data
    RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [1]); % valve open duration from calibration curve, 2-10 ul range
    Location2Probability = (S.GUI.Location2Probability);
    RewardLocation1 = (S.GUI.RewardLocation1); 
    RewardLocation2 = (S.GUI.RewardLocation2);

    R.wrapPoint = 2160; %6 rotations
    Thresholds = [RewardLocation1, RewardLocation2]*Dist2Deg;
    %Sort thresholds to allow for reward locations to be anywhere
    [sortedThresholds, idxThresholds] = sort(Thresholds);
    locations = locations(idxThresholds);
    R.thresholds = sortedThresholds;
    R.startUSBStream('useTimer'); % Start streaming position data from the rotary encoder   

     % Generate a random # to determine trial type
        disp(strcat('Location 2 Probability: ', num2str(Location2Probability)))
        
         hack = rand;
         if hack>=Location2Probability
           id.s=1;
          id.l=1;
         else
           id.s=2;
           id.l=2;
         end

         disp(['Location goal: ' num2str(id.s)])
         %disp(['Sound Location: ' num2str(soundHack)])
         %disp(['Time Delay:' num2str(timeHack)])
         locationStart = locations(id.l);  % start of location window


         % NON OPERANT TRAINING
            sma = NewStateMachine(); 
            
            % Non Operant 2 Locations

            sma = AddState(sma, 'Name', 'WaitForReset',... % waits for start position to be reached
                'Timer', 0,...
                'StateChangeConditions', {'Port2Out', 'ResetPosition'},...
                'OutputActions', {}); 

            sma = AddState(sma, 'Name', 'ResetPosition',... % resets location of track
                'Timer', 0.1,...
                'StateChangeConditions', {'Tup', 'SendTimestamp'},...
                'OutputActions', io.reset);
            
            sma = AddState(sma, 'Name', 'SendTimestamp', ... % play one of two tones to cue location of reward
                'Timer', 0,...
                'StateChangeConditions', {locationStart, 'Reward'},...  %skip playing of sound
                'OutputActions', {'RotaryEncoder1', 1, 'WireState', id.l});

            sma = AddState(sma, 'Name', 'Reward', ... % if lick happens in window, give reward 
                'Timer', RewardTime,...
                'StateChangeConditions', {'Tup', 'BPOD_Lapover'},...
                'OutputActions', {'ValveState', 2^1, 'WireState', id.l+1});   % turns output 3 to high, indicating release of water

            sma = AddState(sma, 'Name', 'BPOD_Lapover',... % waits for start position to be reached
                'Timer', 0.1,...
                'StateChangeConditions', {'Tup', 'exit'},...
                'OutputActions', {'WireState', id.l+4}); 

    
    SendStateMatrix(sma); % Send state machine to the Bpod state machine device
    RawEvents = RunStateMatrix; % Run the trial and return events
    %--- Package and save the trial's data, update plots
    if ~isempty(fieldnames(RawEvents)) % If you didn't stop the session manually mid-trial
        BpodSystem.Data.PositionData{currentTrial} = R.readUSBStream;
        BpodSystem.Data = AddTrialEvents(BpodSystem.Data,RawEvents); % Adds raw events to a human-readable data struct
        BpodSystem.Data.TrialSettings(currentTrial) = S; % Adds the settings used for the current trial to the Data struct (to be saved after the trial ends)
        SaveBpodSessionData; % Saves the field BpodSystem.Data to the current data file
    end 
       %--- Typically a block of code here will update online plots using the newly updated BpodSystem.Data
%     if ~isnan(BpodSystem.Data.RawEvents.Trial{currentTrial}.States.Reward(1))
%         TotalRewardDisplay('add', S.GUI.WaterValveAmt);
%     end
    
    BpodSystem.Data = BpodNotebook('sync', BpodSystem.Data);  %synchronizes notebook data at the end of each trial
    
    %--- This final block of code is necessary for the Bpod console's pause and stop buttons to work
    HandlePauseCondition; % Checks to see if the protocol is paused. If so, waits until user resumes.
    if BpodSystem.Status.BeingUsed == 0
        return
    end
    i=i+1;
    Trial_No = Trial_No + 1;
    R.stopUSBStream; % necessary to stop stream after each lab to reconfigure user inputs
end
end
disp('All finished! Give the mouse a treat and a pat on the head ;-)')
R.stopUSBStream;
