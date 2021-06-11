function RandomAcclimation

%%% This protocol was written by Roy Phillips 1/15/2020
%%% Designed for a blank belt acclimation where reward is randomly given
%%% every 100-260cm that the animal runs, so that the expected reward
%%% location is every 180cm. User has control of the range about and expected value.

global BpodSystem

%% Setup (runs once before the first trial)
MaxTrials = 1000; % set to number of trials wanted (30 laps for blocked, 1000 for random)
R = RotaryEncoderModule(BpodSystem.ModuleUSB.RotaryEncoder1); % Make sure you pair the encoder with its USB port from the Bpod console
% if Rotary Encoder 1 is not showing up on Bpod, unplug the port and
% replug, refresh until Serial 1 --> Rotary Encoder 1
% if still not working, try to upload RotartyEncoderModule Firmware to the teensy


R.wrapMode = 'Unipolar';
R.wrapPoint = 2160; %6 rotations, 
R.sendThresholdEvents = 'on';

Dist2Deg = 360/(pi*2.54*4)/2; % scale factor to degrees for 4" radius wheels, divides by 2 for 512 tic encoder
R.thresholds = Dist2Deg*[180]; % Thresholds in degrees, correspond to 50, 100, 150 in cm



% random number coefficient to degrees corresponding to soundout

%--- Define parameters and trial structure
S = BpodSystem.ProtocolSettings; % Loads settings file chosen in launch manager into current workspace as a struct called 'S'
if isempty(fieldnames(S))  % If chosen settings file was an empty struct, populate struct with default settings
    % Define default settings here as fields of S (i.e S.InitialDelay = 3.2)
    % Note: Any parameters in S.GUI will be shown in UI edit boxes. 
    % See ParameterGUI plugin documentation to show parameters as other UI types (listboxes, checkboxes, buttons, text)
    
    S.GUI.WaterValveAmt = 3.5; %  micro liters
    S.GUI.RewardCenter = 180; %cm
    S.GUI.RewardRange = 160;
    
end


%--- Initialize plots and start USB connections to any modules
 BpodParameterGUI('init', S); % Initialize parameter GUI plugin
 TotalRewardDisplay('init');
 RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [1]); % valve open duration from calibration curve, 2-10 ul range
 BpodNotebook('init');



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
% R.startUSBStream('useTimer'); % Start streaming position data from the rotary encoder   

%% Wait for belt reset
sma = NewStateMachine(); 
sma = AddState(sma, 'Name', 'WaitForReset',... % waits for start position to be reached, modified to just timeout to next state
    'Timer', 0,...
    'StateChangeConditions', {'Tup', 'ResetPosition'},... 
    'OutputActions', {}); 

sma = AddState(sma, 'Name', 'ResetPosition',... % resets location of track
    'Timer', 0,...
    'StateChangeConditions', {'Tup', 'exit'},...
    'OutputActions', io.reset); 


SendStateMachine(sma);
RawEvents = RunStateMachine;

    
%% Main loop (runs once per trial)
for currentTrial = 1:MaxTrials
    disp('__________')
    disp(['Trial#: ' num2str(currentTrial)])
    BpodSystem.PluginObjects.currentTrial = currentTrial;
    S = BpodParameterGUI('sync', S); % Sync parameters with BpodParameterGUI plugin
    
   
    %% Update the GUI data
    
    RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [1]); % valve open duration from calibration curve, 2-10 ul range
    RewardCenter = (S.GUI.RewardCenter);
    RewardRange = (S.GUI.RewardRange);

    
    %%  Remap R.Thresholds to desired values
    RewardArray = [RewardCenter-RewardRange/2, RewardCenter+RewardRange/2];
    RewardLocation = randi(RewardArray,1);
    
    Threshold = [RewardLocation] * Dist2Deg;
    
    fprintf('Reward Location: %g\n', Threshold/Dist2Deg)
    
    R.wrapPoint = 2160;
    R.thresholds = Threshold; 
    R.startUSBStream('useTimer'); % Start streaming position data from the rotary encoder   
    
     %% start state machine
     % Non Operant Reward
    sma = NewStateMachine();
    
    sma = AddState(sma, 'Name', 'SendTimestamp', ... % send Timestamp and start trial
        'Timer', 0,...
        'StateChangeConditions', {'Tup', 'GoToLocation'},...  
        'OutputActions', {'RotaryEncoder1', 1, 'WireState', 0});
    
    sma = AddState(sma, 'Name', 'GoToLocation', ... % go to trigger 1 location
    'Timer', 0,...
    'StateChangeConditions', {'RotaryEncoder1_1', 'Reward', 'Wire2Low', 'ManualReward'},...
    'OutputActions', {});

    sma = AddState(sma, 'Name', 'ManualReward',... % gives reward after clickin WireIn2
     'Timer', RewardTime,...
     'StateChangeConditions', {'Tup', 'GoToLocation'},...
     'OutputActions', {'ValveState', 2^0, 'WireState', 4});   % turns output 3 to high, indicating release of water


    sma = AddState(sma, 'Name', 'Reward', ... % give reward 
        'Timer', RewardTime,...
        'StateChangeConditions', {'Tup', 'ResetPosition'},...
        'OutputActions', {'ValveState', 2^0, 'WireState', 4});   % turns output 3 to high, indicating release of water

    sma = AddState(sma, 'Name', 'ResetPosition',... % resets location of track
        'Timer', 0,...
        'StateChangeConditions', {'Tup', 'exit'},...
        'OutputActions', io.reset); 
    disp('Exit State')
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
    %if ~isnan(BpodSystem.Data.RawEvents.Trial{currentTrial}.States.Reward(1))
    %    TotalRewardDisplay('add', S.GUI.WaterValveAmt);
    %end
    
%     BpodSystem.Data = BpodNotebook('sync', BpodSystem.Data);  %synchronizes notebook data at the end of each trial
    
    %--- This final block of code is necessary for the Bpod console's pause and stop buttons to work
    HandlePauseCondition; % Checks to see if the protocol is paused. If so, waits until user resumes.
    if BpodSystem.Status.BeingUsed == 0
        return
    end
    R.stopUSBStream
end
disp('All finished! Give the mouse a treat and a pat on the head ;-)')
R.stopUSBStream;
