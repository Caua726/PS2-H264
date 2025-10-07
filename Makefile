EE_BIN = h264_ps2_player.elf

H264BSD_SRCS := $(wildcard third_party/h264bsd/src/*.c)
PROJECT_SRCS := \
    src/main.c \
    src/ps2_decoder.c \
    src/ps2_video.c

EE_SRCS := $(PROJECT_SRCS) $(H264BSD_SRCS)
EE_OBJS := $(EE_SRCS:.c=.o)

EE_INCS += -Iinclude -Ithird_party/h264bsd/src -I$(PS2DEV)/gsKit/include

EE_LIBS = -lgskit -ldmakit -ldebug -lc -lkernel
EE_LDFLAGS = -L$(PS2SDK)/ee/lib -L$(PS2DEV)/gsKit/lib

EE_CFLAGS = -Wall -O2 -G0 -fno-builtin

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
