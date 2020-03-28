.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

OBJS = ddfap.obj enable.obj exitsm.obj fptnte.obj fsetup.obj iopac.obj \
	supcal.obj trac.obj

all: $(OBJS)

clean:
	rm -f $(OBJS) *.lst

