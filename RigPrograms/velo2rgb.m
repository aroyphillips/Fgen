function rgb = velo2rgb(velo)
            %% Top speed 40cm/s gets color red, scales velo to wavelenuency then to color using formula from
            %% input velo is a 1xn vector of velocities, output is an nx3 matrix where each row is a color vector
            %% http://www.efg2.com/Lab/ScienceAndEngineering/Spectra.htm
            n = numel(velo);
            rgb = zeros(n, 3);
            for i = 1:n
                wavelen = velo(i) * 5.1 + 380;
                if (380<wavelen && wavelen<440)
                    rgb(i,:) = [-(wavelen-440)/(440-380), 0.0, 1.0]; 
                elseif(wavelen<490)
                    rgb(i,:) = [0.0,(wavelen-440)/(510-440), 1.0];
                elseif (wavelen<510)
                    rgb(i,:) = [0.0, 1.0, -(wavelen-510)/(510-490)];
                elseif (wavelen<580)
                    rgb(i,:) = [(wavelen-510)/(580-510), 1.0, 0.0];
                elseif (wavelen<645)
                    rgb(i,:) = [1.0, -(wavelen -645)/(645-580), 0.0];
                elseif (wavelen<780)
                    rgb(i,:) = [1.0, 0.0, 0.0];
                else 
                    rgb(i,:) = [0.0, 0.0, 0.0];
                end
            end
end