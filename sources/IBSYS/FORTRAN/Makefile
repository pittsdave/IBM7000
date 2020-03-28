.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

OBJS = 9F00.obj 9F01.obj 9F02.obj 9F03.obj 9F04.obj 9F06.obj 9F07.obj \
	9F10.obj 9F11.obj 9F12.obj 9F13.obj 9F18.obj 9F22.obj 9F23.obj \
	9F30.obj 9F32.obj ibsfap.obj \
	fortran.obj library.obj

all: $(OBJS)

clean:
	rm -f *.obj *.lst

fortran.obj: fortran.job
	$(ASM7090) $(ASMFLAGS) -o $@ $<
library.obj: library.job
	$(ASM7090) $(ASMFLAGS) -o $@ $<
