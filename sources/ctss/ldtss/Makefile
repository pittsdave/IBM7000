.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<


SRCS = loadts.fap 
OBJS = loadts.obj 
EXES = loadts.exe

LOADTSOBJS = loadts.obj ../s.util/iopb2c.obj ../s.util/ioconv.obj \
	../s.util/prnter.obj ../fsb4d/fcor2o.obj ../fsb4d/sear2b.obj \
	../fsb4d/srch2a.obj ../fsb4d/scnd4a.obj ../fsb4d/tracb.obj \
	../fsb4d/stik2e.obj ../fsb4d/push2a.obj ../fsb4d/bucm2e.obj \
	../fsb4d/iocm2e.obj ../fsb4d/qman2b.obj ../fsb4d/ddst2i.obj \
	../fsb4d/tman4a.obj ../fsb4d/ddapb.obj ../fsb4d/tpst2a.obj \
	../fsb4d/tpapdm.obj

all: $(OBJS) $(EXES)

loadts.exe:
	$(LNK7090) -wxdmo LOADER loadts.map loadts.exe $(LOADTSOBJS)
	$(OBJ2IMG) -f LOADER -e SAVED -a " M1416PITTS " -o loadts.img $@

clean:
	rm -f $(OBJS) $(EXES) *.lst *.map *.img
	rm -f source.tap mkldtss.runcom

source.tap: $(SRCS)
	ls -1 $(SRCS) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	ls -1 $(SRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO (LIST)" }' >mkldtss.runcom
	echo "NCLOAD LOADTS IOPB2C IOCONV PRNTER FCOR2O SEAR2B SRCH2A SCND4A (MORE)" >>mkldtss.runcom
	echo "USE TRACB STIK2E PUSH2A BUCM2E IOCM2E QMAN2B DDST2I TMAN4A DDAPB (MORE)" >>mkldtss.runcom
	echo "USE TPST2A TPAPDM (NLIB)" >>mkldtss.runcom
	$(OBJ2IMG) -t -f ldtss -e runcom -a " M1416PITTS " -o mkldtss.img mkldtss.runcom
	cat *.img ../EOF.BIN >source.tap
