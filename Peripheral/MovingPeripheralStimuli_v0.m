%%% Script used to test peripheral visual stimuli
%%% Currently the design is to display n white rectangles that move
%%% proportionally to the animals position
%%% Then, a reward stimuli is overlaid for a period of time.
%%% Relies on psychtoolbox PsychRects to run.
%%% Sometimes does not find screen. In that case, run MovingSquareDemo.m
%%% seems to recallibrate everything.

%%Clear comm port

if ~isempty(instrfind)
fclose(instrfind);
delete(instrfind);
end

% Clear the workspace and the screen
sca;
close all;
clearvars;

SerialPort='COM5';   %serial port
TimeInterval=0.01;  %time interval (s) between each input.

%%Set up the serial port object
s = serial(SerialPort,'BaudRate',9600);
fopen(s);

% Here we call some default settings for setting up Psychtoolbox
PsychDefaultSetup(2);

% Get the screen numbers
screens = Screen('Screens');

% Draw to the external screen if avaliable
screenNumber = max(screens);

% Define black and white
white = WhiteIndex(screenNumber);
black = BlackIndex(screenNumber);

% Open an on screen window
[window, windowRect] = PsychImaging('OpenWindow', screenNumber, black);
windowWidth = windowRect(3);
windowHeight = windowRect(4);

% Get the size of the on screen window
[screenXpixels, screenYpixels] = Screen('WindowSize', window);

% Query the frame duration
ifi = Screen('GetFlipInterval', window);

% Get the centre coordinate of the window
[xCenter, yCenter] = RectCenter(windowRect);

% Set the color of the rect to white
rectColor = [1 1 1];
time = 0;

% Sync us and get a time stamp
vbl = Screen('Flip', window);
waitframes = 1;

% Maximum priority level
topPriorityLevel = MaxPriority(window);
Priority(topPriorityLevel);

flushinput(s);
% Loop the animation until a key is pressed
t0 = tic;

% lab parameters

lapDistance = 1800; %cm
numRects = 1;
fillScreenPercent = 10; % percent of width taken up by white space
heightPercent = 20;
rectVelo = 2;


% Make a base Rect given the parameters

% scale the width as a pecent of total screen width
rectWidth = (windowWidth/numRects)*fillScreenPercent/100;
rectHeight = windowHeight*heightPercent/100;
spaceWidth = (windowWidth/numRects)*(1-fillScreenPercent/100);

% reward visual stimuli

rewardLocation = 1600;
rewardColor = [0 0 1]; 
rewardScreenWidthPercent = 10;
rewardHeightPercent = 70;
rewardWidth =  (windowWidth/numRects)*rewardScreenWidthPercent/100;
rewardHeight = windowHeight*rewardHeightPercent/100;
baseRewardRect = [0, 0, rewardWidth, rewardHeight];

% baseRect = NaN(numRects,4);
% for ii = 1:numRects
%     baseRect(ii,:) = [(ii-1)*spaceWidth, 0, rectWidth, windowHeight*heightPercent/100];
% end
baseRect = [0, 0, rectWidth,  rectHeight];

squareXpos = NaN(numRects,1);
yCenterMat = repmat(yCenter, numRects, 1);

while ~KbCheck
    currentPosition = fscanf(s,'%g',4);
    % Position of the square on this frame
    xpos = windowWidth - currentPosition/lapDistance * windowWidth;
    % Add this position to the screen center coordinate. This is the point
    % we want our square to oscillate around
    for ii = 1:numRects
        squareXpos(ii) = rectVelo*xpos + (ii-1)*spaceWidth;
    end
    
    % Center the rectangle on the centre of the screen
    centeredRect = CenterRectOnPointd(baseRect, squareXpos, yCenterMat)';
    
    % ensure wrap around
    centeredRect(1,:)  = mod(centeredRect(1,:), windowWidth);
    centeredRect(3,:) = centeredRect(1,:) + rectWidth;
    % Draw the rect to the screen
    Screen('FillRect', window, rectColor, centeredRect');
    
    % draw the reward stimuli once
    if currentPosition>= rewardLocation
        xposReward =  windowWidth - (currentPosition-rewardLocation/2)/lapDistance * windowWidth;
        fprintf("reward! %g", currentPosition)
        rewardXpos = rectVelo*xposReward;
        if rewardXpos >0 && rewardXpos < windowWidth
            centeredRectReward = CenterRectOnPointd(baseRewardRect', squareXpos, yCenterMat);
            Screen('FillRect', window, rewardColor, centeredRectReward);
        end
    end
    
    % Flip to the screen
    vbl  = Screen('Flip', window, vbl + (waitframes - 0.5) * ifi);

    % Increment the time
    time = time + ifi;
    t1 = tic;
    if (t1-t0)>TimeInterval
        t0 = t1;
        flushinput(s);
    end
end

% Clear the screen
sca;
%% Clean up the serial port
fclose(s);
delete(s);
clear s;

function eqPos = dist2pos(windowRect)
    %%% Given the re
end
 