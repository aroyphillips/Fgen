function ProbabilisticReward2


%%% Provides animal reward located at one of 2 user-specified locations, selected based on an input probability for the second location
%%% Lap is reset in the middle of the trial to ensure physical distancing recording matches bpod recording
%%% Softcoded end of trial is at 150cm. User-specified reward window cannot exceed this position
%%% Manual Reward by clicking BNCIn 2 on the Bpod Console. Manual Reset 
%%% Written by Roy Phillips for Christine Greenberger at Magee Lab 06/08/21

global BpodSystem

%% Setup (runs once before the first trial)
MaxTrials = 1000; % Maximum number of trials
numLocations = 3; % two possible reward sites 

R = RotaryEncoderModule(BpodSystem.ModuleUSB.RotaryEncoder1); % Make sure you pair the encoder with its USB port from the Bpod console
% if Rotary Encoder is not showing up on Bpod, unplug the serial port and
% replug, refresh until Serial 1 --> Rotary Encoder 1

R.wrapMode = 'Unipolar';
R.wrapPoint = 2160; %6 rotations
R.sendThresholdEvents = 'on';
% Thresholds in degrees, For example: correspond to 15, 35, 55, 75, 112, 119, 172, 179 in cm
% R.thresholds = [169 338 507 676 1263 1342 1940 2019]; 
% Fish: First column assigns the locationn of reward, 339 means 30cm. 1119
% meaans 90cm after the lapReset.

Dist2Deg = 360/(pi*2.54*4*2); % divide by 2 for rotary encoder
Thresholds = [30, 90];
R.thresholds = round(Thresholds * Dist2Deg); % Calibration dist(cm) * 11.3 = thresh in deg


%--- Define parameters and trial structure
S = BpodSystem.ProtocolSettings; % Loads settings file chosen in launch manager into current workspace as a struct called 'S'
if isempty(fieldnames(S))  % If chosen settings file was an empty struct, populate struct with default settings
    % Define default settings here as fields of S (i.e S.InitialDelay = 3.2)
    % Note: Any parameters in S.GUI will be shown in UI edit boxes. 
    % See ParameterGUI plugin documentation to show parameters as other UI types (listboxes, checkboxes, buttons, text)
    S.GUI.WaterValveAmt = 5; %  micro liters
    S.GUI.RewardLocation1 = 50; %cm PLUS actual location is 30cm, beam breaker is 20cm backward 
    S.GUI.RewardLocation2 = 110; %cm
    S.GUI.Location2Probability = 0; % = 1 for Location 2 only, = 0 for Location 1 only, and 0.5 for unweighted random
    S.GUI.SearchForLap = 0; % boolean 1 or 0 to specify whether to change the reset signal search state location, 0 defaults to 160cm
    S.GUI.SearchDistance = 160; % customize location to begin searching for reset signal, helps alleviate incongruencies between Bpod & Wavesurfer laps
end


%--- Initialize plots and start USB connections to any modules
 BpodParameterGUI('init', S); % Initialize parameter GUI plugin
 TotalRewardDisplay('init');
 RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [2]); % valve open duration from calibration curve, 2-10 ul range
 BpodNotebook('init');

%**** define events (Locations)
    for i=1:numLocations
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


%% Main loop (runs once per trial)
for currentTrial = 1:MaxTrials
    disp('__________')
    disp(['Trial#: ' num2str(currentTrial)])
    BpodSystem.PluginObjects.currentTrial = currentTrial;
    S = BpodParameterGUI('sync', S); % Sync parameters with BpodParameterGUI plugin
    
%% Update the GUI data
    RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [2]); % valve open duration from calibration curve, 2-10 ul range
    Location2Probability = (S.GUI.Location2Probability);
    RewardLocation1 = (S.GUI.RewardLocation1);
    RewardLocation2 = (S.GUI.RewardLocation2);
    SearchForLap = (S.GUI.SearchForLap);
    SearchDistance = (S.GUI.SearchDistance);
      
    R.wrapPoint = 2160; %6 rotations
    
    % SearchForLap provides a third location at which point the bpod
    % waits for a reset signal to avoid any overlap in distances
    if SearchForLap==0
        Thresholds = round([RewardLocation1, RewardLocation2, 160]*Dist2Deg);
    elseif SearchForLap==1
        Thresholds = round([RewardLocation1, RewardLocation2,SearchDistance]*Dist2Deg);
    end
    
    R.thresholds = Thresholds;
    R.startUSBStream('useTimer'); % Start streaming position data from the rotary encoder   
    
    % Generate a random # to determine trial type
    disp(strcat('Location 2 Probability: ', num2str(Location2Probability)))

    % generate a random number and compare against the location probability
    hack = rand;
    if hack>=Location2Probability
    id.l=1;
    else
    id.l=2;
    end

    disp(['Location goal: ' num2str(id.l)])
    disp(['Reward Position (cm)', num2str(Thresholds(id.l)/Dist2Deg)])
    locationStart = locations(id.l);
    searchLocation = locations(3);
    %% State Machine 
    
    %%% --- Th
    
    sma = NewStateMachine(); 
    
    if SearchForLap ==0
        sma = AddState(sma, 'Name', 'WaitForReset',... % waits for beambreak to be reached
            'Timer', 0,...
            'StateChangeConditions', {'Port2Out', 'ResetPosition'},...
            'OutputActions', {}); 
    
    elseif SearchForLap == 1
        sma = AddState(sma, 'Name', 'WaitForSearch',... % waits for beambreak to be reached
            'Timer', 0,...
            'StateChangeConditions', {searchLocation, 'WaitForReset', 'BNC1High', 'ResetPosition'},...
            'OutputActions', {}); 
        sma = AddState(sma, 'Name', 'WaitForReset',... % waits for beambreak to be reached
            'Timer', 0,...
            'StateChangeConditions', {'Port2Out', 'ResetPosition'},...
            'OutputActions', {}); 
    end
        
    
    sma = AddState(sma, 'Name', 'ResetPosition',... % resets location of track
        'Timer', 0.1,...
        'StateChangeConditions', {'Tup', 'GoToReward'},...
        'OutputActions', {'RotaryEncoder1', 1,'RotaryEncoder1', 2,'BNC1',1}); %reset position

    sma = AddState(sma, 'Name', 'GoToReward', ... %
        'Timer', 0,...
        'StateChangeConditions', {locationStart, 'Reward'},...  
        'OutputActions', {'WireState', id.l}); % give wavesurfer indication which location the reward will be, "bpod target" high for location 2

    sma = AddState(sma, 'Name', 'Reward', ... % if lick happens in window, give reward 
        'Timer', RewardTime,...
        'StateChangeConditions', {'Tup', 'BPOD_Lapover'},...
        'OutputActions', {'ValveState', 2^1, 'WireState', id.l+4});   % turns output 3 to high, indicating release of water

    sma = AddState(sma, 'Name', 'BPOD_Lapover',... % End lap after reward and wait for reset
        'Timer', 0.1,...
        'StateChangeConditions', {'Tup', 'exit'},...
        'OutputActions', {}); 

    
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
       %--- no longer used since this protocol does not do online plotting 
%     if ~isnan(BpodSystem.Data.RawEvents.Trial{currentTrial}.States.Reward(1))
%         TotalRewardDisplay('add', S.GUI.WaterValveAmt);
%     end
    
    BpodSystem.Data = BpodNotebook('sync', BpodSystem.Data);  %synchronizes notebook data at the end of each trial
    
    %--- This final block of code is necessary for the Bpod console's pause and stop buttons to work
    HandlePauseCondition; % Checks to see if the protocol is paused. If so, waits until user resumes.
    if BpodSystem.Status.BeingUsed == 0
        return
    end
    R.stopUSBStream

end
disp('All finished! Give the mouse a treat and a pat on the head ;-)')
R.stopUSBStream;
