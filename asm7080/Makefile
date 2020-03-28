
CC = gcc
TARGETS = asm7080
INSTDIR = /usr/local/bin

ASMOBJS = asm7080.o asmpass0.o asmpass1.o asmpass2.o asmoptab.o asmsupt.o \
	parser.o asmdmem.o

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



asm7080: $(ASMOBJS)
	$(CC) -o asm7080 $(ASMOBJS)

asm7080.tok asm7080.err asm7080.ptb asm7080.sem : asm7080.bnf
	chat -l asm7080.bnf
	@mv asm7080.err t.t
	@sed -e 's/fprintf/strcpy/;s/stderr/errorstring/' <t.t >asm7080.err
	@rm t.t

clean:
	rm -f $(ASMOBJS) asm7080
	rm -f *.obj *.bin *.lst *.trc core

install:
	cp $(TARGETS) $(INSTDIR)

asm7080.o: asm7080.c asmdef.h asm7080.tok asmdmem.h
asmpass0.o: asmpass0.c asmdef.h asm7080.tok asmdmem.h asmmacros.h
asmpass1.o: asmpass1.c asmdef.h asm7080.tok
asmpass2.o: asmpass2.c asmdef.h asmbcd.h asm7080.tok
asmoptab.o: asmoptab.c asmdef.h asm7080.tok asmdmem.h
asmsupt.o: asmsupt.c asmdef.h asm7080.tok asmdmem.h
parser.o: parser.c errors.h stables.h scanner.h asmdef.h asm7080.ptb \
	asm7080.sem asmdmem.h
asmdmem.o: asmdmem.c asmdef.h asmdmem.h
