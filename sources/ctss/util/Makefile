.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

EXES = pusav.exe sumary.exe uascan.exe
IMAGES = links.img sumary.img uascan.img
TAPES = util.tap

LIBS = -L../tslib1
XLIBS = -L../xlibe

PUSAVOBJS = pusav.obj
SUMARYOBJS = sumary.obj daymon.obj
UASCANOBJS = uascan.obj

OBJS = 7read.obj acor.obj adbg.obj args.obj askopr.obj bcrful.obj bitbl.obj \
	calib.obj daymon.obj pusav.obj sumary.obj uascan.obj

all: $(OBJS) $(EXES) $(TAPES)

clean:
	rm -f $(OBJS) *.lst
	rm -f $(EXES) $(IMAGES) *.map *.tap *.img

pusav.exe: $(PUSAVOBJS)
	$(LNK7090) -wxdmo pusav pusav.map $@ $(PUSAVOBJS)  $(LIBS)
	$(OBJ2IMG) -f pusav -e SAVED -a " M1416PITTS " -o pusav.img $@

sumary.exe: $(SUMARYOBJS)
	$(LNK7090) -wxdmo SUMARY sumary.map $@ $(SUMARYOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f SUMARY -e SAVED -a " M1416ADMIN " -o sumary.img $@

uascan.exe: $(UASCANOBJS)
	$(LNK7090) -wxdmo UASCAN uascan.map $@ $(UASCANOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f UASCAN -e SAVED -a " M1416ADMIN " -o uascan.img $@

util.tap : $(EXES) links.dat
	$(OBJ2IMG) -l -f LINKS -e DAT -a " M1416ADMIN " -o links.img links.dat
	cat $(IMAGES) ../EOF.BIN >$@
