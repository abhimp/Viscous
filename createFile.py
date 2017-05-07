import sys
import random
import string

#size = sys.argv[1]
packetSize = 1290
if len(sys.argv) >= 4:
    packetSize = int(sys.argv[3])

size = 0
strSize = sys.argv[1]
filePath = sys.argv[2]

if strSize[-1] == "G" or strSize[-1] == "g":
    size = int(float(strSize[:-1])*(2**30))
elif strSize[-1] == "M" or strSize[-1] == "m":
    size = int(float(strSize[:-1])*(2**20))
elif strSize[-1] == "K" or strSize[-1] == "k":
    size = int(float(strSize[:-1])*(2**10))
elif strSize[-1] == "B" or strSize[-1] == "b":
    size = int(float(strSize[:-1]))
else:
    size = int(float(strSize))


print size

numLines = (size+packetSize-1)/packetSize

print numLines

#''.join(random.choice(string.letters + string.digits+string.punctuation) for _ in xrange(222))

lenforLineNum = len("%s"%numLines)

minPaketLen = lenforLineNum + 3

if packetSize < minPaketLen:
    exit(1)

lastLineLen = size%packetSize
fp = open(filePath, "w")

linetext = ''.join(random.choice(string.letters + string.digits+string.punctuation + " ") for _ in xrange(packetSize*2))

for x in xrange(numLines-1):
    lineNum = "%s"%(x)
    zeroPadding = "0"*(lenforLineNum - len(lineNum))
    lineNum = zeroPadding + lineNum + ": "
    linetextLen = packetSize - minPaketLen
    startIndex = random.randrange(packetSize)
    line = lineNum+linetext[startIndex : startIndex + linetextLen]+"\n"
    if len(line) != packetSize:
        exit(2)
    fp.write(line)

if lastLineLen == 0:
    lastLineLen = packetSize

lineNum = "%s"%(numLines-1)
zeroPadding = "0"*(lenforLineNum - len(lineNum))
lineNum = zeroPadding + lineNum + ": "
linetextLen = packetSize - minPaketLen
startIndex = random.randrange(packetSize)
line = lineNum+linetext[startIndex: startIndex+linetextLen] + "\n"


line = line[:lastLineLen]

fp.write(line)
fp.close()
