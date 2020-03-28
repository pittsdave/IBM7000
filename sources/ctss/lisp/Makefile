.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

TAPES = lisp.tap progs.tap

LIBS = -L../tslib1
XLIBS = -L../xlibe

LISPOBJS = lisp.obj

LISPSRCS = adv1fe.fap apendf.fap appl1f.fap aray1f.fap aray2f.fap arth1f.fap \
	arthmf.fap atoms.fap blckf2.fap carxzf.fap cnsfwf.fap conszf.fap \
	deconf.fap equlzf.fap evqtzf.fap exptfe.fap fixvlf.fap flonmf.fap \
	gcolef.fap gtgcdf.fap iocnfg.fap iomacs.fap lapzfe.fap lisp.fap \
	prntzf.fap punchf.fap redzif.fap setcnf.fap substf.fap syns.fap \
	temlif.fap zerrrf.fap

LISPSRCI = adv1fe.img apendf.img appl1f.img aray1f.img aray2f.img arth1f.img \
	arthmf.img atoms.img blckf2.img carxzf.img cnsfwf.img conszf.img \
	deconf.img equlzf.img evqtzf.img exptfe.img fixvlf.img flonmf.img \
	gcolef.img gtgcdf.img iocnfg.img iomacs.img lapzfe.img lisp.img \
	prntzf.img punchf.img redzif.img setcnf.img substf.img syns.img \
	temlif.img zerrrf.img

LISPPGMS = fact.lisp ffact.lisp propcal.lisp define.lisp
LISPPGMI = fact.img ffact.img propcal.img define.img

OBJS = $(LISPOBJS)

all: $(OBJS) $(TAPES)

clean:
	rm -f $(OBJS) *.lst
	rm -f $(EXES) *.map *.tap
	rm -f $(LISPSRCI) $(LISPPGMI)

lisp.obj : $(LISPSRCS)

lisp.tap : $(OBJS)
	$(OBJ2IMG) -I -f LISP -o lisp.img lisp.obj
	cat lisp.img ../EOF.BIN >$@

progs.tap : $(LISPPGMS)
	$(OBJ2IMG) -c -t -f fact -e lisp -a " m1416guest " -o fact.img fact.lisp
	$(OBJ2IMG) -c -t -f ffact -e lisp -a " m1416guest " -o ffact.img ffact.lisp
	$(OBJ2IMG) -c -t -f procal -e lisp -a " m1416guest " -o propcal.img propcal.lisp
	$(OBJ2IMG) -c -t -f define -e lisp -a " m1416guest " -o define.img define.lisp
	cat $(LISPPGMI) ../EOF.BIN >$@

source.tap: $(LISPSRCS) fact.lisp
	ls -1 $(LISPSRCS) fact.lisp | awk -F . '{ print "obj2img -c -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	cat $(LISPSRCI) ../EOF.BIN >source.tap
