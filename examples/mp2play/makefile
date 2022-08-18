TARGET   = mp2play

BUILD    = dos

# define as dos4g for DOS4GW, pmode for PMODE/W build
SYSTEM   = dos32a
DEBUG    = all

# set debug information level
DLEVEL   = 0

# set to 1 to enable callback debug rasterbars
RASTER   = 0

# path to sndlib and MP2 decode library, respectively
MP2DEC_PATH = mp2dec
SNDLIB_PATH = ../..

INCLUDE  = 

O        = obj
C        = cpp
A        = asm

AS       = nasm.exe
CC       = wpp386.exe
LD       = wlink.exe
CFLAGS   = -5r -zp16 -oneatx -s -d$(DLEVEL) -bt=$(BUILD) -I=$(MP2DEC_PATH);$(SNDLIB_PATH) -DDEBUG_RASTER=$(RASTER)
LFLAGS   =
AFLAGS   = -f win32

OBJS     = mp2play.obj mp2dos.obj
OBJSTR   = file {$(OBJS)}

TESTOBJ  = test.obj
TESTOBJSTR  = file {$(TESTOBJ)}

LIBS     = mp2play.lib $(MP2DEC_PATH)\mp2dec.lib $(SNDLIB_PATH)\sndlib.lib
LIBSTR   = library {$(LIBS)}

all: $(TARGET).lib $(TARGET).exe .symbolic

$(TARGET).lib : $(OBJS) .symbolic
	%create $(TARGET).ls
	for %i in ($(OBJS)) do @%append $(TARGET).ls +%i
	
	wlib -n $(TARGET).lib
	wlib    $(TARGET).lib @$(TARGET).ls
	del     $(TARGET).ls

$(TARGET).exe : $(TESTOBJ) .symbolic
	%write $(TARGET).lnk debug $(DEBUG)
	%write $(TARGET).lnk name $(TARGET)
	%write $(TARGET).lnk option map=$(TARGET).map
	%write $(TARGET).lnk option eliminate
	%write $(TARGET).lnk $(TESTOBJSTR) 
	%write $(TARGET).lnk $(LIBSTR) 
	%write $(TARGET).lnk system $(SYSTEM)
	$(LD) @$(TARGET).lnk $(LFLAGS)
	del $(TARGET).lnk

.cpp.obj:
	$(CC) $< $(CFLAGS)

.asm.obj:
	$(AS) $< $(AFLAGS)
	
# clean all
clean: .symbolic
	del *.$(O)
	del $(TARGET).exe
	del *.err