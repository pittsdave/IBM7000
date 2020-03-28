.SUFFIXES: .asm .obj .lst

CC = gcc
TARGETS = asm7070
INSTDIR = /usr/local/bin

ASMOBJS = asm7070.o asmpass0.o asmpass1.o asmpass2.o asmoptab.o asmsupt.o \
	parser.o asmdmem.o

TESTSRC = hello.asm bottles.asm bottles-lp.asm printcard.asm cardcopy.asm \
	test.asm
TESTOBJ = hello.obj bottles.obj bottles-lp.obj printcard.obj cardcopy.obj \
	test.obj

ASMFLAGS = $(DEBUG) -xl $(subst obj,lst,$@)

.asm.obj:
	asm7070 $(ASMFLAGS) -o $@ $<

all: 
	@if [ "`uname -s`" = "Linux" ] ; then \
		echo "Making Linux on a `uname -m`" ;\
		make $(TARGETS) "CFLAGS=-DUNIX -DLINUX $(DEBUG)" ;\
	elif [ "`uname -s`" = "AIX" ] ; then \
		echo "Making AIX" ;\
		make $(TARGETS) "CFLAGS=-DUNIX -DAIX $(DEBUG)" ;\
	elif [ "`uname -s`" = "HP-UX" ] ; then \
		echo "Making HP-UX" ;\
		make $(TARGETS) "CFLAGS=-DUNIX -DHPUX $(DEBUG)" ;\
	elif [ "`uname -s`" = "OS/390" ] ; then \
		echo "Making OS/390 USS" ;\
		make $(TARGETS) "CC=cc" "CFLAGS=-DUNIX -DUSS $(DEBUG)" ;\
	elif [ "`uname -s`" = "SunOS" ] ; then \
		echo "Making Solaris" ;\
		make $(TARGETS) "CFLAGS=-DUNIX -DSOLARIS $(DEBUG)" ;\
        elif [ "`uname -s | cut -c1-7`" = "MINGW32" ] ; then \
                echo "Making `uname -s` with MSYS" ;\
                make $(TARGETS) "CFLAGS=-DWIN32 -DMINGW $(DEBUG)" ;\
	else \
		echo "OS type `uname -s` is unknown" ;\
		echo "You must enter an OS type. OS types are:" ;\
		echo "   aix | decunix | hpux | linux | nt | openmvs | solaris" ;\
		echo " " ;\
	fi

aix:
	@make $(TARGETS) "CFLAGS = -DUNIX -DAIX $(DEBUG)"

hpux:
	@make $(TARGETS) "CFLAGS = -DUNIX -DHPUX $(DEBUG)"

linux:
	@make $(TARGETS) "CFLAGS = -DUNIX -DLINUX $(DEBUG)"

nt:
	@nmake/nologo -f makefile.nt

openmvs:
	@make $(TARGETS) "CFLAGS = -DUNIX -DUSS $(DEBUG)"

solaris:
	@make $(TARGETS) "CFLAGS = -DUNIX -DSOLARIS $(DEBUG)"



asm7070: $(ASMOBJS)
	$(CC) -o asm7070 $(ASMOBJS)

asm7070.tok asm7070.err asm7070.ptb asm7070.sem : asm7070.bnf
	chat -l asm7070.bnf
	@mv asm7070.err t.t
	@sed -e 's/fprintf/strcpy/;s/stderr/errorstring/' <t.t >asm7070.err
	@rm t.t

cleantest:
	rm -f *.obj *.bin *.lst 

clean: cleantest
	rm -f $(ASMOBJS) asm7070
	rm -f *.trc core

install:
	cp $(TARGETS) $(INSTDIR)

test: $(TESTOBJ)

asm7070.o: asm7070.c asmdef.h asm7070.tok asmdmem.h
asmpass0.o: asmpass0.c asmdef.h asm7070.tok asmdmem.h
asmpass1.o: asmpass1.c asmdef.h asm7070.tok
asmpass2.o: asmpass2.c asmdef.h asmbcd.h asm7070.tok
asmoptab.o: asmoptab.c asmdef.h asm7070.tok asmdmem.h
asmsupt.o: asmsupt.c asmdef.h asm7070.tok asmdmem.h
parser.o: parser.c errors.h stables.h scanner.h asmdef.h asm7070.ptb \
	asm7070.sem asmdmem.h
asmdmem.o: asmdmem.c asmdef.h asmdmem.h
