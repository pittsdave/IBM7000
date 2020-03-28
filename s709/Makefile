CC = gcc
INSTDIR = /usr/local/bin

TARGETS = s709 bcd2txt bcd2bin bcd2cbn bin2bcd bin2txt cbn2bcd crd2cbn txt2bcd \
	txt2bin gendate genext

S709OBJ = arith.o binloader.o chan7607.o chan7909.o console.o io.o lights.o \
	negop.o parity.o posop.o s709.o screen.o sense.o commdev.o

BCD2CBNOBJ = bcd2cbn.o prsf2.o
BCD2BINOBJ = bcd2bin.o prsf2.o
BCD2TXTOBJ = bcd2txt.o prsf2.o
BCDDUMPOBJ = bcddump.o prsf2.o

BIN2BCDOBJ = bin2bcd.o prsf2.o
BIN2TXTOBJ = bin2txt.o prsf2.o

CBN2BCDOBJ = cbn2bcd.o prsf2.o
CRD2CBNOBJ = crd2cbn.o prsf2.o

TXT2BCDOBJ = txt2bcd.o prsf2.o
TXT2BINOBJ = txt2bin.o prsf2.o

GENDATEOBJ = gendate.o
GENEXTOBJ = genext.o

all :
	@if [ "`uname -s`" = "Linux" ] ; then \
		echo "Making Linux on a `uname -m`" ;\
		case "`uname -m`" in \
		x86_64 | s390x ) \
			make $(TARGETS) \
			"LIBS = -lpthread" \
			"CFLAGS = -DUSE64 -DUNIX -DLINUX -DANSICRT -Iinclude $(DEBUG)" ;\
			;; \
		*) \
			make $(TARGETS) \
			"LIBS = -lpthread" \
			"CFLAGS = -DUNIX -DLINUX -DANSICRT -Iinclude $(DEBUG)" ;\
			;; \
		esac ;\
	elif [ "`uname -s`" = "AIX" ] ; then \
		echo "Making AIX" ;\
		make $(TARGETS) \
		"CFLAGS = -O -DUNIX -DAIX -DANSICRT -Iinclude $(DEBUG)" ;\
	elif [ "`uname -s`" = "Darwin" ] ; then \
		echo "Making OSX" ;\
		make $(TARGETS) \
		"CFLAGS = -O -DUNIX -DDARWIN -DANSICRT -Iinclude $(DEBUG)" ;\
	elif [ "`uname -s`" = "HP-UX" ] ; then \
		echo "Making HP-UX" ;\
		make $(TARGETS) \
		"CFLAGS = -O -DUNIX -DHPUX -DANSICRT -Iinclude $(DEBUG)" ;\
	elif [ "`uname -s`" = "OS/390" ] ; then \
		echo "Making OS/390 USS" ;\
		make $(TARGETS) \
		"CC = cc" \
		"CFLAGS = -DUNIX -DUSS -DANSICRT -Iinclude $(DEBUG)" ;\
	elif [ "`uname -s`" = "SunOS" ] ; then \
		echo "Making Solaris" ;\
		make $(TARGETS) \
		"LIBS = -lpthread" \
		"CFLAGS = -O -DUNIX -DSOLARIS -DANSICRT -Iinclude $(DEBUG)" ;\
        elif [ "`uname -s | cut -c1-7`" = "MINGW32" ] ; then \
                echo "Making `uname -s` with MSYS" ;\
                make $(TARGETS) \
		"CFLAGS=-DWIN32 -DMINGW -DANSICRT -Iinclude $(DEBUG)" ;\
	else \
		echo "OS type `uname -s` is unknown" ;\
		echo "You must enter an OS type. OS types are:" ;\
		echo "   aix | darwin | hpux | linux | mingw | nt | solaris | uss " ;\
		echo " " ;\
	fi


aix: 
	@make $(TARGETS) \
		"CFLAGS = -O -DUNIX -DAIX -DANSICRT -Iinclude $(DEBUG)"

darwin: 
	@make $(TARGETS) \
		"CFLAGS = -O -DUNIX -DDARWIN -DANSICRT -Iinclude $(DEBUG)"

hpux: 
	@make $(TARGETS) \
		"CFLAGS = -O -DUNIX -DHPUX -DANSICRT -Iinclude $(DEBUG)"

linux: 
	@make $(TARGETS) \
		"CFLAGS = -O2 -DUNIX -DLINUX -DANSICRT -Iinclude $(DEBUG)"

mingw:
	@make $(TARGETS) \
		"CFLAGS = -O -DWIN32 -DMINGW -DANSICRT -Iinclude $(DEBUG)"

nt:
	@nmake/nologo -f makefile.nt

solaris:
	@make $(TARGETS) \
		"LIBS = -lpthread" \
		"CFLAGS = -O -DUNIX -DSOLARIS -DANSICRT -Iinclude $(DEBUG)"

uss:
	@make $(TARGETS) \
		"CC = cc" \
		"CFLAGS = -DUNIX -DUSS -DANSICRT -Iinclude $(DEBUG)"


s709: $(S709OBJ)
	$(CC) -o s709 $(S709OBJ) $(DEBUG) $(LIBS)

bcd2bin: $(BCD2BINOBJ)
	$(CC) -o bcd2bin $(BCD2BINOBJ)
bcd2cbn: $(BCD2CBNOBJ)
	$(CC) -o bcd2cbn $(BCD2CBNOBJ)
bcd2txt: $(BCD2TXTOBJ)
	$(CC) -o bcd2txt $(BCD2TXTOBJ)

bin2bcd: $(BIN2BCDOBJ)
	$(CC) -o bin2bcd $(BIN2BCDOBJ)
bin2txt: $(BIN2TXTOBJ)
	$(CC) -o bin2txt $(BIN2TXTOBJ)

cbn2bcd: $(CBN2BCDOBJ)
	$(CC) -o cbn2bcd $(CBN2BCDOBJ)
crd2cbn: $(CRD2CBNOBJ)
	$(CC) -o crd2cbn $(CRD2CBNOBJ)

txt2bcd: $(TXT2BCDOBJ)
	$(CC) -o txt2bcd $(TXT2BCDOBJ)
txt2bin: $(TXT2BINOBJ)
	$(CC) -o txt2bin $(TXT2BINOBJ)

gendate: $(GENDATEOBJ)
	$(CC) -o gendate $(GENDATEOBJ)

genext: $(GENEXTOBJ)
	$(CC) -o genext $(GENEXTOBJ)

clean: cleanrun
	-rm -f $(S709OBJ) s709
	-rm -f $(BCD2BINOBJ) bcd2bin
	-rm -f $(BCD2CBNOBJ) bcd2cbn
	-rm -f $(BCD2TXTOBJ) bcd2txt
	-rm -f $(BCDDUMPOBJ) bcddump
	-rm -f $(BIN2BCDOBJ) bin2bcd
	-rm -f $(BIN2TXTOBJ) bin2txt
	-rm -f $(CBN2BCDOBJ) cbn2bcd
	-rm -f $(CRD2CBNOBJ) crd2cbn
	-rm -f $(TXT2BCDOBJ) txt2bcd
	-rm -f $(TXT2BINOBJ) txt2bin
	-rm -f $(GENDATEOBJ) gendate
	-rm -f $(GENEXTOBJ) genext

cleanrun:
	-rm -f print.* *.lst *.bin *.obj *.trc
	-rm -f reader.* punch.* tape1a sys*
	-rm -f cmd.txt cmd.cbn cmd.bcd out.bcd

distclean: clean
	-rm -f IBSYS1.DSK IBSYS2.DSK IBSYS3.DSK tags cscope*
	-rm -f diagctss extractctss installctss runctss setupctss salvagectss \
	runctsst cleanupctss dskedtctss plotctss adduserctss deluserctss \
	reportuserctss
	-rm -f diagctss.cmd runctss.cmd utilctss.cmd fmtchk.cmd fmt.cmd \
	mfd.cmd loader.cmd salvagectss.cmd runctsst.cmd cleanupctss.cmd \
	dskedtctss.cmd 
	-rm -f ctss.map salvage.map com800.symtb

dasdlink: 
	-ln -s ../dasd/IBSYS1.DSK 
	-ln -s ../dasd/IBSYS2.DSK 
	-ln -s ../dasd/IBSYS3.DSK 

install:
	cp $(TARGETS) $(INSTDIR)

tags: *.c include/*.h
	@ctags *.c include/*.h

cscope: *.c include/*.h
	@cscope -u -b -k -q *.c include/*.h

arith.o: arith.c include/sysdef.h include/regs.h include/s709.h \
	include/console.h
binloader.o: binloader.c include/sysdef.h include/regs.h include/binloader.h
chan7607.o: chan7607.c include/sysdef.h include/regs.h include/s709.h \
	include/parity.h include/io.h include/trace.h include/chan7607.h \
	include/chan7909.h include/console.h include/lights.h include/dasd.h
chan7909.o: chan7909.c include/sysdef.h include/regs.h include/s709.h \
	include/parity.h include/io.h include/trace.h include/chan7607.h \
	include/chan7909.h include/console.h include/lights.h include/dasd.h \
	include/comm.h
console.o: console.c include/sysdef.h include/regs.h include/trace.h \
	include/lights.h include/io.h include/chan7607.h include/chan7909.h \
	include/console.h include/screen.h include/binloader.h 
io.o: io.c include/sysdef.h include/regs.h include/parity.h \
	include/nativebcd.h include/chan7607.h include/chan7909.h \
	include/io.h include/screen.h include/dasd.h include/comm.h
lights.o: lights.c include/sysdef.h include/regs.h include/trace.h \
	include/io.h include/screen.h
negop.o: negop.c include/sysdef.h include/regs.h include/s709.h \
	include/arith.h include/io.h include/chan7607.h include/chan7909.h \
	include/negop.h
parity.o: parity.c include/sysdef.h
posop.o: posop.c include/sysdef.h include/regs.h include/s709.h \
	include/arith.h include/chan7607.h include/chan7909.h include/io.h \
	include/posop.h
s709.o: s709.c include/sysdef.h include/regs.h include/trace.h include/s709.h \
	include/io.h include/chan7607.h include/chan7909.h include/negop.h \
	include/posop.h include/sense.h include/console.h include/lights.h \
	include/screen.h
screen.o: screen.c
sense.o: sense.c include/sysdef.h include/regs.h include/arith.h \
	include/chan7607.h include/chan7909.h
commdev.o: commdev.c include/sysdef.h include/regs.h include/parity.h \
	include/nativebcd.h include/chan7607.h include/chan7909.h \
	include/io.h include/screen.h include/comm.h

bcd2bin.o: bcd2bin.c include/sysdef.h include/cvtpar.h include/prsf2.h
bcd2cbn.o: bcd2bin.c include/sysdef.h include/cvtpar.h include/prsf2.h
bcd2txt.o: bcd2txt.c include/sysdef.h include/nativebcd.h include/prsf2.h
bcddump.o: bcddump.c include/sysdef.h include/prsf2.h include/parity.h

bin2bcd.o: bin2bcd.c include/sysdef.h include/prsf2.h include/cvtpar.h
bin2txt.o: bin2txt.c include/sysdef.h include/prsf2.h include/cvtpar.h

cbn2bcd.o: cbn2bcd.c include/sysdef.h include/prsf2.h include/cvtpar.h
crd2cbn.o: crd2cbn.c include/sysdef.h include/prsf2.h

txt2bcd.o: txt2bcd.c include/sysdef.h include/prsf2.h include/nativebcd.h \
	include/cvtpar.h
txt2bin.o: txt2bin.c include/sysdef.h include/prsf2.h include/cvtpar.h

prsf2.o: prsf2.c
