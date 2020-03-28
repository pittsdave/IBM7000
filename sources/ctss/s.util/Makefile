.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = $(DEBUG) -I ../include -xwc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

EXES = clock.exe  dsetup.exe  hsdt1a.exe ld7750.exe
TAPES = clock.tap  dsetup.tap  hsdt1a.tap ld7750.tap dg7750.tap

CLOCKOBJS = clock.obj ioconv.obj iopa2c.obj iodef.obj iostub.obj prnter.obj
DSETUPOBJS = dsetup.obj ddio4c.obj ddpac.obj
HSDT1AOBJS = hsdt1a.obj diag.obj ioconv.obj iodef.obj iopa2c.obj iostub.obj \
	prnter.obj
LD7750OBJS = 7750ld.obj 7750rd.obj 7750ch.obj ioconv.obj iodef.obj iopa2c.obj \
	iostub.obj prnter.obj
DG7750OBJS = 7750dg.obj

OBJS = 7750ch.obj 7750dg.obj 7750ld.obj 7750rd.obj b1inp1.obj b1main.obj \
	cdmp1d.obj clock.obj cnvt1d.obj cylod.obj ddio4c.obj ddpac.obj \
	diag.obj dpatch.obj dsetup.obj hsdt1a.obj ioconv.obj iodef.obj \
	iopa2c.obj iopb2c.obj iopc2c.obj iopd2c.obj iope2c.obj iostub.obj \
	list1d.obj mdmp2d.obj opcd1d.obj panic.obj prntbk.obj prnter.obj \
	upck2d.obj

all: $(OBJS) $(TAPES)

clean:
	rm -f $(OBJS) *.lst
	rm -f $(TAPES) $(EXES) *.map *.bin 

hsdt1a.tap: $(HSDT1AOBJS)
	$(LNK7090) -xdmo HSDT1A hsdt1a.map hsdt1a.exe $(HSDT1AOBJS)
	$(OBJ2BIN) -l 64 hsdt1a.exe hsdt1a.bin
	cat ../loader.bin hsdt1a.bin ../EOF.BIN >$@

dsetup.tap: $(DSETUPOBJS)
	$(LNK7090) -xdmo DSETUP dsetup.map dsetup.exe $(DSETUPOBJS)
	$(OBJ2BIN) -l 64 dsetup.exe dsetup.bin
	cat ../loader.bin dsetup.bin ../EOF.BIN >$@

clock.tap: $(CLOCKOBJS)
	$(LNK7090) -xdmo CLOCK clock.map clock.exe $(CLOCKOBJS)
	$(OBJ2BIN) -l 64 clock.exe clock.bin
	cat ../loader.bin clock.bin ../EOF.BIN >$@

ld7750.tap: $(LD7750OBJS)
	$(LNK7090) -dmo LD7750 ld7750.map ld7750.exe $(LD7750OBJS)
	$(OBJ2BIN) -l 64 ld7750.exe ld7750.bin
	cat ../loader.bin ld7750.bin ../EOF.BIN >$@

dg7750.tap: $(DG7750OBJS)
	$(OBJ2BIN) -l 64 7750dg.obj dg7750.bin
	cat ../loader.bin dg7750.bin ../EOF.BIN >$@

