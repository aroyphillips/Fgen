function OptionalOperantFixedRewardTime


%%% Provides animal reward located at user specified location if the animal
%%% licks in the specified time. Assumed lick port is port 1.
%%% Lap is reset at the beginning of the trial to ensure physical distancing recording matches bpod recording
%%% Manual Reward by clicking Wire2Low on the Bpod Console. Manual Reset by clicking BNCIn 1 
%%% Written by Roy Phillips for Christine Grienberger at Magee Lab 06/08/21

global BpodSystem

%% Setup (runs once before the first trial)
MaxTrials = 1000; % Maximum number of trials
numLocations = 2; % one possible reward site, plus end of location

LickPort = 1;

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
Thresholds = [140,150];
R.thresholds = round(Thresholds * Dist2Deg); % Calibration dist(cm) * 11.3 = thresh in deg


%--- Define parameters and trial structure
S = BpodSystem.ProtocolSettings; % Loads settings file chosen in launch manager into current workspace as a struct called 'S'
if isempty(fieldnames(S))  % If chosen settings file was an empty struct, populate struct with default settings
    % Define default settings here as fields of S (i.e S.InitialDelay = 3.2)
    % Note: Any parameters in S.GUI will be shown in UI edit boxes. 
    % See ParameterGUI plugin documentation to show parameters as other UI types (listboxes, checkboxes, buttons, text)
        
    S.GUI.WaterValveAmt = 3.5; %  micro liters
    S.GUI.RewardLocationStart = 140; %cm
    S.GUI.RewardLocationEnd = 150;
    S.GUI.RewardLocationDurationT = 0.5; %s
    S.GUI.IsOperant = 1; % 1==operant, 0==non-operant
    
end


%--- Initialize plots and start USB connections to any modules
 BpodParameterGUI('init', S); % Initialize parameter GUI plugin
 TotalRewardDisplay('init');
 RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [1]); % valve open duration from calibration curve, 2-10 ul range
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

LickPortEvent = sprintf('Port%gIn', LickPort);

%% Main loop (runs once per trial)
for currentTrial = 1:MaxTrials
    disp('__________')
    disp(['Trial#: ' num2str(currentTrial)])
    BpodSystem.PluginObjects.currentTrial = currentTrial;
    S = BpodParameterGUI('sync', S); % Sync parameters with BpodParameterGUI plugin
    
%% Update the GUI data
    RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [1]); % valve open duration from calibration curve, 2-10 ul range
    RewardLocationStart = (S.GUI.RewardLocationStart);
    RewardLocationEnd = (S.GUI.RewardLocationEnd);
    RewardLocationDuration = (S.GUI.RewardLocationDurationT);
    isOperant = (S.GUI.IsOperant);
    
    R.wrapPoint = 2160; %6 rotations
    
    Thresholds = round([RewardLocationStart, RewardLocationEnd]*Dist2Deg);
    R.thresholds = Thresholds;
    R.startUSBStream('useTimer'); % Start streaming position data from the rotary encoder   
    
    % Generate a random # to determine trial type
    if isOperant
        fprintf('Operant Reward Location %gcm for %g seconds\n', RewardLocationStart, RewardLocationDuration)
    else
        fprintf('Non-Operant Reward Location %gcm\n', RewardLocationStart)
    end
    locationStart = locations(1);
    locationEnd = locations(2);
    %% State Machine 
    
    
    sma = NewStateMachine(); 
    
    sma = AddState(sma, 'Name', 'StartLap',... % waits for beambreak to be reached
        'Timer', 0,...
        'StateChangeConditions', {'Port3Out', 'ResetPosition', 'BNC1High', 'ResetPosition'},...
        'OutputActions', {});       
    
    sma = AddState(sma, 'Name', 'ResetPosition',... % resets location of track
        'Timer', 0.1,...
        'StateChangeConditions', {'Tup', 'SendTimestamp'},...
        'OutputActions', io.reset); %reset position and send reset signal to 
    
    sma = AddState(sma, 'Name', 'SendTimestamp', ... % send Timestamp and start trial
        'Timer', 0,...
        'StateChangeConditions', {'Tup', 'GoToReward'},...  
        'OutputActions', {'RotaryEncoder1', 1}); 
    
    if isOperant

        sma = AddState(sma, 'Name', 'GoToReward', ... %
            'Timer', 0,...
            'StateChangeConditions', {locationStart, 'WaitForLick', 'Wire2Low', 'ManualReward'},...  
            'OutputActions', {});

        sma = AddState(sma, 'Name', 'WaitForLick', ... %
            'Timer', RewardLocationDuration,...
            'StateChangeConditions', {'Tup', 'Bpod_Lapover', LickPortEvent, 'Reward', 'Wire2Low', 'ManualReward', locationEnd, 'TryAgain'},...  
            'OutputActions', {});
        sma = AddState(sma, 'Name', 'TryAgain', ... % if lick happens in window, give reward 
            'Timer', 0,...
            'StateChangeConditions', {'Tup', 'Bpod_Lapover'},...
            'OutputActions', {});   % turns output 3 to high, indicating release of water
    else
        
        sma = AddState(sma, 'Name', 'GoToReward', ... %
            'Timer', 0,...
            'StateChangeConditions', {locationStart, 'Reward', 'Wire2Low', 'ManualReward'},...  
            'OutputActions', {});
    end
    
    sma = AddState(sma, 'Name', 'Reward', ... % if lick happens in window, give reward 
        'Timer', RewardTime,...
        'StateChangeConditions', {'Tup', 'Bpod_Lapover'},...
        'OutputActions', {'ValveState', 2^0, 'WireState', 5});   % turns output 3 to high, indicating release of water

    sma = AddState(sma, 'Name', 'ManualReward',... % gives reward after clickin WireIn2
        'Timer', RewardTime,...
        'StateChangeConditions', {'Tup', 'Bpod_Lapover'},...
        'OutputActions', {'ValveState', 2^0, 'WireState', 5});   % turns output 3 to high, indicating release of water
 
    sma = AddState(sma, 'Name', 'Bpod_Lapover',... % End lap after reward and wait for reset
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
