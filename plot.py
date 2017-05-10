#!/usr/bin/python
 # * This is an implemetation of Viscous protocol.
 # * Copyright (C) 2017  Abhijit Mondal
 # *
 # * This program is free software: you can redistribute it and/or modify
 # * it under the terms of the GNU General Public License as published by
 # * the Free Software Foundation, either version 3 of the License, or
 # * (at your option) any later version.
 # *
 # * This program is distributed in the hope that it will be useful,
 # * but WITHOUT ANY WARRANTY; without even the implied warranty of
 # * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # * GNU General Public License for more details.
 # *
 # * You should have received a copy of the GNU General Public License
 # * along with this program.  If not, see <http://www.gnu.org/licenses/>.

 
import matplotlib.pyplot as plt
import sys
fyl = sys.argv[1]
f = open(fyl)
trace = "tracedata:"
ackTrace = "newackrecved trace:"
timeoutTrace = "Retransmission count:"

outgoingTrace = "SenderInterfacePacketCount:"

testFlow1 = "testflowtrace1:"
testFlow2 = "testflowtrace2:"

traceData = []

sentData = []
retransData = []

outgoingTraceData = []

testFlowData1 = []
testFlowData2 = []

for tmpX in f:
    if tmpX.find(trace) >= 0:
        traceData += [tmpX[tmpX.find(trace) + len(trace):].strip().split(",")]

    if tmpX.find(ackTrace) >= 0:
        sentData += [tmpX[tmpX.find(ackTrace) + len(ackTrace):].strip().split(",")]

    if tmpX.find(timeoutTrace) >= 0:
        retransData += [tmpX[tmpX.find(timeoutTrace) + len(timeoutTrace):].strip().split(",")]
    
    if tmpX.find(testFlow1) >= 0:
        testFlowData1 += [tmpX[tmpX.find(testFlow1) + len(testFlow1):].strip().split(",")]

    if tmpX.find(testFlow2) >= 0:
        testFlowData2 += [tmpX[tmpX.find(testFlow2) + len(testFlow2):].strip().split(",")]

    if tmpX.find(outgoingTrace) >= 0:
        outgoingTraceData += [tmpX[tmpX.find(outgoingTrace) + len(outgoingTrace):].strip().split(",")]

traceDataComp = [[float(y) for y in x] for x in sentData]
sentDataComp = [[float(y) for y in x] for x in traceData]
retransDataComp = [[float(y) for y in x] for x in retransData]
outgoingTraceDataComp = [[float(y) for y in x] for x in outgoingTraceData]
testFlowStartEndDataComp = [[float(y) for y in x] for x in testFlowData1]
testFlowSendDataComp = [[float(y) for y in x] for x in testFlowData2]


timeZero = traceDataComp[0][0]

if len(sentDataComp) > 0:
    timeZero = min(timeZero, sentDataComp[0][0])
if len(retransDataComp) > 0:
    timeZero = min(timeZero, retransDataComp[0][0])
if len(outgoingTraceDataComp):
    timeZero = min(timeZero, outgoingTraceDataComp[0][0])

if len(testFlowSendDataComp) > 0:
    timeZero = min(timeZero, testFlowSendDataComp[0][1])
if len(testFlowStartEndDataComp) > 0:
    timeZero = min(timeZero, testFlowStartEndDataComp[0][1])
    
channels = []

traceTime = {}
cwnd = {}
srtt = {}
ssthresh = {}
rttvar = {}
rto = {}
for t in sentDataComp:
    chid = t[5]
    if chid not in channels:
        channels.append(chid)
    p = traceTime.setdefault(chid, [])
    p.append((t[0]-timeZero)/1000000.0)
    p = cwnd.setdefault(chid, [])
    p.append(t[1])
    p = srtt.setdefault(chid, [])
    p.append(t[2])
    p = ssthresh.setdefault(chid, [])
    p.append(t[3])
    p = rttvar.setdefault(chid, [])
    p.append(t[6])
    p = rto.setdefault(chid, [])
    p.append(t[7])


packetSentTime = {}
packetSentCount = {}
byteSentCount = {}
packetSentTimeChannels = []

for t in outgoingTraceDataComp:
    chid = ""
    if chid not in packetSentTimeChannels:
        packetSentTimeChannels.append(chid)
    p = packetSentTime.setdefault(chid, [])
    p.append((t[0] - timeZero)/1000000)
    p = packetSentCount.setdefault(chid, [])
    p.append(t[1])
    p = byteSentCount.setdefault(chid, [])
    p.append(t[1])

    

sentTime = {}
totalAck = {}
totalSent = {}

for t in traceDataComp:
    chid = t[1]
    p = sentTime.setdefault(chid, [])
    p.append((t[0]-timeZero)/1000000.0)
    p = totalAck.setdefault(chid, [])
    p.append(t[2])
    p = totalSent.setdefault(chid, [])
    p.append(t[3])

retrancTime = {}
totalTimeouts = {}
totalDuplicateAck = {} 
totalRetransmitions = {}

for t in retransDataComp:
    chid = t[1]
    p = retrancTime.setdefault(chid, [])
    p.append((t[0]-timeZero)/1000000.0)
    p = totalDuplicateAck.setdefault(chid, [])
    p.append(t[2])
    p = totalTimeouts.setdefault(chid, [])
    p.append(t[3])
    p = totalRetransmitions.setdefault(chid, [])
    p.append(t[4])

flows = []
testStEnTime = {}
testStEnValu = {}
testSentTime = {}
testSentValu = {}

for t in testFlowStartEndDataComp:
    fl = t[0]
    if fl not in flows:
        flows.append(fl)
    p = testStEnTime.setdefault(fl, [])
    p.append((t[1] - timeZero)/1000000.0)

    p = testStEnValu.setdefault(fl, [])
    p.append(fl)

for t in testFlowSendDataComp:
    fl = t[0]
    if fl not in flows:
        flows.append(fl)
    p = testSentTime.setdefault(fl, [])
    p.append((t[1] - timeZero)/1000000.0)

    p = testSentValu.setdefault(fl, [])
    p.append(fl)
    

#traceTime = [(t[0]-timeZero)/1000000.0 for t in sentDataComp]
#cwnd = [t[1] for t in sentDataComp]
#srtt = [t[2] for t in sentDataComp]
#l = [t[:2] for t in sentDataComp]

def plot(channels, xlabel, ylabel, title, datas, legends = True):
    size = 10
    plt.xticks(size=size)
    plt.yticks(size=size)
    plt.xlabel(xlabel, size=size)
    plt.ylabel(ylabel, size=size)
    plt.title(title, size=size+10)
    plt.grid(True)
    #plt.xlim([0,0.25])
    
    #plt.plot(traceTime, cwnd, linewidth=2, marker="+", mec="red", ms=16, mew=4)
    for dt in datas:
        for ch in channels:
            if ch in dt[0] and ch in dt[1]:
                plt.plot(dt[0][ch], dt[1][ch], linewidth=2, marker=dt[2], ms=16, mew=4, label=str(ch) + " " + dt[3])
#     for ch in channels:
#         plt.plot(sentTime[ch], totalSent[ch], linewidth=2, marker="x", ms=16, mew=4, label=str(ch+1) + " total totalSent")
    
    if legends:
        plt.legend()
    plt.show()
    #plt.savefig("/tmp/cwnd.png")
    plt.clf()
    
#==============

plot(flows, "Time $s$", "FlowId", "chart",
     [[testSentTime, testSentValu, "+", "sent"], [testStEnTime, testStEnValu, ".", "span"]], False)
if len(outgoingTraceDataComp)> 0:
    plot(packetSentTimeChannels, "Time ($s$)", "Packet count", "Plottinf of packet sent over time",
            [[packetSentTime, packetSentCount, "+", "packets"]], False)
if len(retransDataComp) > 0:
    plot(channels, "Time ($s$)", "Packet Count ($packet$)", "Ploting of retranstion timeout count vs Time where packet size is 1300byte", 
     [[retrancTime, totalTimeouts, "+", "timeout count"], [retrancTime, totalDuplicateAck, "x", "duplicate"]])

plot(channels, "sentTime ($s$)", "Packet Count ($packet$)", "Ploting of Packet count vs sentTime where packet size is 1300byte", 
     [[sentTime, totalAck, "+", "total acked"], [sentTime, totalSent, "x", "total sent"]])

plot(channels, "sentTime ($s$)", "window size ($packet$)", "Ploting of window size vs sentTime where packet size is 1300byte",
     [[traceTime, cwnd, "+", "cwnd"], [traceTime, ssthresh, "x", "ssthresh"]])

plot(channels, "sentTime ($s$)", "srtt ($\mu s$)", "Ploting of srtt vs sentTime where packet size is 1300byte",
     [[traceTime, srtt, "+", ""]])
