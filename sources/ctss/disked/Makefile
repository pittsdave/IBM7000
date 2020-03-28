.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2IMG = obj2img
OBJ2BIN = obj2bin

ASMFLAGS = $(DEBUG) -C -I ./ -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

LIBS = -L../tslib1

TAPES = dskedt.tap

FIMGS = 03200.img cddelt.img chrg.b.img chrg.s.img deccnv.img derbc.img \
	edtio.img err.img extbkg.img filsys.img flip.img getape.img gett.img \
	iou.img packfp.img privl7.img privlz.img rcard.img scan.img setup.img \
	sprdoc.img stmarg.img unpack.img untab.img
MIMGS = adjprb.img bpnch.img dpnch.img dskedt.img input.img \
	mfddlt.img pnch7.img scnmf1.img scnmfd.img comdsk.img sw.img
IMAGES = $(FIMGS) $(MIMGS) core.img

FSRCS = 03200.fap cddelt.fap chrg.b.fap chrg.s.fap deccnv.fap derbc.fap \
	edtio.fap err.fap extbkg.fap filsys.fap flip.fap getape.fap gett.fap \
	iou.fap packfp.fap privl7.fap privlz.fap rcard.fap scan.fap setup.fap \
	sprdoc.fap stmarg.fap unpack.fap untab.fap
MSRCS = adjprb.mad bpnch.mad dpnch.mad dskedt.mad input.mad \
	mfddlt.mad pnch7.mad scnmf1.mad scnmfd.mad

SRCS = $(FSRCS) $(MSRCS)

OBJS = 03200.obj cddelt.obj chrg.b.obj chrg.s.obj deccnv.obj derbc.obj \
	err.obj extbkg.obj filsys.obj getape.obj gett.obj \
	iou.obj packfp.obj privl7.obj privlz.obj scan.obj setup.obj \
	sprdoc.obj stmarg.obj unpack.obj untab.obj

AOBJS = edtioa.obj flipa.obj rcarda.obj
BOBJS = edtiob.obj flipb.obj rcardb.obj

EDTBSSOBJS = input.mdo dpnch.mdo bpnch.mdo pnch7.mdo mfddlt.mdo packfp.obj \
	unpack.obj untab.obj cddelt.obj scan.obj getape.obj stmarg.obj \
	adjprb.mdo sprdoc.obj deccnv.obj gett.obj derbc.obj iou.obj err.obj \
	03200.obj

SADSKEDTOBJS = dskedt.mdo $(EDTBSSOBJS) scnmf1.mdo privl7.obj $(AOBJS) \
	chrg.s.obj setup.obj ../lib2/ljus.obj \
	../s.util/iope2c.obj ../s.util/ioconv.obj ../s.util/prnter.obj \
	../fsa4d/fcor2o.obj ../fsa4d/sear2b.obj ../fsa4d/srch2a.obj \
	../fsa4d/scnd4a.obj ../fsa4d/trac2a.obj ../fsa4d/stik2e.obj \
	../fsa4d/push2a.obj ../fsa4d/bucm2e.obj ../fsa4d/iocm2e.obj \
	../fsa4d/qman2b.obj ../fsa4d/ddst2i.obj ../fsa4d/tman4a.obj \
	../fsa4d/ddap4c.obj ../fsb4d/tpst2a.obj ../fsb4d/tpapdm.obj

DSKEDTOBJS = dskedt.mdo $(EDTBSSOBJS) scnmfd.mdo privl7.obj $(BOBJS) \
	chrg.b.obj filsys.obj extbkg.obj

EXES = sadskedt.exe dskedt.exe

all: $(OBJS) $(EXES) $(TAPES)

edtioa.obj: edtio.fap
	cp corea.inc core.inc
	$(ASM7090) $(ASMFLAGS) -o $@ edtio.fap

flipa.obj: flip.fap
	cp corea.inc core.inc
	$(ASM7090) $(ASMFLAGS) -o $@ flip.fap

rcarda.obj: rcard.fap
	cp corea.inc core.inc
	$(ASM7090) $(ASMFLAGS) -o $@ rcard.fap

edtiob.obj: edtio.fap
	cp coreb.inc core.inc
	$(ASM7090) $(ASMFLAGS) -o $@ edtio.fap

flipb.obj: flip.fap
	cp coreb.inc core.inc
	$(ASM7090) $(ASMFLAGS) -o $@ flip.fap

rcardb.obj: rcard.fap
	cp coreb.inc core.inc
	$(ASM7090) $(ASMFLAGS) -o $@ rcard.fap

sadskedt.exe: $(SADSKEDTOBJS)
	$(LNK7090) -Cwxdmo DSKEDT sadskedt.map $@ $(SADSKEDTOBJS)

dskedt.tap: sadskedt.exe
	$(OBJ2BIN) -l 64 sadskedt.exe sadskedt.bin
	cat ../loader.bin sadskedt.bin ../EOF.BIN >$@

dskedt.exe: $(DSKEDTOBJS)
	$(LNK7090) -Cwxdmo DSKEDT dskedt.map $@ $(DSKEDTOBJS) $(LIBS)
	$(OBJ2IMG) -f DSKEDT -e SAVED -a " M1416ADMIN " -o dskedt.img $@

clean:
	rm -f $(OBJS) $(AOBJS) $(BOBJS) *.lst dskedt.runcom
	rm -f $(TAPES) $(IMAGES) dskedt_r.img source.tap core.inc
	rm -f $(EXES) sadskedt.bin sadskedt.map dskedt.map sadskedt.img 

source.tap: $(SRCS) comdsk.mad sw.mad
	@echo -n >/tmp/fapimg.sh
	@echo -n >dskedt.runcom
	for i in $(FSRCS); do \
	   j=`basename $$i .fap` ; \
	   echo $$j $$i | awk '{ print "obj2img -t -f " $$1  " -e FAP -a \" m1416pitts \" -o " $$1 ".img " $$2 }' >>/tmp/fapimg.sh ; \
	done
	ls -1 $(MSRCS) | awk -F . '{ print "obj2img -c -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >>/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -c -t -f "COMDSK" -e mad -a " M1416PITTS " -o comdsk.img comdsk.mad
	$(OBJ2IMG) -c -t -f ".SW." -e mad -a " M1416PITTS " -o sw.img sw.mad
	$(OBJ2IMG) -t -f core -e fap -a " M1416PITTS " -o core.img coreb.inc
	for i in $(FSRCS); do \
	   j=`basename $$i .fap` ; \
	   echo $$j | awk '{ print "FAP " $$1 " (FLAG) NO (LIST)" }' >>dskedt.runcom; \
	done
	ls -1 $(MSRCS) | awk -F . '{ print "MAD " $$1 " (LIST)" }' >>dskedt.runcom
	echo "COMBIN * EDTBSS BSS INPUT DPNCH BPNCH PNCH7 MFDDLT PACKFP UNPACK UNTAB" >>dskedt.runcom
	echo "COMBIN * EDTBSS BSS EDTBSS CDDELT SCAN GETAPE STMARG ADJPRB SPRDOC" >>dskedt.runcom
	echo "COMBIN * EDTBSS BSS EDTBSS DECCNV GETT DERBC IOU ERR 03200" >>dskedt.runcom
	echo "NCLOAD DSKEDT SCNMFD PRIVL7 EDTIO FLIP RCARD CHRG.B FILSYS (MORE)" >>dskedt.runcom
	echo "USE EDTBSS EXTBKG" >>dskedt.runcom
	echo "SAVE DSKEDT" >>dskedt.runcom
	$(OBJ2IMG) -t -f dskedt -e runcom -a " M1416PITTS " -o dskedt_r.img dskedt.runcom
	cat $(IMAGES) dskedt_r.img ../EOF.BIN >source.tap
