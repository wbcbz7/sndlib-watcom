TARGET   = wavesine

# leave this at "dos"
BUILD    = dos

# dos4g for DOS4GW build
# pmodew for PMODE/W build
# causeway for Causeway build
# dos32a for DOS32A build
SYSTEM   = dos32a

# set to "none" to exclude debug inforrmation from the executable
DEBUG    = all

# set debug information level
DLEVEL   = 0

# set to 1 to enable callback debug rasterbars
RASTER   = 0

# path to sndlib
SNDLIB_PATH = ../..

# optional include paths
INCLUDE  = 

!ifdef __UNIX__
DEL = rm -f
!else
DEL = del
!endif

AS       = nasm
CC       = wpp386
LD       = wlink
CFLAGS   = -5r -zp16 -oneatx -s -d$(DLEVEL) -bt=$(BUILD) -I="$(SNDLIB_PATH);$(INCLUDE)" -DDEBUG_RASTER=$(RASTER)
LFLAGS   =
AFLAGS   = -f win32

OBJS  	 = $(TARGET).obj
OBJSTR   = file {$(OBJS)}

LIBS     = $(SNDLIB_PATH)/sndlib.lib
LIBSTR   = library {$(LIBS)}

all: $(TARGET).exe .symbolic

$(TARGET).exe : $(OBJS) .symbolic
	%write $(TARGET).lnk debug $(DEBUG)
	%write $(TARGET).lnk name $(TARGET)
	%write $(TARGET).lnk option map=$(TARGET).map
	%write $(TARGET).lnk option eliminate
	%write $(TARGET).lnk $(OBJSTR) 
	%write $(TARGET).lnk $(LIBSTR) 
	%write $(TARGET).lnk system $(SYSTEM)
	$(LD) @$(TARGET).lnk $(LFLAGS)
	$(DEL) $(TARGET).lnk

.cpp.obj:
	$(CC) $< $(CFLAGS) -fo=$@

.asm.obj:
	$(AS) $< $(AFLAGS) -o $@
	
# clean all
clean: .symbolic
	$(DEL) *.obj
	$(DEL) $(TARGET).exe
	$(DEL) *.err