.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -C -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

TAPES = lib2-1.tap lib2-2.tap

IMAGES1 = fpt0.img free.img fsta.img fstt.img gac0.img gblp.img gbuf.img \
	gclc.img getc.img geti.img getm.img gett.img gloc.img gmem.img \
	gnam.img gopt.img gsys.img gtcl.img gtcm.img gtdy.img gtlo.img \
	gtnm.img gwrd.img int0.img iodg.img ioen.img ioh0.img isin.img \
	macro.img misc.img lib2-1.img
IMAGES2 = ldfl.img ldum.img ljus.img log0.img max0.img max1.img min0.img \
	min1.img mod0.img moun.img move.img mtx0.img ncom.img nexc.img \
	ocbc.img open.img pack.img pad0.img pcmt.img pmow.img pr12.img \
	prdg.img prer.img prfl.img prms.img prnt.img prsl.img \
	macro.img misc.img lib2-2.img

SRCS1 = fpt0.fap free.fap fsta.fap fstt.fap gac0.fap gblp.fap gbuf.fap \
	gclc.fap getc.fap geti.fap getm.fap gett.fap gloc.fap gmem.fap \
	gnam.fap gopt.fap gsys.fap gtcl.fap gtcm.fap gtdy.fap gtlo.fap \
	gtnm.fap gwrd.fap int0.fap iodg.fap ioen.fap ioh0.fap isin.fap 
SRCS2 = ldfl.fap ldum.fap ljus.fap log0.fap max0.fap max1.fap min0.fap \
	min1.fap mod0.fap moun.fap move.fap mtx0.fap ncom.fap nexc.fap \
	ocbc.fap open.fap pack.fap pad0.fap pcmt.fap pmow.fap pr12.fap \
	prdg.fap prer.fap prfl.fap prms.fap prnt.fap prsl.fap

OBJS = fpt0.obj free.obj fsta.obj fstt.obj gac0.obj gblp.obj gbuf.obj \
	gclc.obj getc.obj geti.obj getm.obj gett.obj gloc.obj gmem.obj \
	gnam.obj gopt.obj gsys.obj gtcl.obj gtcm.obj gtdy.obj gtlo.obj \
	gtnm.obj gwrd.obj int0.obj iodg.obj ioen.obj ioh0.obj isin.obj \
	ldfl.obj ldum.obj ljus.obj log0.obj max0.obj max1.obj min0.obj \
	min1.obj mod0.obj moun.obj move.obj mtx0.obj ncom.obj nexc.obj \
	ocbc.obj open.obj pack.obj pad0.obj pcmt.obj pmow.obj pr12.obj \
	prdg.obj prer.obj prfl.obj prms.obj prnt.obj prsl.obj

all: $(OBJS)
	cp $(OBJS) ../tslib1/

clean:
	rm -f $(OBJS) *.lst lib2.runcom
	rm -f $(TAPES) $(IMAGES1) $(IMAGES2)

lib2-1.tap: $(SRCS1) ../include/macro.inc ../include/misc.inc
	ls -1 $(SRCS1) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -t -f macro -e fap -a " M1416PITTS " -o macro.img ../include/macro.inc
	$(OBJ2IMG) -t -f misc -e fap -a " M1416PITTS " -o misc.img ../include/misc.inc
	ls -1 $(SRCS1) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >lib2.runcom
	$(OBJ2IMG) -t -f lib2 -e runcom -a " M1416PITTS " -o lib2-1.img lib2.runcom
	cat $(IMAGES1) ../EOF.BIN >lib2-1.tap

lib2-2.tap: $(SRCS2) ../include/macro.inc ../include/misc.inc
	ls -1 $(SRCS2) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -t -f macro -e fap -a " M1416PITTS " -o macro.img ../include/macro.inc
	$(OBJ2IMG) -t -f misc -e fap -a " M1416PITTS " -o misc.img ../include/misc.inc
	ls -1 $(SRCS2) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >lib2.runcom
	$(OBJ2IMG) -t -f lib2 -e runcom -a " M1416PITTS " -o lib2-2.img lib2.runcom
	cat $(IMAGES2) ../EOF.BIN >lib2-2.tap
