CC    := ppc-amigaos-gcc
STRIP := ppc-amigaos-strip

CFLAGS  := -O2 -g -Wall -Wwrite-strings -Werror -I. -I./include
LDFLAGS := -static
LIBS    := 

TARGET  := cdplayer.library
VERSION := 52

SRCS := init.c cdplayer_68k.c cdplayer_private.c

main_SRCS := main/CDActive.c main/CDCurrentTitle.c main/CDEject.c main/CDGetVolume.c \
             main/CDInfo.c main/CDJump.c main/CDPlay.c main/CDPlayAddr.c main/CDReadTOC.c \
             main/CDResume.c main/CDSetVolume.c main/CDStop.c main/CDTitleTime.c

OBJS := $(SRCS:.c=.o) $(main_SRCS:.c=.o)

.PHONY: all
all: $(TARGET) examples/simple_play

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -nostartfiles -o $@.debug $^ $(LIBS)
	$(STRIP) -R.comment -o $@ $@.debug

init.o: $(TARGET)_rev.h

$(OBJS): cdplayer_private.h

examples/simple_play: examples/simple_play.o
	$(CC) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) -R.comment -o $@ $@.debug

examples/simple_play.o: CFLAGS += -D__USE_INLINE__ -Wno-deprecated-declarations

.PHONY: clean
clean:
	rm -f $(TARGET) $(TARGET).debug *.o main/*.o
	rm -f examples/simple_play examples/simple_play.debug examples/*.o

.PHONY: revision
revision:
	bumprev $(VERSION) $(TARGET)

