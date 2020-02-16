%% Setup the Import Options and import the data
opts = delimitedTextImportOptions("NumVariables", 274);

% Specify range and delimiter
opts.DataLines = [2, Inf];
opts.Delimiter = ",";

% Specify column names and types
opts.VariableNames = ["Timestamp", "TimestampSeconds", "ClocksPerSec", "VERSION", "VERSIONDiff", "VERSIONClocksSumDiff", "VERSIONClocksAvgDiff", "VERSIONClocksAvg", "VERSIONClocksMax", "VERSIONBytesSumDiff", "VERSIONBytesAvgDiff", "VERSIONBytesAvg", "VERSIONBytesMax", "VERACK", "VERACKDiff", "VERACKClocksSumDiff", "VERACKClocksAvgDiff", "VERACKClocksAvg", "VERACKClocksMax", "VERACKBytesSumDiff", "VERACKBytesAvgDiff", "VERACKBytesAvg", "VERACKBytesMax", "ADDR", "ADDRDiff", "ADDRClocksSumDiff", "ADDRClocksAvgDiff", "ADDRClocksAvg", "ADDRClocksMax", "ADDRBytesSumDiff", "ADDRBytesAvgDiff", "ADDRBytesAvg", "ADDRBytesMax", "INV", "INVDiff", "INVClocksSumDiff", "INVClocksAvgDiff", "INVClocksAvg", "INVClocksMax", "INVBytesSumDiff", "INVBytesAvgDiff", "INVBytesAvg", "INVBytesMax", "GETDATA", "GETDATADiff", "GETDATAClocksSumDiff", "GETDATAClocksAvgDiff", "GETDATAClocksAvg", "GETDATAClocksMax", "GETDATABytesSumDiff", "GETDATABytesAvgDiff", "GETDATABytesAvg", "GETDATABytesMax", "MERKLEBLOCK", "MERKLEBLOCKDiff", "MERKLEBLOCKClocksSumDiff", "MERKLEBLOCKClocksAvgDiff", "MERKLEBLOCKClocksAvg", "MERKLEBLOCKClocksMax", "MERKLEBLOCKBytesSumDiff", "MERKLEBLOCKBytesAvgDiff", "MERKLEBLOCKBytesAvg", "MERKLEBLOCKBytesMax", "GETBLOCKS", "GETBLOCKSDiff", "GETBLOCKSClocksSumDiff", "GETBLOCKSClocksAvgDiff", "GETBLOCKSClocksAvg", "GETBLOCKSClocksMax", "GETBLOCKSBytesSumDiff", "GETBLOCKSBytesAvgDiff", "GETBLOCKSBytesAvg", "GETBLOCKSBytesMax", "GETHEADERS", "GETHEADERSDiff", "GETHEADERSClocksSumDiff", "GETHEADERSClocksAvgDiff", "GETHEADERSClocksAvg", "GETHEADERSClocksMax", "GETHEADERSBytesSumDiff", "GETHEADERSBytesAvgDiff", "GETHEADERSBytesAvg", "GETHEADERSBytesMax", "TX", "TXDiff", "TXClocksSumDiff", "TXClocksAvgDiff", "TXClocksAvg", "TXClocksMax", "TXBytesSumDiff", "TXBytesAvgDiff", "TXBytesAvg", "TXBytesMax", "HEADERS", "HEADERSDiff", "HEADERSClocksSumDiff", "HEADERSClocksAvgDiff", "HEADERSClocksAvg", "HEADERSClocksMax", "HEADERSBytesSumDiff", "HEADERSBytesAvgDiff", "HEADERSBytesAvg", "HEADERSBytesMax", "BLOCK", "BLOCKDiff", "BLOCKClocksSumDiff", "BLOCKClocksAvgDiff", "BLOCKClocksAvg", "BLOCKClocksMax", "BLOCKBytesSumDiff", "BLOCKBytesAvgDiff", "BLOCKBytesAvg", "BLOCKBytesMax", "GETADDR", "GETADDRDiff", "GETADDRClocksSumDiff", "GETADDRClocksAvgDiff", "GETADDRClocksAvg", "GETADDRClocksMax", "GETADDRBytesSumDiff", "GETADDRBytesAvgDiff", "GETADDRBytesAvg", "GETADDRBytesMax", "MEMPOOL", "MEMPOOLDiff", "MEMPOOLClocksSumDiff", "MEMPOOLClocksAvgDiff", "MEMPOOLClocksAvg", "MEMPOOLClocksMax", "MEMPOOLBytesSumDiff", "MEMPOOLBytesAvgDiff", "MEMPOOLBytesAvg", "MEMPOOLBytesMax", "PING", "PINGDiff", "PINGClocksSumDiff", "PINGClocksAvgDiff", "PINGClocksAvg", "PINGClocksMax", "PINGBytesSumDiff", "PINGBytesAvgDiff", "PINGBytesAvg", "PINGBytesMax", "PONG", "PONGDiff", "PONGClocksSumDiff", "PONGClocksAvgDiff", "PONGClocksAvg", "PONGClocksMax", "PONGBytesSumDiff", "PONGBytesAvgDiff", "PONGBytesAvg", "PONGBytesMax", "NOTFOUND", "NOTFOUNDDiff", "NOTFOUNDClocksSumDiff", "NOTFOUNDClocksAvgDiff", "NOTFOUNDClocksAvg", "NOTFOUNDClocksMax", "NOTFOUNDBytesSumDiff", "NOTFOUNDBytesAvgDiff", "NOTFOUNDBytesAvg", "NOTFOUNDBytesMax", "FILTERLOAD", "FILTERLOADDiff", "FILTERLOADClocksSumDiff", "FILTERLOADClocksAvgDiff", "FILTERLOADClocksAvg", "FILTERLOADClocksMax", "FILTERLOADBytesSumDiff", "FILTERLOADBytesAvgDiff", "FILTERLOADBytesAvg", "FILTERLOADBytesMax", "FILTERADD", "FILTERADDDiff", "FILTERADDClocksSumDiff", "FILTERADDClocksAvgDiff", "FILTERADDClocksAvg", "FILTERADDClocksMax", "FILTERADDBytesSumDiff", "FILTERADDBytesAvgDiff", "FILTERADDBytesAvg", "FILTERADDBytesMax", "FILTERCLEAR", "FILTERCLEARDiff", "FILTERCLEARClocksSumDiff", "FILTERCLEARClocksAvgDiff", "FILTERCLEARClocksAvg", "FILTERCLEARClocksMax", "FILTERCLEARBytesSumDiff", "FILTERCLEARBytesAvgDiff", "FILTERCLEARBytesAvg", "FILTERCLEARBytesMax", "SENDHEADERS", "SENDHEADERSDiff", "SENDHEADERSClocksSumDiff", "SENDHEADERSClocksAvgDiff", "SENDHEADERSClocksAvg", "SENDHEADERSClocksMax", "SENDHEADERSBytesSumDiff", "SENDHEADERSBytesAvgDiff", "SENDHEADERSBytesAvg", "SENDHEADERSBytesMax", "FEEFILTER", "FEEFILTERDiff", "FEEFILTERClocksSumDiff", "FEEFILTERClocksAvgDiff", "FEEFILTERClocksAvg", "FEEFILTERClocksMax", "FEEFILTERBytesSumDiff", "FEEFILTERBytesAvgDiff", "FEEFILTERBytesAvg", "FEEFILTERBytesMax", "SENDCMPCT", "SENDCMPCTDiff", "SENDCMPCTClocksSumDiff", "SENDCMPCTClocksAvgDiff", "SENDCMPCTClocksAvg", "SENDCMPCTClocksMax", "SENDCMPCTBytesSumDiff", "SENDCMPCTBytesAvgDiff", "SENDCMPCTBytesAvg", "SENDCMPCTBytesMax", "CMPCTBLOCK", "CMPCTBLOCKDiff", "CMPCTBLOCKClocksSumDiff", "CMPCTBLOCKClocksAvgDiff", "CMPCTBLOCKClocksAvg", "CMPCTBLOCKClocksMax", "CMPCTBLOCKBytesSumDiff", "CMPCTBLOCKBytesAvgDiff", "CMPCTBLOCKBytesAvg", "CMPCTBLOCKBytesMax", "GETBLOCKTXN", "GETBLOCKTXNDiff", "GETBLOCKTXNClocksSumDiff", "GETBLOCKTXNClocksAvgDiff", "GETBLOCKTXNClocksAvg", "GETBLOCKTXNClocksMax", "GETBLOCKTXNBytesSumDiff", "GETBLOCKTXNBytesAvgDiff", "GETBLOCKTXNBytesAvg", "GETBLOCKTXNBytesMax", "BLOCKTXN", "BLOCKTXNDiff", "BLOCKTXNClocksSumDiff", "BLOCKTXNClocksAvgDiff", "BLOCKTXNClocksAvg", "BLOCKTXNClocksMax", "BLOCKTXNBytesSumDiff", "BLOCKTXNBytesAvgDiff", "BLOCKTXNBytesAvg", "BLOCKTXNBytesMax", "REJECT", "REJECTDiff", "REJECTClocksSumDiff", "REJECTClocksAvgDiff", "REJECTClocksAvg", "REJECTClocksMax", "REJECTBytesSumDiff", "REJECTBytesAvgDiff", "REJECTBytesAvg", "REJECTBytesMax", "UNDOCUMENTED", "UNDOCUMENTEDDiff", "UNDOCUMENTEDClocksSumDiff", "UNDOCUMENTEDClocksAvgDiff", "UNDOCUMENTEDClocksAvg", "UNDOCUMENTEDClocksMax", "UNDOCUMENTEDBytesSumDiff", "UNDOCUMENTEDBytesAvgDiff", "UNDOCUMENTEDBytesAvg", "UNDOCUMENTEDBytesMax", "VarName274"];
opts.VariableTypes = ["datetime", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "double", "string"];

% Specify file level properties
opts.ExtraColumnsRule = "ignore";
opts.EmptyLineRule = "read";

% Specify variable properties
opts = setvaropts(opts, "VarName274", "WhitespaceRule", "preserve");
opts = setvaropts(opts, "VarName274", "EmptyFieldRule", "auto");
opts = setvaropts(opts, "Timestamp", "InputFormat", "yyyy-MM-dd HH:mm:ss.SSS");

% Import the data
data = readtable("~/Desktop/GetMsgInfoLog.csv", opts);


%% Clear temporary variables
clear opts

%% Begin plotting
[numRows, numCols] = size(data);

titles = ["Number of messages (total)", "Number of messages (second)", "Total clocks (second)", "Average clocks (second)", "Average clocks (total)", "Max clocks (total)", "Total bytes (second)", "Average bytes (second)", "Average bytes (total)", "Max bytes (total)"]

x = table2array(data(:, 2));
for j = 1:10
    subplot(2,5,j)
    y1 = double(table2array(data(:,((3 + j):10:end))));
    hold on
    for i = 1:length(y1(1, :))
        plot(x, y1(:, i), 'LineWidth', 2);
    end
    grid on
    xlabel("Timestamp");
    title(titles(j));
    % Make the axis show the full number
    %set(gca, 'XTickLabel', num2cell(get(gca, 'XTick')));
    set(gca, 'YTickLabel', num2cell(get(gca, 'YTick')));
    
    hold off
end

legend("VERSION", "VERACK", "ADDR", "INV", "GETDATA", "MERKLEBLOCK", "GETBLOCKS", "GETHEADERS", "TX", "HEADERS", "BLOCK", "GETADDR", "MEMPOOL", "PING", "PONG", "NOTFOUND", "FILTERLOAD", "FILTERADD", "FILTERCLEAR", "SENDHEADERS", "FEEFILTER", "SENDCMPCT", "CMPCTBLOCK", "GETBLOCKTXN", "BLOCKTXN", "REJECT", "[UNDOCUMENTED]");

