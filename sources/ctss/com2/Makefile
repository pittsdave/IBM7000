.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

EXES = fib.exe fibmon.exe hello.exe ldabs.exe link.exe listf.exe
IMAGES = fib.img fibmon.img hello.img fap.img ldabs.img link.img listf.img
TAPES = com2.tap

LIBS = -L../tslib1
XLIBS = -L../xlibe

FIBOBJS = fib.mdo
FIBMONOBJS = fibmon.mdo
HELLOOBJS = hello.obj helout.obj ../com3/messg.obj ../com3/seterr.obj \
	../com3/svcpu.obj
LDABSOBJS = madmain.obj ldabs.mdo cksum.obj
LINKOBJS = link.mdo
LISTFOBJS = listf.mdo lfunc.mdo lsort.mdo nullb.obj readfl.obj

OBJS = abbr.obj cksum.obj comand.obj fap.obj fmstap.obj gencom.obj gtprob.obj \
	hello.obj helout.obj iccom.obj list5.obj madmain.obj main.obj \
	nullb.obj pri.obj profl.obj readfl.obj

all: $(OBJS) $(EXES) $(TAPES)

clean:
	rm -f $(OBJS) *.lst
	rm -f $(EXES) $(IMAGES) *.map *.tap

fib.exe: $(FIBOBJS)
	$(LNK7090) -wxdmo FIB fib.map $@ $(FIBOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f FIB -o fib.img $@

fibmon.exe: $(FIBMONOBJS)
	$(LNK7090) -wxdmo FIBMON fibmon.map $@ $(FIBMONOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f FIBMON -o fibmon.img $@

hello.exe: $(HELLOOBJS)
	$(LNK7090) -wxdmo HELLO hello.map $@ $(HELLOOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f HELO8A -o hello.img $@

ldabs.exe: $(LDABSOBJS)
	$(LNK7090) -wxdmo LDABS ldabs.map $@ $(LDABSOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f LDABS -o ldabs.img $@

link.exe: $(LINKOBJS)
	$(LNK7090) -wxdmo LINK link.map $@ $(LINKOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f LINK -o link.img $@

listf.exe: $(LISTFOBJS)
	$(LNK7090) -wxdmo LISTF listf.map $@ $(LISTFOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f LISTF -o listf.img $@

com2.tap : $(EXES)
	$(OBJ2IMG) -f FAP -o fap.img fap.obj
	cat $(IMAGES) ../EOF.BIN >com2.tap
