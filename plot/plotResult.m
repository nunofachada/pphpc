function plotResult(file, sheepCol, sheepScale, wolfCol, wolfScale, grassCol, grassScale)

data = dlmread(file);
figure;
hold on;
grid on;
plot(data(:, sheepCol) * sheepScale, 'b');
plot(data(:, wolfCol) * wolfScale, 'r');
plot(data(:, grassCol) * grassScale, 'g');
legend('Sheep', 'Wolf', 'Grass');