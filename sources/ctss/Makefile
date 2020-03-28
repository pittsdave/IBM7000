#
# This is the Makefile for CTSS. It requires that the cross development 
# environment (asm7090, lnk7090, and utilities) have been installed.
#

# Edit the following for your environment:

TAPEDIR = ../../tapes
S709DIR = ../../s709-2.3.5
KITDIR = ../../ctss.kit
DOSKITDIR = ../../ctss.winkit

OBJ2IMG = obj2img

SCRIPTS = cleanupctss diagctss dskedtctss extractctss installctss plotctss \
	runctss setupctss salvagectss \
	adduserctss deluserctss reportuserctss \
	cleanupctss.cmd diagctss.cmd dskedtctss.cmd fmt.cmd fmtchk.cmd \
	loader.cmd mfd.cmd runctss.cmd salvagectss.cmd utilctss.cmd

DOSSCRIPTS = cleanupctss.bat diagctss.bat dskedtctss.bat extractctss.bat \
	installctss.bat plotctss.bat runctss.bat setupctss.bat salvagectss.bat \
	adduserctss.bat deluserctss.bat reportuserctss.bat \
	cleanupctss.cmd diagctss.cmd dskedtctss.cmd fmt.cmd fmtchk.cmd \
	loader.cmd mfd.cmd runctss.cmd salvagectss.cmd utilctss.cmd

#ALLDIRS = asclib com1 com2 com3 com4 com5 disked fgdaem filsim fsa4d fsb4d \
#	ldtss lib1 lib2 lib3 mit8c0 plibe sadaem salall salsim s.util \
#	tslib2 util xlibe

.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

# CTSS Runtime libraries:

LIBDIRS = asclib lib1 lib2 lib3 xlibe

# CTSS Kernel:

KERNELDIRS = fsa4d mit8c0 cmfl01

# CTSS Commands:

COMDIRS = com3 com1 com2 com4 com5 lisp

# Miscellaneous utilities:

MISCDIRS = fgdaem filsim 

# System utility dirs:

UTILDIRS = s.util util fsb4d ldtss salall disked

# CTSS System dirs:

SYSDIRS = $(LIBDIRS) $(KERNELDIRS) $(COMDIRS) $(UTILDIRS)

# The collected install tapes:

COMTAPES = com1/com1.tap com2/com2.tap com3/com3.tap com4/com4.tap \
	com5/com5.tap
UTILTAPES = com3/comdata.tap com3/quotas.tap mit8c0/ctss.tap \
	mit8c0/setup.tap mit8c0/extract.tap util/util.tap s.util/clock.tap \
	s.util/dsetup.tap s.util/hsdt1a.tap s.util/ld7750.tap \
	s.util/dg7750.tap comlinks.tap fibmon.tap guest.tap \
	cmfl01/cmfl01.tap loader.tap salall/salv4.tap \
	lisp/lisp.tap lisp/progs.tap disked/dskedt.tap
TAPES = $(COMTAPES) $(UTILTAPES)

all: loader.bin
	@for dir in $(SYSDIRS); do \
		$(MAKE) DEBUG=$(DEBUG) -C $$dir; \
	done

everything: loader.bin
	@for dir in $(SYSDIRS) $(MISCDIRS); do \
		$(MAKE) DEBUG=$(DEBUG) -C $$dir; \
	done

coms commands:
	@for dir in $(COMDIRS); do \
		$(MAKE) DEBUG=$(DEBUG) -C $$dir; \
	done

libs library:
	@for dir in $(LIBDIRS); do \
		$(MAKE) DEBUG=$(DEBUG) -C $$dir; \
	done

kernel: loader.bin
	@for dir in $(KERNELDIRS); do \
		$(MAKE) DEBUG=$(DEBUG) -C $$dir; \
	done

utils: loader.bin
	@for dir in $(UTILDIRS); do \
		$(MAKE) DEBUG=$(DEBUG) -C $$dir; \
	done


clean:
	@for dir in $(SYSDIRS) $(MISCDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f tslib1/*
	rm -f make.log alllinks.dat comlinks.tap
	rm -f fibjob.* fibmon.* guest.tap
	rm -f bottle.img cqa1.img hello.img primes.img
	rm -f loader.lst loader.bin loader.obj

distclean: clean
	rm -rf checkdir

kit:
	cp $(TAPES) $(KITDIR)
	cp $(SCRIPTS) $(KITDIR)
	chmod +w $(KITDIR)/loader.tap
	cp README_kit.txt $(KITDIR)/README.txt
	cp Makefile.kit $(KITDIR)/Makefile

doskit:
	cp com1/com1.tap $(DOSKITDIR)/cmd1.tap
	cp com2/com2.tap $(DOSKITDIR)/cmd2.tap
	cp com3/com3.tap $(DOSKITDIR)/cmd3.tap
	cp com4/com4.tap $(DOSKITDIR)/cmd4.tap
	cp com5/com5.tap $(DOSKITDIR)/cmd5.tap
	cp $(UTILTAPES) $(DOSKITDIR)
	chmod +w $(DOSKITDIR)/loader.tap
	cp $(DOSSCRIPTS) $(DOSKITDIR)
	cp README_dos.txt $(DOSKITDIR)/README.txt

loader.bin: loader.obj
	$(OBJ2BIN) -be loader.obj loader.bin

install:
	cat com?/links.dat >alllinks.dat
	$(OBJ2IMG) -l -f LINKS -e DAT -o comlinks.tap alllinks.dat
	touch fibjob.file
	$(OBJ2IMG) -t -f FIBJOB -e FILE -a " C0056FIBMON" -o fibjob.img \
		fibjob.file
	cat fibjob.img EOF.BIN >fibmon.tap
	$(OBJ2IMG) -c -t -f BOTTLE -e MAD -a " M1416GUEST " -o bottle.img \
		bottle.mad
	$(OBJ2IMG) -c -t -f CQA1 -e MAD -a " M1416GUEST " -o cqa1.img \
		cqa1.mad
	$(OBJ2IMG) -c -t -f HELLO -e MAD -a " M1416GUEST " -o hello.img \
		hello.mad
	$(OBJ2IMG) -t -f PRIMES -e MADTRN -a " M1416GUEST " -o primes.img \
		primes.f
	cat bottle.img cqa1.img hello.img primes.img EOF.BIN >guest.tap
	cp $(TAPES) $(TAPEDIR)
	cp $(SCRIPTS) $(S709DIR)
	chmod +w $(TAPEDIR)/loader.tap

setcheck:
	@for dir in $(SYSDIRS); do \
	   if [ "$$dir" != "cmfl01" ]; then \
	      mkdir -p checkdir/$$dir; \
	      cp $$dir/*.obj checkdir/$$dir/; \
	   fi \
	done

check:
	@for dir in $(SYSDIRS); do \
	   if [ "$$dir" != "cmfl01" ]; then \
	      checkit $$dir; \
	   fi \
	done
