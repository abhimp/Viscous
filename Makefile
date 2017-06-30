CC := gcc
CPP := g++

CFLAGS := \
-std=c11 \
-Wno-pointer-sign \
-Wno-sign-compare \
-I"src/common_lib/header" \
-Wall -c \
-DEV_STANDALONE=1 \
-pthread

CCFLAGS := \
-std=c++0x \
-Wno-sign-compare \
-I"src/common_lib/header" \
-Wall \
-DEV_STANDALONE=1 \
-fmessage-length=0 \
-pthread

DEBUG_FLAG := -O0 -g3 
RELASE_FLAG := -O3

DEBUG_OUTDIR := Debug
RELEASE_OUTDIR := Release


_SUBDIRS := \
src/common_lib \
src/common_lib/header \
src/TunnelLib \
src/TunnelLib/ARQ \
src/TunnelLib/InterfaceController \
src/util \
test 

OUTDIR := Debug

COMMON_LIB_OBJ := \
src/common_lib/common.o \
src/common_lib/appThread.o \
src/common_lib/network.o

ARQ_OBJ := \
src/TunnelLib/ARQ/Streamer.o \
src/TunnelLib/ARQ/StreamHandler.o 

INTERFACE_CONTROLLER_OBJ := \
src/TunnelLib/InterfaceController/ValueType.o \
src/TunnelLib/InterfaceController/SearchMac.o \
src/TunnelLib/InterfaceController/Addresses.o \
src/TunnelLib/InterfaceController/SendThroughInterface.o \
src/TunnelLib/InterfaceController/SendingSocket.o \
src/TunnelLib/InterfaceController/arpResolv.o 

TUNNEL_LIB_OBJ := \
src/TunnelLib/PacketEventHandler.o \
src/TunnelLib/multiplexer.o \
src/TunnelLib/SackNewRenoChannelHandler.o \
src/TunnelLib/BasicChannelHandler.o \
src/TunnelLib/PacketPool.o \
src/TunnelLib/CommonHeaderImpl.o \
src/TunnelLib/CubicChannelHandler.o \
src/TunnelLib/InterfaceScheduler.o \
src/TunnelLib/ChannelHandler.o \
src/TunnelLib/Packet.o \
src/TunnelLib/ClientConnection.o \
src/TunnelLib/log.o \
src/TunnelLib/Connection.o \
src/TunnelLib/PendingAcks.o \
src/TunnelLib/ServerConnectionClientDetail.o \
src/TunnelLib/ServerConnection.o \
$(ARQ_OBJ) \
$(INTERFACE_CONTROLLER_OBJ)

UTIL_OBJ := \
src/util/ThreadPool.o \
src/util/Logger.o \
src/util/libev.o \
src/util/LinuxMath.o 

TEST_OBJ := \
test/testThreadPool.o \
test/Main.o \
test/PacketHijacking.o \
test/TrafficGenerator.o \
test/test_distribution.o \
test/TcpTrafficGenerator.o \
test/TcpMultiplexingTrafficGenerator.o 

_OBJS := ${COMMON_LIB_OBJ}
_OBJS += ${UTIL_OBJ}
_OBJS += ${TUNNEL_LIB_OBJ}
_OBJS += ${TEST_OBJ}

LIBS := -lnfnetlink -lnetfilter_queue
OUTDIR := $(DEBUG_OUTDIR)

ifeq ($(release), yes)
	OUTDIR := $(RELEASE_OUTDIR)
	DEBUG_FLAG := $(RELASE_FLAG)
endif

OBJS = $(patsubst %,$(OUTDIR)/%,$(_OBJS))
SUBDIRS := $(patsubst %,$(OUTDIR)/%,$(_SUBDIRS))


#=========================================

all: $(SUBDIRS) $(OUTDIR)/MultiPathUdp
	@echo DONE


clean:
	rm -rf ${OUTDIR}

#========================================
$(OUTDIR)/src/common_lib:
	mkdir -p ${OUTDIR}/src/common_lib

$(OUTDIR)/src/common_lib/header:
	mkdir -p ${OUTDIR}/src/common_lib/header

$(OUTDIR)/src/TunnelLib/ARQ:
	mkdir -p ${OUTDIR}/src/TunnelLib/ARQ

$(OUTDIR)/src/TunnelLib/InterfaceController:
	mkdir -p ${OUTDIR}/src/TunnelLib/InterfaceController

$(OUTDIR)/src/TunnelLib:
	mkdir -p ${OUTDIR}/src/TunnelLib

$(OUTDIR)/src/util:
	mkdir -p ${OUTDIR}/src/util

$(OUTDIR)/test:
	mkdir -p ${OUTDIR}/test

#======================================

$(OUTDIR)/MultiPathUdp: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -pthread -o $(OUTDIR)/"MultiPathUdp" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

$(OUTDIR)/%.o: %.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	$(CC) -c $(CFLAGS) $(DEBUG_FLAG) -o $@ $^
	@echo 'Finished building: $<'
	@echo ' '

$(OUTDIR)/%.o: %.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CPP) -c $(CCFLAGS) $(DEBUG_FLAG) -o $@ $^
	@echo 'Finished building: $<'
	@echo ' '

$(OUTDIR)/src/util/libev.o: src/util/libev.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -Wno-comment -Wno-sign-compare -Wno-unused-value -Wno-parentheses -DEV_STANDALONE=1 -I"src/common_lib/header" $(DEBUG_FLAG) -Wall -c -fmessage-length=0 -pthread -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
