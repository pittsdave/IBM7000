.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img


ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

TAPES = ctss.tap setup.tap extract.tap

CTSSSRCS = main8c.fap utrp8a.fap rtrn8e.fap pdpa8b.fap cloc8h.fap pmti8a.fap \
	conv8a.fap tcor8b.fap scda8c.fap scdb8a.fap scdc8c.fap scdd8a.fap \
	ibuf8b.fap chne8a.fap bksv8c.fap brkp8d.fap cpyc8b.fap util8c.fap \
	wrfl8b.fap dump8b.fap comc8g.fap ctrl8f.fap edbg8a.fap savc8c.fap \
	savr8b.fap adpi8a.fap ulib8h.fap ulod8a.fap fint8b.fap unit8b.fap \
	ap758a.fap wrhi8a.fap tsto8b.fap high8a.fap onln8f.fap com800.fap

CTSSSRCI = main8c.img utrp8a.img rtrn8e.img pdpa8b.img cloc8h.img pmti8a.img \
	conv8a.img tcor8b.img scda8c.img scdb8a.img scdc8c.img scdd8a.img \
	ibuf8b.img chne8a.img bksv8c.img brkp8d.img cpyc8b.img util8c.img \
	wrfl8b.img dump8b.img comc8g.img ctrl8f.img edbg8a.img savc8c.img \
	savr8b.img adpi8a.img ulib8h.img ulod8a.img fint8b.img unit8b.img \
	ap758a.img wrhi8a.img tsto8b.img high8a.img onln8f.img com800.img \
	common.img equ.img macro.img t2char.img tachar.img \
	twchar.img txchar.img mkctss.img mit8c0.img

CTSSOBJS = main8c.obj utrp8a.obj rtrn8e.obj pdpa8b.obj cloc8h.obj pmti8a.obj \
	conv8a.obj tcor8b.obj scda8c.obj scdb8a.obj scdc8c.obj scdd8a.obj \
	ibuf8b.obj chne8a.obj bksv8c.obj brkp8d.obj cpyc8b.obj util8c.obj \
	wrfl8b.obj dump8b.obj comc8g.obj ctrl8f.obj edbg8a.obj savc8c.obj \
	savr8b.obj adpi8a.obj ulib8h.obj ulod8a.obj fint8b.obj unit8b.obj \
	ap758a.obj wrhi8a.obj tsto8b.obj high8a.obj onln8f.obj \
	../fsa4d/qman2b.obj ../fsa4d/tpap2h.obj ../fsa4d/fcor2o.obj \
	../fsa4d/bucm2e.obj ../fsa4d/ddst2i.obj ../fsa4d/ddap4c.obj \
	../fsa4d/tpsm2q.obj ../fsa4d/iocm2e.obj ../fsa4d/sear2b.obj \
	../fsa4d/srch2a.obj ../fsa4d/tman4a.obj ../fsa4d/push2a.obj \
	../fsa4d/scnd4a.obj ../fsa4d/stik2e.obj ../fsa4d/trac2a.obj

EXTRACTOBJS = extract.obj extfil.obj utrp8a.obj rtrn8e.obj pdpa8b.obj \
	cloc8h.obj pmti8a.obj conv8a.obj tcor8b.obj scda8c.obj scdb8a.obj \
	scdc8c.obj scdd8a.obj ibuf8b.obj bksv8c.obj brkp8d.obj cpyc8b.obj \
	util8c.obj wrfl8b.obj dump8b.obj comc8g.obj ctrl8f.obj edbg8a.obj \
	savc8c.obj savr8b.obj ulib8h.obj ulod8a.obj fint8b.obj unit8b.obj \
	tsto8b.obj onln8f.obj ../lib1/bbld.obj \
	../fsa4d/qman2b.obj ../fsa4d/tpap2h.obj ../fsa4d/fcor2o.obj \
	../fsa4d/bucm2e.obj ../fsa4d/ddst2i.obj ../fsa4d/ddap4c.obj \
	../fsa4d/tpsm2q.obj ../fsa4d/iocm2e.obj ../fsa4d/sear2b.obj \
	../fsa4d/srch2a.obj ../fsa4d/tman4a.obj ../fsa4d/push2a.obj \
	../fsa4d/scnd4a.obj ../fsa4d/stik2e.obj ../fsa4d/trac2a.obj

SETUPOBJS = setup.obj setfil.obj utrp8a.obj rtrn8e.obj pdpa8b.obj cloc8h.obj \
	pmti8a.obj conv8a.obj tcor8b.obj scda8c.obj scdb8a.obj scdc8c.obj \
	scdd8a.obj ibuf8b.obj bksv8c.obj brkp8d.obj cpyc8b.obj util8c.obj \
	wrfl8b.obj dump8b.obj comc8g.obj ctrl8f.obj edbg8a.obj savc8c.obj \
	savr8b.obj ulib8h.obj ulod8a.obj fint8b.obj unit8b.obj tsto8b.obj \
	onln8f.obj ../lib1/bbld.obj \
	../fsa4d/qman2b.obj ../fsa4d/tpap2h.obj ../fsa4d/fcor2o.obj \
	../fsa4d/bucm2e.obj ../fsa4d/ddst2i.obj ../fsa4d/ddap4c.obj \
	../fsa4d/tpsm2q.obj ../fsa4d/iocm2e.obj ../fsa4d/sear2b.obj \
	../fsa4d/srch2a.obj ../fsa4d/tman4a.obj ../fsa4d/push2a.obj \
	../fsa4d/scnd4a.obj ../fsa4d/stik2e.obj ../fsa4d/trac2a.obj

SASETUPOBJS = sa_setfil.obj ../s.util/iopa2c.obj ../s.util/ioconv.obj \
        ../s.util/prnter.obj \
	../fsa4d/qman2b.obj ../fsb4d/tpapdm.obj ../fsa4d/fcor2o.obj \
	../fsa4d/bucm2e.obj ../fsa4d/ddst2i.obj ../fsa4d/ddap4c.obj \
	../fsb4d/tpst2a.obj ../fsa4d/iocm2e.obj ../fsa4d/sear2b.obj \
	../fsa4d/srch2a.obj ../fsa4d/tman4a.obj ../fsa4d/push2a.obj \
	../fsa4d/scnd4a.obj ../fsa4d/stik2e.obj ../fsa4d/trac2a.obj

OBJS = adpi8a.obj ap758a.obj bksv8c.obj brkp8d.obj chne8a.obj cloc8h.obj \
	comc8g.obj conv8a.obj cpyc8b.obj ctrl8f.obj dump8b.obj edbg8a.obj \
	fint8b.obj high8a.obj ibuf8b.obj main8c.obj onln8f.obj pdpa8b.obj \
	pmti8a.obj rtrn8e.obj savc8c.obj savr8b.obj scda8c.obj scdb8a.obj \
	scdc8c.obj scdd8a.obj tcor8b.obj tsto8b.obj ulib8h.obj ulod8a.obj \
	unit8b.obj util8c.obj utrp8a.obj wrfl8b.obj wrhi8a.obj setup.obj \
	setfil.obj extract.obj extfil.obj com800.obj

LOADER = ../loader.bin

all: $(OBJS) $(TAPES) coms00.img com800.img

clean:
	rm -f $(OBJS) *.lst
	rm -f $(TAPES) *.map *.img *.exe *.bin
	rm -f mkctss.runcom source.tap
	rm -f sa_setup.tap sa_setfil.obj

ctss.tap: $(CTSSOBJS) $(LOADER)
	$(LNK7090) -wxdmo CTSS ctss.map ctss.exe $(CTSSOBJS)
	$(OBJ2BIN) -l 64 ctss.exe ctss.bin
	cat $(LOADER) ctss.bin ../EOF.BIN >$@

extract.tap: $(EXTRACTOBJS) $(LOADER)
	$(LNK7090) -wxdmo EXTRCT extract.map extract.exe $(EXTRACTOBJS)
	$(OBJ2BIN) -l 64 extract.exe extract.bin
	cat $(LOADER) extract.bin ../EOF.BIN >$@

setup.tap: $(SETUPOBJS) $(LOADER)
	$(LNK7090) -wxdmo SETUP setup.map setup.exe $(SETUPOBJS)
	$(OBJ2BIN) -l 64 setup.exe setup.bin
	cat $(LOADER) setup.bin ../EOF.BIN >$@

sa_setup.tap: $(SASETUPOBJS) $(LOADER)
	$(LNK7090) -wxdmo SASET sa_setup.map sa_setup.exe $(SASETUPOBJS)
	$(OBJ2BIN) -l 64 sa_setup.exe sa_setup.bin
	cat $(LOADER) sa_setup.bin ../EOF.BIN >$@

coms00.img: com800.fap
	$(ASM7090) -sI ../include -wc CTSS -lo com800.lst com800.obj com800.fap
	$(OBJ2IMG) -s -a " M1416CMFL04" -f COMS00 -o coms00.img com800.obj

com800.img: com800.symtb
	$(OBJ2IMG) -t -a " M1416CMFL04" -f COM800 -e SYMTB -o com800.img com800.symtb

source.tap: $(CTSSSRCS) ../include/common.inc ../include/equ.inc \
	../include/macro.inc ../include/t2char.inc ../include/tachar.inc \
	../include/twchar.inc ../include/txchar.inc
	ls -1 $(CTSSSRCS) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -t -f common -e fap -a " M1416PITTS " -o common.img ../include/common.inc
	$(OBJ2IMG) -t -f equ -e fap -a " M1416PITTS " -o equ.img ../include/equ.inc
	$(OBJ2IMG) -t -f macro -e fap -a " M1416PITTS " -o macro.img ../include/macro.inc
	$(OBJ2IMG) -t -f t2char -e fap -a " M1416PITTS " -o t2char.img ../include/t2char.inc
	$(OBJ2IMG) -t -f tachar -e fap -a " M1416PITTS " -o tachar.img ../include/tachar.inc
	$(OBJ2IMG) -t -f twchar -e fap -a " M1416PITTS " -o twchar.img ../include/twchar.inc
	$(OBJ2IMG) -t -f txchar -e fap -a " M1416PITTS " -o txchar.img ../include/txchar.inc
	#ls -1 $(CTSSSRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nUPDATE 1 " $$1 " BSS\nDELETE " $$1 " BSS\nDELETE " $$1 " SYMTB" }' >mkctss.runcom
	ls -1 $(CTSSSRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >mkctss.runcom
	#echo "UPDATE 1 MIT8C0 CTSSLD" >>mkctss.runcom
	echo "FAP COM800 (FLAG) NO" >>mkctss.runcom
	#echo "UPDATE 4 COM800 SYMTB" >>mkctss.runcom
	$(OBJ2IMG) -t -f mkctss -e runcom -a " M1416PITTS " -o mkctss.img mkctss.runcom
	$(OBJ2IMG) -t -f mit8c0 -e ctssld -a " M1416PITTS " -o mit8c0.img mit8c0.ctssld
	cat $(CTSSSRCI) ../EOF.BIN >source.tap
