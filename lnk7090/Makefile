
CC = gcc
TARGETS = lnk7090
INSTDIR = /usr/local/bin

LNKOBJS = lnk7090.o lnkloader.o lnklibrary.o lnksupt.o lnkpunch.o

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



lnk7090: $(LNKOBJS)
	$(CC) -o lnk7090 $(LNKOBJS)

clean:
	rm -f $(LNKOBJS) lnk7090
	rm -f *.bin *.lst

install:
	cp $(TARGETS) $(INSTDIR)

lnk7090.o: lnk7090.c lnkdef.h
lnkloader.o: lnkloader.c lnkdef.h
lnklibrary.o: lnklibrary.c lnkdef.h
lnksupt.o: lnksupt.c lnkdef.h
lnkpunch.o: lnkpunch.c lnkdef.h tobcd.h
