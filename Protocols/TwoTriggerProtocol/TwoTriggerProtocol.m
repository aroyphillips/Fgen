function TwoTriggerProtocol

%%% This protocol was originally written by Roy Phillips/ Randy Chitwood / Sophie
%%% Clayton. SV has just modified and adapted it to use BPOD Waveplayer and
%%% generate random block of trials based on the inherent structure. 
%%% Modified 12/13/19 by RP to fit YiDing's Protocol: 
%%% Fully Controllable Release of Water, and trigger to stimulus map in wavesurfer

global BpodSystem

%% Setup (runs once before the first trial)
MaxTrials = 1000; % set to number of trials wanted (30 laps for blocked, 1000 for random)
R = RotaryEncoderModule(BpodSystem.ModuleUSB.RotaryEncoder1); % Make sure you pair the encoder with its USB port from the Bpod console
% if Rotary Encoder 1 is not showing up on Bpod, unplug the port and
% replug, refresh until Serial 1 --> Rotary Encoder 1
% if still not working, try to upload RotartyEncoderModule Firmware to the teensy


R.wrapMode = 'Unipolar';
R.wrapPoint = 2160; %6 rotations
R.sendThresholdEvents = 'on';

Dist2Deg = 360/(pi*2.54*4)/2; % scale factor to degrees for 4" radius wheels, divides by 2 for 512 tic encoder
R.thresholds = Dist2Deg*[50, 100, 150, 179]; % Thresholds in degrees, correspond to 50, 100, 150 in cm



% random number coefficient to degrees corresponding to soundout

%--- Define parameters and trial structure
S = BpodSystem.ProtocolSettings; % Loads settings file chosen in launch manager into current workspace as a struct called 'S'
if isempty(fieldnames(S))  % If chosen settings file was an empty struct, populate struct with default settings
    % Define default settings here as fields of S (i.e S.InitialDelay = 3.2)
    % Note: Any parameters in S.GUI will be shown in UI edit boxes. 
    % See ParameterGUI plugin documentation to show parameters as other UI types (listboxes, checkboxes, buttons, text)
    %S.GUI.WaterValveAmt = 10; %s
    S.GUI.WaterValveAmt = 3.5; %  micro liters
    S.GUI.RewardLocation = 100; %cm
    S.GUI.StimulusTrigger1 = 50 %cm
    S.GUI.StimulusTrigger2 = 150; %cm
    
end


%--- Initialize plots and start USB connections to any modules
 BpodParameterGUI('init', S); % Initialize parameter GUI plugin
 TotalRewardDisplay('init');
 RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [1]); % valve open duration from calibration curve, 2-10 ul range
 BpodNotebook('init');

 locations = cell(1,4);
%**** define events (Locations)
    for i=1:4
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
    RewardLocation = (S.GUI.RewardLocation);
    Trigger1 = (S.GUI.StimulusTrigger1);
    Trigger2 = (S.GUI.StimulusTrigger2);
    
    %%  Remap R.Thresholds to desired values
    
    Thresholds = [RewardLocation, Trigger1, Trigger2, 179] * Dist2Deg;
    [sortedThresholds, idxThresholds] = sort(Thresholds);
    
    fprintf('Reward Location: %g\n Trigger1:%g\n Trigger2: %g\n', Thresholds(1)/Dist2Deg,Thresholds(2)/Dist2Deg, Thresholds(3)/Dist2Deg)
    fprintf('Angle: %g\n', sortedThresholds)
    
    R.wrapPoint = 2160;
    R.thresholds = ceil(sortedThresholds); 
    R.startUSBStream('useTimer'); % Start streaming position data from the rotary encoder   

    
    
    disp('go')
    % determine threshold numbers 
     id.l = idxThresholds(1); % reward location 
     id.t1 = idxThresholds(2); % trigger 1
     id.t2 = idxThresholds(3); % trigger 2 
     
     RewardSite = locations(id.l);  % start of location window
     trigger1Site = locations(id.t1);
     trigger2Site = locations(id.t2);
     reset = locations(4);
     
     %% start state machine
     % Non Operant Reward
    sma = NewStateMachine();
    
    sma = AddState(sma, 'Name', 'SendTimestamp', ... % send Timestamp and start trial
        'Timer', 0,...
        'StateChangeConditions', {'Tup', 'GoToLocation'},...  
        'OutputActions', {'RotaryEncoder1', 1, 'WireState', 0});
    
    sma = AddState(sma, 'Name', 'GoToLocation', ... % go to trigger 1 location
    'Timer', 10,...
    'StateChangeConditions', {trigger1Site,'Trigger1', RewardSite, 'Reward', trigger2Site, 'Trigger2', reset, 'WaitForReset'},...
    'OutputActions', {});

    sma = AddState(sma, 'Name', 'Trigger1', ... % trigger 1 TTL wire 1
    'Timer', 0,...
    'StateChangeConditions', {'Tup','GoToLocation'},...
    'OutputActions', {'WireState', 1});

    sma = AddState(sma, 'Name', 'Trigger2', ... % trigger 2 TTL wire 2
    'Timer', 0,...
    'StateChangeConditions', {'Tup','GoToLocation'},...
    'OutputActions', {'WireState', 2});

    sma = AddState(sma, 'Name', 'Reward', ... % give reward 
        'Timer', RewardTime,...
        'StateChangeConditions', {'Tup', 'GoToLocation'},...
        'OutputActions', {'ValveState', 2^1, 'WireState', 4});   % turns output 3 to high, indicating release of water

    sma = AddState(sma, 'Name', 'WaitForReset',... % waits for start position to be reached
        'Timer', 10,...
        'StateChangeConditions', {'Wire1Low', 'ResetPosition', 'Tup', 'ResetPosition'},...
        'OutputActions', {}); 

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
    i=i+1;
    R.stopUSBStream
end
disp('All finished! Give the mouse a treat and a pat on the head ;-)')
R.stopUSBStream;
