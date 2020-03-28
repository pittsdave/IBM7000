
ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

OBJS = ibmapj.obj ibmapk.obj 

all: $(OBJS)

clean:
	rm -f *.obj *.lst

ibmapj.obj: ibmapj.map
	$(ASM7090) $(ASMFLAGS) -o $@ $<
ibmapk.obj: ibmapk.map
	$(ASM7090) $(ASMFLAGS) -o $@ $<
