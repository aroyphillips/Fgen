%%Wavesurfer Acquisition Graphing

function plt = recreatePlot(filepath)
% filepath = '../TrainingFish/untitled_0001.h5';

%clear; close all;
names = split(filepath, '\');

% access data from file
data = ws.loadDataFile(filepath);
sweeps = fieldnames(data);
sweep = getfield(data, char(sweeps(2)));
Ain = sweep.analogScans;
Din = sweep.digitalScans;
rewardRelease = Ain(:,4); % I think it should be Din(:,2) but nonexistant
dist = Ain(:,2);
velo = Ain(:,1);
licks = Ain(:,3); 
bpodTarget = Ain(:,7);

%find lap starts (reset of teensy distance)
lapStarts = find(diff(dist)<-1);
laps= length(lapStarts) ;

%Bins for Velocity
Nbins = 50;
trackLength  = 180;
binWidth = trackLength/Nbins/100;
bins = conv([0:binWidth*100:trackLength]', [1 1])/2;

%convert voltage to velocity
veloCM = (velo-1.25)*80;

figure(1);



%% Plot Licks, Velo, Release bar, taraget location
for n = 1:length(lapStarts)-1
    
    %find data for current lap
    lapDist = dist(lapStarts(n):lapStarts(n+1));
    lapVelo = veloCM(lapStarts(n):lapStarts(n+1));
    lapLicks= licks(lapStarts(n):lapStarts(n+1));
    lapTarget = mean(bpodTarget(lapStarts(n):lapStarts(n+1)));
    lapRewardRelease = rewardRelease(lapStarts(n):lapStarts(n+1));
    rewardIdx = find(diff(lapRewardRelease)>1);

    if lapTarget >1
        targ = [1.72, 1.80];
    else
        targ = [1.12, 1.20];
    end
    
    inTargInds = lapDist>targ(1) & lapDist<targ(2);
    targDist = lapDist(inTargInds);
    notTargDist = lapDist(~inTargInds);


    correctLicks = lapLicks(inTargInds);
    wrongLicks = lapLicks(~inTargInds);
    correctStartLicks = find(diff(correctLicks)>1);
    wrongStartLicks = find(diff(wrongLicks)>1);

    %find binned velocities
    bindices = ceil(lapDist./binWidth);
    inLapBindices = bindices>0 & bindices<51;
    binVelo = splitapply(@mean, lapVelo(inLapBindices), bindices(inLapBindices));
    veloRGB = velo2rgb(binVelo);

    figure(1)

    %plot licks
    plot(targDist(correctStartLicks')'*100, repmat(n,size(correctStartLicks')), '^','MarkerSize',5,'MarkerEdgeColor','green','MarkerFaceColor',[.2 .8 .8]);
    hold on
    plot(notTargDist(wrongStartLicks')'*100, repmat(n,size(wrongStartLicks')), '^','MarkerSize',5,'MarkerEdgeColor','red','MarkerFaceColor',[1 .2 .2]);
    hold on

    %plot velocity
    scatter(bins(2:end-1), (n-0.4).*ones(1,50),13, veloRGB, '+')
    hold on
    %plot reward zone and reward release
    plot(targ*100, [n-.05, n-.05], 'Color', 'Blue')
    hold on
    if ~isempty(rewardIdx)            
        plot([lapDist(rewardIdx(1)), lapDist(rewardIdx(1))]*100, [n+.4, n-.4],'Color', 'Black')
    end
end

%% label graph
if nargin<2
    is_label=false;
end
xlim([0 185])
ylim([0.5 laps-.5])
xticks([0:20:180]);
xticklabels(string([0:20:180]));
xtickangle(45)
xlabel('position on treadmill(cm)')
ylabel('lap #')
yticks([1:1:laps]);
yticklabels(string([1:1:laps]));
ax = gca;
ax.YDir = 'reverse';

if is_label
    expType = names{4};
    mouseDate = names{6};
    title(strcat(expType(1:end-4), ' ' , mouseDate(1:end-3)));
end

%% resize

hFig = figure(1);
set(hFig, 'Units', 'Normalized', 'OuterPosition', [0,0,.3,1]);
%pbaspect([0.573569482288828,1,0.573569482288828]);
end