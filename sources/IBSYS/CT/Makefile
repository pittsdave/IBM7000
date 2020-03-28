
ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

OBJS = ct.obj 

all: $(OBJS)

clean:
	rm -f $(OBJS) *.lst

ct.obj: ct.fap
	$(ASM7090) $(ASMFLAGS) -o $@ $<
