.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -C -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

EXES = madtrn.exe 
IMAGES = madtrn.img
TAPES = com4.tap

LIBS = -L../tslib1
XLIBS = -L../xlibe

MADTRNOBJS = madmain.obj mtrn.mdo mtr2.mdo mrrp.mdo mfns.mdo iop9.mdo bcdbn.obj

OBJS = bcdbn.obj madbug.obj madmain.obj

all: $(OBJS) $(EXES) $(TAPES)

clean:
	rm -f $(OBJS) *.lst
	rm -f $(EXES) $(IMAGES) *.map *.tap

madtrn.exe: $(madtrnOBJS)
	$(LNK7090) -C -wxdmo MADTRN madtrn.map $@ $(MADTRNOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f MADTRN -o madtrn.img $@

com4.tap : $(EXES)
	cat $(IMAGES) ../EOF.BIN >com4.tap

