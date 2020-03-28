
ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

OBJS = iocs.obj random.obj

all: $(OBJS)

clean:
	rm -f *.obj *.lst

iocs.obj: iocs.fap
	$(ASM7090) $(ASMFLAGS) -o $@ $<
random.obj: random.fap
	$(ASM7090) $(ASMFLAGS) -o $@ $<
