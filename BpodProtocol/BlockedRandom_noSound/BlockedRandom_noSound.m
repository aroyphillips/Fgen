function BlockedRandom_noSound

%%% This protocol was originally written by Roy Phillips/ Randy Chitwood / Sophie
%%% Clayton. SV has just modified and adapted it to use BPOD Waveplayer and
%%% generate random block of trials based on the inherent structure. 
%%% Cleaned up 06/11/2021 by RP and removed sound for use on Christine's rig

global BpodSystem

%% Setup (runs once before the first trial)
MaxBlocks = 200; % set to number of blocks wanted. Total trials will be blockLenght * MaxBlocks

R = RotaryEncoderModule(BpodSystem.ModuleUSB.RotaryEncoder1); % Make sure you pair the encoder with its USB port from the Bpod console
% if Rotary Encoder 1 is not showing up on Bpod, unplug the port and
% replug, refresh until Serial 1 --> Rotary Encoder 1
% %% Resolve WavePlayer USB port
% if (isfield(BpodSystem.ModuleUSB, 'WavePlayer1'))
%     WavePlayerUSB = BpodSystem.ModuleUSB.WavePlayer1;
% else
%     error('Error: To run this protocol, you must first pair the WavePlayer1 module with its USB port. Click the USB config button on the Bpod console.')
% end
% %% Create an instance of the audioPlayer module
% W = BpodWavePlayer(WavePlayerUSB);


% %% Define stimuli and send to analog module
% W.SamplingRate = SF;
% W.TriggerMode = 'Master';
% 
% % Set Bpod serial message library with correct codes to trigger sounds 1-4 on analog output channels 1-2
% analogPortIndex = find(strcmp(BpodSystem.Modules.Name, 'WavePlayer1'));
% if isempty(analogPortIndex)
%     error('Error: Bpod WavePlayer module not found. If you just plugged it in, please restart Bpod.')
% end
% 
% PlayCh1Light_L = ['P' 1 2]; % Play, Channel in Bits, Sound(0,1,2)
% PlayCh2Light_R = ['P' 2 2];
% 
% %LoadSerialMessages('AudioPlayer1', {['P' 0], ['P' 1], ['P' 2], ['P' 3],['X']});
% LoadSerialMessages('WavePlayer1',{PlayCh1Light_L,PlayCh2Light_R});


R.wrapMode = 'Unipolar';
R.wrapPoint = 2160/2; %6 rotations
R.sendThresholdEvents = 'on';
Dist2Deg = (360/(pi*2.54*4))/2; % scale factor to degrees for 4" radius wheels, divides by 2 for 512 tic encoder
R.thresholds = ceil(Dist2Deg*[15 30 45 60 110 115 170 175]); % Thresholds converted from cm to degrees


%--- Define parameters and trial structure
S = BpodSystem.ProtocolSettings; % Loads settings file chosen in launch manager into current workspace as a struct called 'S'
if isempty(fieldnames(S))  % If chosen settings file was an empty struct, populate struct with default settings
    % Define default settings here as fields of S (i.e S.InitialDelay = 3.2)
    % Note: Any parameters in S.GUI will be shown in UI edit boxes. 
    % See ParameterGUI plugin documentation to show parameters as other UI types (listboxes, checkboxes, buttons, text)
    %S.GUI.WaterValveAmt = 10; %s
    S.GUI.WaterValveAmt = 4; %  micro liters
    S.GUI.BlockMin =12;
    S.GUI.BlockMax =18;
    S.GUI.lickWindow     = 20;  %s
    S.GUI.OperantProbability = 0; 
end


%--- Initialize plots and start USB connections to any modules
 BpodParameterGUI('init', S); % Initialize parameter GUI plugin
 TotalRewardDisplay('init');
 RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [1]); % valve open duration from calibration curve, 2-10 ul range
 LocationTime = (S.GUI.lickWindow);
 BpodNotebook('init');

%**** define events (Locations)
    for i=1:8
       locations{1,i} = strcat('RotaryEncoder1_', num2str(i));
    end
    soundouts     = {1,2};             %output ports for sounds 1 and 2

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
R.startUSBStream('useTimer'); % Start streaming position data from the rotary encoder   

% %% Wait for belt reset
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
% SendStateMachine(sma);
% RawEvents = RunStateMachine;

Trial_No = 1;


%%% -- block logic: same location for a randomized length of trials
for block = 1:MaxBlocks
    if mod(block,2)==1    
        % Location1
        Location2Probability = 0;
        WireState = bin2dec('001'); % all wire outs low
        locationStart = locations(5); % first 4 rotary_encoder positions are reserved for 
        locationEnd = locations(6);
%         soundid = soundouts{1};    % BNC2 High to generate tone    
    else
        % Location2
        Location2Probability = 1; 
        WireState = bin2dec('011'); % flip Wire2Out High to show wavesurfer that reward is in location2
        locationStart = locations(7);
        locationEnd = locations(8);
%         soundid = soundouts{2};  % BNC1 High to generate tone
    end
    
    blockLength=randi([S.GUI.BlockMin,S.GUI.BlockMax],1,1); % modified SV 10/16/19
    %% Main loop (runs once per trial)
    for currentTrial = 1:blockLength
        disp('__________')
        disp(['Trial#: ' num2str(Trial_No)])
        disp(['Block Trial#: ' num2str(currentTrial)])
        disp(['Block #: ' num2str(block)])
        BpodSystem.PluginObjects.currentTrial = currentTrial;
        S = BpodParameterGUI('sync', S); % Sync parameters with BpodParameterGUI plugin

        %% Update the GUI data
        RewardTime = GetValveTimes(S.GUI.WaterValveAmt, [1]); % valve open duration from calibration curve, 2-10 ul range
        LocationTime = (S.GUI.lickWindow);
        OperantProbability = (S.GUI.OperantProbability);

         % Generate a random # to determine trial type
        disp(strcat('Location 2 Probability: ', num2str(Location2Probability)))

        soundHack = randi(4); % randomly generate an integer from 0 to 4 to play the sound in one of the first four positions
        timeHack = rand*0.375;             % time delay to increase randomness of soundout, 15cm/(40cm/s)=0.375seconds
        soundLocation = locations(soundHack); % choose the sound location based on the random integer soundHack
        disp(['Location goal: ' num2str(Location2Probability+1)])

        % -- uncomment these lines to show the sound location and operant information
        %disp(['Sound Location: ' num2str(soundHack)])
        %disp(['Time Delay:' num2str(timeHack)])

        operantRnd = rand;

        % OPERANT TRAINING
        if operantRnd <= OperantProbability

            disp(['Operant?: Yes']) 
            %--- Assemble state machine
            sma = NewStateMachine();

            % play sound if 2 site operant training

            sma = AddState(sma, 'Name', 'StartLap',... % waits for start position to be reached
                    'Timer', 0,...
                    'StateChangeConditions', {'Port3Out', 'ResetPosition'},...
                    'OutputActions',{'WireState', WireState}); % send BNC reset to wavesurfer and teensy

            sma = AddState(sma, 'Name', 'ResetPosition',... % resets location of track
                'Timer', 0.1,...
                'StateChangeConditions', {'Tup', 'SendTimestamp'},...
                'OutputActions', {'RotaryEncoder1', 2, 'BNC1',1, 'WireState', WireState}); 

            sma = AddState(sma, 'Name', 'SendTimestamp', ... % play one of two tones to cue location of reward
                'Timer', 0,...
                'StateChangeConditions', {soundLocation, 'SoundDelay'},... % Delay for a random amount of time
                'OutputActions', {'RotaryEncoder1', 1, 'WireState', WireState}); % WireState gives location of the reward, either 2 or 0 (flip Wire2Out)

            sma = AddState(sma, 'Name', 'SoundDelay', ... % delay for 0 to 0.375 seconds after reaching location
                'Timer', timeHack,...
                'StateChangeConditions', {'Tup', 'GoToLocation'},...
                'OutputActions', {'WireState', WireState});

%             sma = AddState(sma, 'Name', 'PlaySound', ... % play one of two tones to cue location of reward
%                 'Timer', 0.5,...
%                 'StateChangeConditions', {'Tup', 'GoToLocation'},...
%                 'OutputActions', {'AudioPlayer1', soundid, 'WireState', WireState});

            sma = AddState(sma, 'Name', 'GoToLocation', ... % wait until location A or B is reached
                'Timer', 0,...
                'StateChangeConditions', {locationStart, 'WaitForLick'},...
                'OutputActions', {'RotaryEncoder1', 'E', 'WireState', WireState});


            sma = AddState(sma, 'Name', 'WaitForLick', ... % lick within window or forfit reward
                'Timer', LocationTime,...
                'StateChangeConditions', {'Port1In', 'Reward', locationEnd, 'TryAgain'},...
                'OutputActions', {'PWM3', 255, 'WireState', WireState});            % light flashes to indicates window start

            sma = AddState(sma, 'Name', 'Reward', ... % if lick happens in window, give reward 
                'Timer', RewardTime,...
                'StateChangeConditions', {'Tup', 'BPOD_Lap_over'},...
                'OutputActions', {'ValveState', 2^-1, 'WireState', WireState+4}); % WireState gives location of reward and reward release, add 4 to make Wire3Out High (4=bin2dec('100'))

            sma = AddState(sma, 'Name', 'TryAgain' , ...% if no lick before time is up, exit
                'Timer', RewardTime,...
                'StateChangeConditions', {'Tup', 'BPOD_Lap_over'},...
               'OutputActions', {'PWM3', 255, 'WireState', WireState});    % light flashes to indicates window end

            sma = AddState(sma, 'Name', 'BPOD_Lap_over',... % resets location of track
            'Timer', 0.001,...
            'StateChangeConditions', {'Tup', 'exit'},...
            'OutputActions', {'WireState', WireState}); 

        % NON OPERANT TRAINING
        else
            disp(['Operant?: No'])
            sma = NewStateMachine();  

            sma = AddState(sma, 'Name', 'StartLap',... % waits for start position to be reached
                    'Timer', 0,...
                    'StateChangeConditions', {'Port3Out', 'ResetPosition'},...
                    'OutputActions', {'WireState', WireState}); 

             sma = AddState(sma, 'Name', 'ResetPosition',... % resets location of track
                'Timer', 0.1,...
                'StateChangeConditions', {'Tup', 'SendTimestamp'},...
                'OutputActions', {'RotaryEncoder1', 2, 'BNC1',1, 'WireState', WireState});               


            sma = AddState(sma, 'Name', 'SendTimestamp', ... % play one of two tones to cue location of reward
                'Timer', 0,...
                'StateChangeConditions', {'RotaryEncoder1_1', 'GoToLocation'},...  %skip playing of sound
                'OutputActions', {'RotaryEncoder1', 1, 'WireState', WireState});

%             sma = AddState(sma, 'Name', 'PlaySound', ... % play one of two tones to cue location of reward
%                 'Timer', 0.01,...
%                 'StateChangeConditions', {'Tup', 'Lights'},...
%                 'OutputActions', {'WavePlayer1', (soundid + 2), 'WireState', WireState});
% 
%             sma = AddState(sma, 'Name', 'Lights', ... % play one of two tones to cue location of reward
%                 'Timer', 0.01,...
%                 'StateChangeConditions', {'Tup', 'GoToLocation'},...
%                 'OutputActions', {'WavePlayer1', (soundid), 'WireState', WireState});

            sma = AddState(sma, 'Name', 'GoToLocation', ... % wait unti location A or B is reached
                'Timer', 0,...
                'StateChangeConditions', {locationStart, 'Reward'},...
                'OutputActions', {'WireState', WireState});

            sma = AddState(sma, 'Name', 'Reward', ... % if lick happens in window, give reward 
                'Timer', RewardTime,...
                'StateChangeConditions', {'Tup', 'BPOD_Lap_over'},...
                'OutputActions', {'ValveState', 2^1, 'WireState', WireState+4});   % turns output 3 to high, indicating release of water

            sma = AddState(sma, 'Name', 'BPOD_Lap_over',... % resets location of track
                'Timer', 0.1,...
                'StateChangeConditions', {'Tup', 'exit'},...
                'OutputActions', {'WireState', WireState}); 

        end
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
        if ~isnan(BpodSystem.Data.RawEvents.Trial{currentTrial}.States.Reward(1))
            TotalRewardDisplay('add', S.GUI.WaterValveAmt);
        end

        BpodSystem.Data = BpodNotebook('sync', BpodSystem.Data);  %synchronizes notebook data at the end of each trial

        %--- This final block of code is necessary for the Bpod console's pause and stop buttons to work
        HandlePauseCondition; % Checks to see if the protocol is paused. If so, waits until user resumes.
        if BpodSystem.Status.BeingUsed == 0
            return
        end
        Trial_No = Trial_No + 1;
    end
end
disp('All finished! Give the mouse a treat and a pat on the head ;-)')
R.stopUSBStream;
