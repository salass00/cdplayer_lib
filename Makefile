CC    := ppc-amigaos-gcc
AS    := ppc-amigaos-as
STRIP := ppc-amigaos-strip

CFLAGS  := -O2 -g -Wall -Wwrite-strings -Werror -I. -I./include
ASFLAGS := -mregnames
LDFLAGS := -static
LIBS    := 

TARGET := cdplayer.library
VERSION := 52

OBJS := init.o cdplayer_68k.o main/CDEject.o main/CDPlay.o main/CDResume.o \
	main/CDStop.o main/CDJump.o main/CDActive.o main/CDCurrentTitle.o \
	main/CDTitleTime.o main/CDGetVolume.o main/CDSetVolume.o \
	main/CDReadTOC.o main/CDInfo.o main/CDPlayAddr.o cdplayer_private.o
RELEASEDIR := ../

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

