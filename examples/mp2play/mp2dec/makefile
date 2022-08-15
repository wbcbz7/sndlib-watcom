TARGET   = mp2dec

BUILD    = dos

SYSTEM   = dos
DEBUG    = dwarf all
DLEVEL   = 0

SYSDEF   = WIN32

INCLUDE  = .\include

O        = obj
C        = cpp
A        = asm

AS       = nasm.exe
CC       = wpp386.exe
LD       = wlink.exe
CFLAGS   = -5r -fp5 -fpi87 -zp16 -oneatx -ol+ -s -d$(DLEVEL) -d_$(SYSDEF) -i=$(INCLUDE) -bt=$(BUILD)
LFLAGS   =
AFLAGS   = -f win32

# add object files here
OBJS     = mp2dec.obj rdsmp.obj getbits.obj mp2dec_a.obj transfrm.obj

OBJLIST  = $(OBJS)
OBJSTR   = file {$(OBJLIST)}

all: $(TARGET).lib .symbolic

$(TARGET).lib : $(OBJS) .symbolic
	%create $(TARGET).ls
	for %i in ($(OBJS)) do @%append $(TARGET).ls +%i
	
	wlib -n $(TARGET).lib
	wlib    $(TARGET).lib @$(TARGET).ls
	del     $(TARGET).ls
	
.c.obj:
	$(CC) $< $(CFLAGS)
	
.cpp.obj:
	$(CC) $< $(CFLAGS)

.asm.obj:
	$(AS) $< $(AFLAGS)
