.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

TAPES = source.tap

IMAGES = dbug.img fpd0.img fpm0.img trac.img defs.img tslib2.img

SRCS = dbug.fap fpd0.fap fpm0.fap trac.fap

OBJS = dbug.obj fpd0.obj fpm0.obj trac.obj

all: $(OBJS)

clean:
	rm -f $(OBJS) *.lst tslib2.runcom
	rm -f $(TAPES) $(IMAGES)

source.tap: $(SRCS) defs.inc
	ls -1 $(SRCS) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	ls -1 $(SRCS) | awk -F . '{ print "FAP " $$1 " (LIST) (FLAG) NO\nDELETE " $$1 " SYMTB" }' >tslib2.runcom
	echo "COMBIN * TSLIB2 BSS DBUG FPD0 FPM0 TRAC" >>tslib2.runcom
	$(OBJ2IMG) -t -f defs -e fap -a " M1416PITTS " -o defs.img defs.inc
	$(OBJ2IMG) -t -f tslib2 -e runcom -a " M1416PITTS " -o tslib2.img tslib2.runcom
	cat $(IMAGES) ../EOF.BIN >source.tap
