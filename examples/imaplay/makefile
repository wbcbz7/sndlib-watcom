TARGET   = imaplay

BUILD    = dos

# define as dos4g for DOS4GW, pmode for PMODE/W build
SYSTEM   = dos32a
DEBUG    = all

# set debug information level
DLEVEL   = 0

# set to 1 to enable callback debug rasterbars
RASTER   = 0

# set to 1 to enable high-level IMA ADPCM decoder (debug only)
USE_C_DECODERS = 0

# path to sndlib
SNDLIB_PATH = ../..

INCLUDE  = 

O        = obj
C        = cpp
A        = asm

AS       = nasm.exe
CC       = wpp386.exe
LD       = wlink.exe
CFLAGS   = -5r -fp5 -fpi87 -zp16 -oneatx -s -d$(DLEVEL) -bt=$(BUILD) -I=$(SNDLIB_PATH);$(INCLUDE) -DDEBUG_RASTER=$(RASTER) -DIMAPLAY_USE_C_DECODERS=$(USE_C_DECODERS)
LFLAGS   =
AFLAGS   = -f win32 -dIMAPLAY_USE_C_DECODERS=$(USE_C_DECODERS)

OBJS     = imaplay.obj imados.obj decode.obj
OBJSTR   = file {$(OBJS)}

TESTOBJ  = test.obj
TESTOBJSTR  = file {$(TESTOBJ)}

LIBS     = imaplay.lib $(SNDLIB_PATH)\sndlib.lib
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