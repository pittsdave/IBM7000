
ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

OBJS = sort.obj 

all: $(OBJS)

clean:
	rm -f *.obj *.lst

sort.obj: sort.job
	$(ASM7090) $(ASMFLAGS) -o $@ $<
