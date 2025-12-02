TARGET   = mp2dec

BUILD    = dos

SYSTEM   = dos
DEBUG    = dwarf all
DLEVEL   = 0

SYSDEF   = WIN32

INCLUDE  = ./include

!ifdef __UNIX__
DEL = rm -f
!else
DEL = del
!endif

O        = obj
C        = cpp
A        = asm

AS       = nasm
CC       = wpp386
LD       = wlink
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
	$(DEL)  $(TARGET).ls
	
.c.obj:
	$(CC) $< $(CFLAGS) -fo=$@
	
.cpp.obj:
	$(CC) $< $(CFLAGS) -fo=$@

.asm.obj:
	$(AS) $< $(AFLAGS) -o $@
