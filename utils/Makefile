CC = gcc

TARGETS = obj2bin obj2img bd bsplit bincmp disasm gelbl2txt mkdasd \
	gendate txt2bcd bcd2txt patchdasd bss2obj stripbss punchctss \
	plotcvt genext useraddctss userdelctss userreportctss
INSTDIR = /usr/local/bin

BCDDUMP = bd.o 
PATCHDASD = patchdasd.o 
OBJ2CBN = obj2cbn.o 
OBJ2BIN = obj2bin.o
OBJ2IMG = obj2img.o
BCDSPLIT = bsplit.o 
BINCMP = bincmp.o 
DISASM = disasm.o 
MKDASD = mkdasd.o 
GELBL2TXT = gelbl2txt.o 
BSS2OBJ = bss2obj.o prsf2.o
BCD2BSSOBJ = bcd2bss.o
STRIPBSSOBJ = stripbss.o

GENDATEOBJ = gendate.o
GENEXTOBJ = genext.o
BCD2TXTOBJ = bcd2txt.o prsf2.o
TXT2BCDOBJ = txt2bcd.o prsf2.o

PUNCHCTSSOBJ = punchctss.o
PLOTCVTOBJ = plotcvt.o calcomp.o

USERADDOBJ = useraddctss.o usersupport.o
USERDELOBJ = userdelctss.o usersupport.o
USERREPORTOBJ = userreportctss.o usersupport.o

all: 
	@if [ "`uname -s`" = "Linux" ] ; then \
		echo "Making Linux on a `uname -m`" ;\
		make $(TARGETS) "CFLAGS=-DLINUX $(DEBUG)" ;\
	elif [ "`uname -s`" = "AIX" ] ; then \
		echo "Making AIX" ;\
		make $(TARGETS) "CFLAGS=-DAIX $(DEBUG)" ;\
	elif [ "`uname -s`" = "Darwin" ] ; then \
		echo "Making OSX" ;\
		make $(TARGETS) "CFLAGS=-DDARWIN $(DEBUG)" ;\
	elif [ "`uname -s`" = "HP-UX" ] ; then \
		echo "Making HP-UX" ;\
		make $(TARGETS) "CFLAGS=-DHPUX $(DEBUG)" ;\
	elif [ "`uname -s`" = "OS/390" ] ; then \
		echo "Making OS/390 USS" ;\
		make $(TARGETS) "CC=cc" "CFLAGS=-DUSS $(DEBUG)" ;\
	elif [ "`uname -s`" = "SunOS" ] ; then \
		echo "Making Solaris" ;\
		make $(TARGETS) "CFLAGS=-DSOLARIS $(DEBUG)" ;\
        elif [ "`uname -s | cut -c1-7`" = "MINGW32" ] ; then \
                echo "Making `uname -s` with MSYS" ;\
                make $(TARGETS) "CFLAGS=-DWIN32 -DMINGW $(DEBUG)" ;\
	else \
		echo "OS type `uname -s` is unknown" ;\
		echo "You must enter an OS type. OS types are:" ;\
		echo "   aix | darwin | hpux | linux | nt | openmvs | solaris" ;\
		echo " " ;\
	fi

aix:
	@make $(TARGETS) "CFLAGS = -DAIX $(DEBUG)"

darwin:
	@make $(TARGETS) "CFLAGS = -DDARWIN $(DEBUG)"

hpux:
	@make $(TARGETS) "CFLAGS = -DHPUX $(DEBUG)"

linux:
	@make $(TARGETS) "CFLAGS = -DLINUX $(DEBUG)"

nt:
	@nmake/nologo -f makefile.nt

openmvs:
	@make $(TARGETS) "CFLAGS = -DUSS $(DEBUG)"

solaris:
	@make $(TARGETS) "CFLAGS = -DSOLARIS $(DEBUG)"


obj2bin: $(OBJ2BIN)
	$(CC) -o $@ $(OBJ2BIN) $(DEBUG)

obj2img: $(OBJ2IMG)
	$(CC) -o $@ $(OBJ2IMG) $(DEBUG)

obj2cbn: $(OBJ2CBN)
	$(CC) -o $@ $(OBJ2CBN) $(DEBUG)

bd: $(BCDDUMP)
	$(CC) -o $@ $(BCDDUMP) $(DEBUG)

patchdasd: $(PATCHDASD)
	$(CC) -o $@ $(PATCHDASD) $(DEBUG)

bsplit: $(BCDSPLIT)
	$(CC) -o $@ $(BCDSPLIT) $(DEBUG)

bincmp: $(BINCMP)
	$(CC) -o $@ $(BINCMP) $(DEBUG)

disasm: $(DISASM)
	$(CC) -o $@ $(DISASM) $(DEBUG)

gelbl2txt: $(GELBL2TXT)
	$(CC) -o $@ $(GELBL2TXT) $(DEBUG)

mkdasd: $(MKDASD)
	$(CC) -o $@ $(MKDASD) $(DEBUG)

bss2obj: $(BSS2OBJ)
	$(CC) -o bss2obj $(BSS2OBJ) $(DEBUG)

bcd2bss: $(BCD2BSSOBJ)
	$(CC) -o bcd2bss $(BCD2BSSOBJ) $(DEBUG)

stripbss: $(STRIPBSSOBJ)
	$(CC) -o stripbss $(STRIPBSSOBJ) $(DEBUG)

bcd2txt: $(BCD2TXTOBJ)
	$(CC) -o bcd2txt $(BCD2TXTOBJ) $(DEBUG)

txt2bcd: $(TXT2BCDOBJ)
	$(CC) -o txt2bcd $(TXT2BCDOBJ) $(DEBUG)

gendate: $(GENDATEOBJ)
	$(CC) -o gendate $(GENDATEOBJ) $(DEBUG)

genext: $(GENEXTOBJ)
	$(CC) -o genext $(GENEXTOBJ) $(DEBUG)

punchctss: $(PUNCHCTSSOBJ)
	$(CC) -o punchctss $(PUNCHCTSSOBJ) $(DEBUG)

plotcvt: $(PLOTCVTOBJ)
	$(CC) -o plotcvt $(PLOTCVTOBJ) $(DEBUG) -lm

useraddctss: $(USERADDOBJ)
	$(CC) -o useraddctss $(USERADDOBJ) $(DEBUG)

userdelctss: $(USERDELOBJ)
	$(CC) -o userdelctss $(USERDELOBJ) $(DEBUG)

userreportctss: $(USERREPORTOBJ)
	$(CC) -o userreportctss $(USERREPORTOBJ) $(DEBUG)

clean:
	rm -f $(OBJ2CBN) obj2cbn
	rm -f $(OBJ2BIN) obj2bin
	rm -f $(OBJ2IMG) obj2img
	rm -f $(BSS2OBJ) bss2obj
	rm -f $(BCDDUMP) bd
	rm -f $(PATCHDASD) patchdasd
	rm -f $(BCDSPLIT) bsplit
	rm -f $(BINCMP) bincmp
	rm -f $(DISASM) disasm
	rm -f $(GELBL2TXT) gelbl2txt
	rm -f $(MKDASD) mkdasd
	rm -f $(BCD2TXTOBJ) bcd2txt
	rm -f $(BCD2BSSOBJ) bcd2bss
	rm -f $(TXT2BCDOBJ) txt2bcd
	rm -f $(GENDATEOBJ) gendate
	rm -f $(GENEXTOBJ) genext
	rm -f $(STRIPBSSOBJ) stripbss
	rm -f $(PUNCHCTSSOBJ) punchctss
	rm -f $(PLOTCVTOBJ) plotcvt
	rm -f $(USERADDOBJ) useraddctss
	rm -f $(USERDELOBJ) userdelctss
	rm -f $(USERREPORTOBJ) userreportctss

install:
	cp $(TARGETS) $(INSTDIR)

obj2cbn.o: obj2cbn.c sysdef.h objdef.h
obj2bin.o: obj2bin.c sysdef.h objdef.h parity.h
obj2img.o: obj2img.c sysdef.h objdef.h parity.h tobcd.h octdump.h
bd.o: bd.c nativebcd.h dasd.h
bsplit.o: bsplit.c 
bincmp.o: bincmp.c sysdef.h
disasm.o: disasm.c sysdef.h nativebcd.h
gelbl2txt.o: gelbl2txt.c sysdef.h nativebcd.h
mkdasd.o: mkdasd.c sysdef.h dasd.h
bcd2txt.o: bcd2txt.c sysdef.h nativebcd.h prsf2.h
bcd2bss.o: bcd2bss.c sysdef.h 
bss2obj.o: bss2obj.c sysdef.h nativebcd.h prsf2.h octdump.h
txt2bcd.o: txt2bcd.c sysdef.h prsf2.h nativebcd.h parity.h tobcd.h
prsf2.o: prsf2.c prsf2.h
gendate.o: gendate.c
genext.o: genext.c
patchdasd.o: patchdasd.c sysdef.h nativebcd.h dasd.h octdump.h
punchctss.o: punchctss.c sysdef.h nativebcd.h octdump.h
plotcvt.o: plotcvt.c sysdef.h nativebcd.h octdump.h
calcomp.o: calcomp.c 
useraddctss.o: useraddctss.c timacc.h usersupport.h
userdelctss.o: userdelctss.c timacc.h usersupport.h
userreportctss.o: userreportctss.c timacc.h usersupport.h
usersupport.o: usersupport.c timacc.h usersupport.h
