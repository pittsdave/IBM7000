
ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

OBJS = ior00.obj 

all: $(OBJS)

clean:
	rm -f *.obj *.lst

ior00.obj: ior00.map
	$(ASM7090) $(ASMFLAGS) -o $@ $<
