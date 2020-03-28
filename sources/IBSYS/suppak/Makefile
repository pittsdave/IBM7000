
ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

OBJS = suppak.obj 

all: $(OBJS)

clean:
	rm -f $(OBJS) *.lst

suppak.obj: suppak.job
	$(ASM7090) $(ASMFLAGS) -o $@ $<
