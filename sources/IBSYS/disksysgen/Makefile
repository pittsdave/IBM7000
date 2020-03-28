.SUFFIXES: .map .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wl $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

.map.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

OBJS := ibsys.obj ibjob.obj jdump.obj dmprec.obj iocsb.obj ibjobb.obj \
	ibjobi.obj ibjobl.obj ibjoby.obj \
	iocs.obj random.obj

all: $(OBJS)

clean:
	rm -f *.obj *.lst

