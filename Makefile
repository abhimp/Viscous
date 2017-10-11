CC              := gcc
CPP             := g++
LINK            := g++
AR              := ar

OUTPUT_LIBRARY  := Viscous
OUTPUT_LIB_OBJ  := lib$(OUTPUT_LIBRARY).a
OUTPUT_BINARY   := ViscousTest

CFLAGS          := \
                -std=c11 \
                -Wno-pointer-sign \
                -Wno-sign-compare \
                -I"src/common_lib/header" \
                -Wall -c \
                -DNEW_CHANNEL_HANDLE \
                -DEV_STANDALONE=1 \
                -pthread

CCFLAGS         := \
                -std=c++0x \
                -Wno-sign-compare \
                -I"src/common_lib/header" \
                -Wall \
                -DNEW_CHANNEL_HANDLE \
                -DEV_STANDALONE=1 \
                -fmessage-length=0 \
                -pthread

DEBUG_FLAG      := -O0 -g3 
RELASE_FLAG     := -O3

DEBUG_OUTDIR    := Debug
RELEASE_OUTDIR  := Release


_SUBDIRS        := \
                src/common_lib \
                src/common_lib/header \
                src/TunnelLib \
                src/TunnelLib/FlowHandler \
                src/TunnelLib/InterfaceController \
                src/util \
                test \


OUTDIR          := Debug

_COMMON_LIB_OBJ := \
                src/common_lib/common.o \
                src/common_lib/appThread.o \
                src/common_lib/network.o \


_FLOW_HANDLER_OBJ        := \
                src/TunnelLib/FlowHandler/Streamer.o \
                src/TunnelLib/FlowHandler/StreamHandler.o \
                src/TunnelLib/FlowHandler/NewFlowHandler.o \


_INTERFACE_CONTROLLER_OBJ := \
                src/TunnelLib/InterfaceController/ValueType.o \
                src/TunnelLib/InterfaceController/SearchMac.o \
                src/TunnelLib/InterfaceController/Addresses.o \
                src/TunnelLib/InterfaceController/SendThroughInterface.o \
                src/TunnelLib/InterfaceController/SendingSocket.o \
                src/TunnelLib/InterfaceController/arpResolv.o \
				src/TunnelLib/InterfaceController/InterfaceMonitor.o \


_CHANNEL_HANDLER_OBL := \
                src/TunnelLib/ChannelHandler/ChannelHandler.o \
                src/TunnelLib/ChannelHandler/BasicChannelHandler.o \
                src/TunnelLib/ChannelHandler/SackNewRenoChannelHandler.o \
                src/TunnelLib/ChannelHandler/NewChannelHandler.o \


_TUNNEL_LIB_OBJ := \
                src/TunnelLib/PacketEventHandler.o \
                src/TunnelLib/multiplexer.o \
                src/TunnelLib/PacketPool.o \
                src/TunnelLib/CommonHeaderImpl.o \
                src/TunnelLib/ChannelScheduler/InterfaceScheduler.o \
				src/TunnelLib/ChannelScheduler/NewChannelScheculer.o \
                src/TunnelLib/Packet.o \
                src/TunnelLib/ClientConnection.o \
                src/TunnelLib/Connection.o \
                src/TunnelLib/PendingAcks.o \
                src/TunnelLib/ServerConnectionClientDetail.o \
                src/TunnelLib/ServerConnection.o \
                $(_FLOW_HANDLER_OBJ) \
                $(_CHANNEL_HANDLER_OBL) \
                $(_INTERFACE_CONTROLLER_OBJ)

#                src/TunnelLib/ChannelHandler/CubicChannelHandler.o 
#                src/TunnelLib/ChannelHandler/log.o 

_UTIL_OBJ       := \
                src/util/ThreadPool.o \
                src/util/Logger.o \
                src/util/LinuxMath.o \
                src/util/AppThread.o \

#                src/util/libev.o \


_EVAL_OBJ       := \
                evaluation/testThreadPool.o \
                evaluation/TrafficGenerator.o \
                evaluation/test_distribution.o \
                evaluation/TcpTrafficGenerator.o \
				evaluation/ListentoNetworkEvents.o \
				evaluation/QualityTest.o \
				evaluation/TimeTest.o \
                evaluation/Main.o \

#                evaluation/TcpMultiplexingTrafficGenerator.o \
#                evaluation/PacketHijacking.o \

OUTDIR          := $(DEBUG_OUTDIR)

ifeq ($(release), yes)
    OUTDIR := $(RELEASE_OUTDIR)
    DEBUG_FLAG := $(RELASE_FLAG)
endif


_OBJS           := ${_COMMON_LIB_OBJ}
_OBJS           += ${_UTIL_OBJ}
_OBJS           += ${_TUNNEL_LIB_OBJ}
_OBJS           += ${_EVAL_OBJ}

LIBS            := 
#-lnfnetlink -lnetfilter_queue


COMMON_LIB_OBJ  := $(patsubst %,$(OUTDIR)/%,$(_COMMON_LIB_OBJ))
UTIL_OBJ        := $(patsubst %,$(OUTDIR)/%,$(_UTIL_OBJ))
TUNNEL_LIB_OBJ  := $(patsubst %,$(OUTDIR)/%,$(_TUNNEL_LIB_OBJ))
EVAL_OBJ        := $(patsubst %,$(OUTDIR)/%,$(_EVAL_OBJ))
OBJS            := $(patsubst %,$(OUTDIR)/%,$(_OBJS))
SUBDIRS         := $(patsubst %,$(OUTDIR)/%,$(_SUBDIRS))

COMMON_LIB_DEP  := $(patsubst %.o,%.d,$(COMMON_LIB_OBJ))
UTIL_DEP        := $(patsubst %.o,%.d,$(UTIL_OBJ))
TUNNEL_LIB_DEP  := $(patsubst %.o,%.d,$(TUNNEL_LIB_OBJ))
EVAL_DEP        := $(patsubst %.o,%.d,$(EVAL_OBJ))
DEPS            := $(patsubst %.o,%.d,$(OBJS))

#=========================================
.PHONY: clean doxy doxy-clean all deps

.SECONDARY:

all: $(OUTDIR)/$(OUTPUT_BINARY)

LOAD := no

ifeq (clean, $(filter clean, $(MAKECMDGOALS)))
    LOAD := no
else ifeq (all, $(filter all, $(MAKECMDGOALS)))
    LOAD := yes
else ifndef $(MAKECMDGOALS)
    LOAD := yes
endif

ifeq ($(LOAD), yes)
-include $(DEPS)
endif

clean:
	rm -rf $(OBJS) $(DEPS) 
	rm -rf $(OUTDIR)/$(OUTPUT_LIB_OBJ) $(OUTDIR)/$(OUTPUT_BINARY)
	rm -rf $(OUTDIR)

doxy-clean: 
	@rm -rf html latex
doxy:
	doxygen Doxyfile

$(OUTDIR)/$(OUTPUT_LIB_OBJ): $(COMMON_LIB_OBJ) $(UTIL_OBJ) $(TUNNEL_LIB_OBJ)
	@echo 'archiving'
	@$(AR) -rv $@ $^

$(OUTDIR)/$(OUTPUT_BINARY): $(OUTDIR)/$(OUTPUT_LIB_OBJ) $(EVAL_OBJ)
	@echo "LD 	$@"
	@$(LINK) -pthread -o $@ $(EVAL_OBJ) -L $(OUTDIR) $(LIBS) -l$(OUTPUT_LIBRARY)

$(OUTDIR)/%.o: %.c $(OUTDIR)/%.d
	@mkdir -p $(dir $@)
	@echo "CC 	$<"
	@$(CC) -c $(CFLAGS) $(DEBUG_FLAG) -o $@ $<

$(OUTDIR)/%.o: %.cc $(OUTDIR)/%.d
	@mkdir -p $(dir $@)
	@echo "CPP 	$<"
	@$(CPP) -c $(CCFLAGS) $(DEBUG_FLAG) -o $@ $<

$(OUTDIR)/src/util/libev.o: src/util/libev.cc $(OUTDIR)/src/util/libev.d
	@mkdir -p $(dir $@)
	@echo "CPP 	$<"
	@$(CPP) -std=c++0x -Wno-comment -Wno-sign-compare -Wno-unused-value -Wno-parentheses -DEV_STANDALONE=1 -I"src/common_lib/header" $(DEBUG_FLAG) -Wall -c -fmessage-length=0 -pthread -o "$@" "$<"

#=======Dependancy====
$(OUTDIR)/%.d: %.c
	@mkdir -p $(dir $@)
	@echo "DEPS 	$@"
	@$(CC) $(CFLAGS) $(DEBUG_FLAG) -MG -MM -MT "$@" -MF"$@" $<

$(OUTDIR)/%.d: %.cc
	@mkdir -p $(dir $@)
	@echo "DEPS 	$@"
	@$(CPP) $(CCFLAGS) $(DEBUG_FLAG) -MG -MM -MT "$@" -MF"$@" $<
