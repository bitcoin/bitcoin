[num,txt,raw] = xlsread('MEMPOOL_DATA.xlsx');
disp('Data has been read.');
timestamp = num(:, 1);
mempool_size = num(:, 2);
bytes = num(:, 3);
usage = num(:, 4);
max_mempool = num(:, 5);
mempool_min_fee = num(:, 6);
min_relay_tx_fee = num(:, 7);

x = timestamp;

figure

y = mempool_size;
subplot(2,1,1);
plot(x, y)
title('Mempool Number of Transactions Over Time');
xlabel('Timestamp (s)');
ylabel('Number of Transactions');

y2 = bytes;
subplot(2,1,2);
plot(x, y2)
title('Mempool Number of Bytes Over Time');
xlabel('Timestamp (s)');
ylabel('Size in Bytes');
