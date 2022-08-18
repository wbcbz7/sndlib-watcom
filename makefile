TARGET   = sndlib

BUILD    = dos

SYSTEM   = dos32a
DEBUG    = dwarf all
DLEVEL   = 0

SYSDEF   = DOS

INCLUDE  = .\include

AS       = nasm.exe
CC       = wpp386.exe
LD       = wlink.exe
AFLAGS   = -f win32 -l $<.lst
CFLAGS   = -5r -zp16 -onhasbmi -s -zv -d$(DLEVEL) -d_$(SYSDEF) -i=$(INCLUDE) -bt=$(BUILD)
#CFLAGS   = -5r -zp16 -oneatxh -s -d$(DLEVEL) -d_$(SYSDEF) -i=$(INCLUDE) -bt=$(BUILD)
LFLAGS   =

# add object files here
OBJS     = sndlib.obj convert.obj dma.obj dpmi.obj tinypci.obj irq.obj logerror.obj sndmisc.obj
OBJS     = $(OBJS) snddev.obj devsb.obj devwss.obj devpas.obj devhda.obj devhonk.obj devhonka.obj devds1.obj

OBJLIST  = $(OBJS)
OBJSTR   = file {$(OBJLIST)}

all: $(TARGET).lib .symbolic

$(TARGET).lib : $(OBJS) .symbolic
	%create $(TARGET).ls
	for %i in ($(OBJS)) do @%append $(TARGET).ls +%i
	
	wlib -n $(TARGET).lib
	wlib    $(TARGET).lib @$(TARGET).ls
	del     $(TARGET).ls

# custom rule to enable "option eliminate"
dpmi.obj:
	$(CC) dpmi.cpp $(CFLAGS) -zm
dma.obj:
	$(CC) dma.cpp $(CFLAGS) -zm
tinypci.obj:
	$(CC) tinypci.cpp $(CFLAGS) -zm
	
.cpp.obj:
	$(CC) $< $(CFLAGS)

.asm.obj:
	$(AS) $< $(AFLAGS)