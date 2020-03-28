
ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

OBJS = 9pac.obj 

all: $(OBJS)

clean:
	rm -f *.obj *.lst

9pac.obj: 9pac.job
	$(ASM7090) $(ASMFLAGS) -o $@ $<
