.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -C -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

TAPES = xlibe.tap

IMAGES = absdat.img abstim.img brake.img delete.img exit.img fix.img fmt.img \
	fracbc.img geta.img getim.img gtch1.img gtwdb.img gtwdbx.img hrmin.img \
	iconv.img move.img namask.img pkwd1.img prdata.img ptias.img \
	reprob.img scan.img snap.img snooze.img spray.img tdec.img tsk.img \
	wrflx.img \
	macro.img misc.img xlibe.img

SRCS = absdat.fap abstim.fap brake.fap delete.fap exit.fap fix.fap fmt.fap \
	fracbc.fap geta.fap getim.fap gtch1.fap gtwdb.fap gtwdbx.fap hrmin.fap \
	iconv.fap move.fap namask.fap pkwd1.fap prdata.fap ptias.fap \
	reprob.fap scan.fap snap.fap snooze.fap spray.fap tdec.fap tsk.fap \
	wrflx.fap

OBJS = absdat.obj abstim.obj brake.obj delete.obj exit.obj fix.obj fmt.obj \
	fracbc.obj geta.obj getim.obj gtch1.obj gtwdb.obj gtwdbx.obj hrmin.obj \
	iconv.obj move.obj namask.obj pkwd1.obj prdata.obj ptias.obj \
	reprob.obj scan.obj snap.obj snooze.obj spray.obj tdec.obj tsk.obj \
	wrflx.obj

all: $(OBJS)

clean:
	rm -f $(OBJS) *.lst xlibe.runcom
	rm -f $(TAPES) $(IMAGES)

xlibe.tap: $(SRCS) ../include/macro.inc ../include/misc.inc
	ls -1 $(SRCS) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -t -f macro -e fap -a " M1416PITTS " -o macro.img ../include/macro.inc
	$(OBJ2IMG) -t -f misc -e fap -a " M1416PITTS " -o misc.img ../include/misc.inc
	#ls -1 $(SRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >xlibe.runcom
	ls -1 $(SRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO" }' >xlibe.runcom
	$(OBJ2IMG) -t -f xlibe -e runcom -a " M1416PITTS " -o xlibe.img xlibe.runcom
	cat $(IMAGES) ../EOF.BIN >xlibe.tap
