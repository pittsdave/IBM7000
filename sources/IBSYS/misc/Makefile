.SUFFIXES: .map .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

.map.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

OBJS = car03.obj car04.obj car08.obj car10.obj car11.obj car13.obj car14.obj \
	car15.obj cars1.obj cars2.obj _dstrn.obj _dstro.obj _ibdbi.obj \
	_ibsys.obj _iocsb.obj _iocsl.obj _iocs.obj _iocsm.obj _iodef.obj \
	_ioex.obj _jbcon.obj

all: $(OBJS) 

clean:
	rm -f *.obj *.lst

