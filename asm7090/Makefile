
CC = gcc
TARGETS = asm7090
INSTDIR = /usr/local/bin

ASMOBJS = asm7090.o asmpass0.o asmpass1.o asmpass2.o asmoptab.o asmsupt.o \
	parser.o asmfps.o asmdmem.o

all: 
	@if [ "`uname -s`" = "Linux" ] ; then \
		echo "Making Linux on a `uname -m`" ;\
		make $(TARGETS) "CFLAGS=-DUNIX -DLINUX $(DEBUG)" ;\
	elif [ "`uname -s`" = "AIX" ] ; then \
		echo "Making AIX" ;\
		make $(TARGETS) "CFLAGS=-DUNIX -DAIX $(DEBUG)" ;\
	elif [ "`uname -s`" = "Darwin" ] ; then \
		echo "Making OSX" ;\
		make $(TARGETS) "CFLAGS=-DUNIX -DDARWIN $(DEBUG)" ;\
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
		echo "   aix | darwin | hpux | linux | nt | openmvs | solaris" ;\
		echo " " ;\
	fi

aix:
	@make $(TARGETS) "CFLAGS = -DUNIX -DAIX $(DEBUG)"

darwin:
	@make $(TARGETS) "CFLAGS = -DUNIX -DDARWIN $(DEBUG)"

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



asm7090: $(ASMOBJS)
	$(CC) -o asm7090 $(ASMOBJS)

asm7090.tok asm7090.err asm7090.ptb asm7090.sem : asm7090.bnf
	chat -l asm7090.bnf
	@mv asm7090.err t.t
	@sed -e 's/fprintf/strcpy/;s/stderr/errorstring/' <t.t >asm7090.err
	@rm t.t

clean:
	rm -f $(ASMOBJS) asm7090
	rm -f *.obj *.bin *.lst *.trc

distclean: clean
	-rm -f tags cscope*

tags: *.c *.h
	@ctags *.c *.h

cscope: *.c *.h
	@cscope -u -b -k -q *.c *.h

install:
	cp $(TARGETS) $(INSTDIR)

asm7090.o: asm7090.c asmdef.h asm7090.tok asmdmem.h asmsymbols.h
asmpass0.o: asmpass0.c asmdef.h asm7090.tok asmdmem.h asmmacros.h
asmpass1.o: asmpass1.c asmdef.h asm7090.tok
asmpass2.o: asmpass2.c asmdef.h asmbcd.h asm7090.tok
asmoptab.o: asmoptab.c asmdef.h asm7090.tok asmdmem.h
asmsupt.o: asmsupt.c asmdef.h asm7090.tok asmdmem.h
parser.o: parser.c errors.h stables.h scanner.h asmdef.h asm7090.ptb \
	asm7090.sem asmfps.h asmnums.h asmdmem.h scanfuncs.h
asmfps.o: asmfps.c asmdef.h asmfps.h
asmdmem.o: asmdmem.c asmdef.h asmdmem.h
