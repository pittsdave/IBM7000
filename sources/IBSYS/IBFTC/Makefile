.SUFFIXES: .map .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

.map.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

OBJS = 1ftce.obj 2ftce.obj 3ftce.obj 4ftce.obj 5ftcf.obj 6ftcf.obj 7ftcg.obj

all: $(OBJS) rtl
	$(MAKE) -C rtl

clean:
	rm -f *.obj *.lst
	$(MAKE) -C rtl clean

