.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

LIBS = -L../tslib1
XLIBS = -L../xlibe

EXES = daemon.exe dsdump.exe dsload.exe
IMAGES = daemon.img dsdump.img dsload.img

TAPES = fgdaem.tap

DAEMONOBJS = daemon.obj geta.obj prcode.obj putin.obj
DSDUMPOBJS = dsdump.obj nlrec.mdo settim.mdo dcycle.obj maplin.obj sdf.obj \
	ckcode.obj rdmfd.obj gdt.obj dusr.obj gdsig.obj dtsave.obj putin.obj \
	prcode.obj xmod.obj
DSLOADOBJS = dsload.obj nlrec.mdo recap.mdo ltfile.obj opendf.obj inittf.obj \
	getlsg.obj askopr.obj geta.obj prcode.obj iobdt.obj xmod.obj

OBJS = askopr.obj ckcode.obj daemon.obj dcycle.obj dsdump.obj dsload.obj \
	dtsave.obj dusr.obj gdsig.obj gdt.obj geta.obj getlsg.obj inittf.obj \
	iobdt.obj ltfile.obj maplin.obj mount.obj opendf.obj prcode.obj \
	putin.obj rdmfd.obj sdf.obj setup.obj swt.obj unique.obj valid.obj \
	xmod.obj

all: $(OBJS) $(EXES) $(TAPES)

clean:
	rm -f $(OBJS) *.lst
	rm -f $(EXES) $(IMAGES) $(TAPES) *.map

daemon.exe: $(DAEMONOBJS)
	$(LNK7090) -wxdmo DAEMON daemon.map $@ $(DAEMONOBJS) $(LIBS)
	$(OBJ2IMG) -f DAEM8A -o daemon.img $@

dsdump.exe: $(DSDUMPOBJS)
	$(LNK7090) -wxdmo DSDUMP dsdump.map $@ $(DSDUMPOBJS) $(LIBS)
	$(OBJ2IMG) -f DSDUMP -o dsdump.img $@

dsload.exe: $(DSLOADOBJS)
	$(LNK7090) -wxdmo DSLOAD dsload.map $@ $(DSLOADOBJS) $(LIBS)
	$(OBJ2IMG) -f DSLOAD -o dsload.img $@

fgdaem.tap : $(EXES)
	cat $(IMAGES) ../EOF.BIN >$@
