.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -C -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

TAPES = lib1-1.tap lib1-2.tap

IMAGES1 = 01300.img 01301.img 01311.img 03310.img acor.img acos.img actn.img \
	adj0.img akno.img appe.img atan.img attn.img bbld.img bcdc.img \
	bcoc.img brea.img bsf0.img bst0.img btoc.img bwrt.img \
	bz57.img bzel.img chfl.img chmd.img chnc.img cler.img cloc.img \
	cmfl.img cmlc.img \
	macro.img misc.img lib1-1.img 
IMAGES2 = colt.img com0.img coma.img comf.img copy.img cos0.img cot0.img \
	csh0.img defb.img dele.img detc.img dfad.img dim0.img dpnv.img \
	drea.img dskd.img dskr.img dwrt.img eftm.img enco.img eofx.img \
	erro.img exit.img exme.img exp0.img exp1.img exp2.img exp3.img \
	fint.img \
	macro.img misc.img lib1-2.img

SRCS1 = 01300.fap 01301.fap 01311.fap 03310.fap acor.fap acos.fap actn.fap \
	adj0.fap akno.fap appe.fap atan.fap attn.fap bbld.fap bcdc.fap \
	bcoc.fap brea.fap bsf0.fap bst0.fap btoc.fap bwrt.fap \
	bz57.fap bzel.fap chfl.fap chmd.fap chnc.fap cler.fap cloc.fap \
	cmfl.fap cmlc.fap 
SRCS2 = colt.fap com0.fap coma.fap comf.fap copy.fap cos0.fap cot0.fap \
	csh0.fap defb.fap dele.fap detc.fap dfad.fap dim0.fap dpnv.fap \
	drea.fap dskd.fap dskr.fap dwrt.fap eftm.fap enco.fap eofx.fap \
	erro.fap exit.fap exme.fap exp0.fap exp1.fap exp2.fap exp3.fap \
	fint.fap

OBJS = 01300.obj 01301.obj 01311.obj 03310.obj acor.obj acos.obj actn.obj \
	adj0.obj akno.obj appe.obj atan.obj attn.obj bbld.obj bcdc.obj \
	bcoc.obj brea.obj bsf0.obj bst0.obj btoc.obj bwrt.obj \
	bz57.obj bzel.obj chfl.obj chmd.obj chnc.obj cler.obj cloc.obj \
	cmfl.obj cmlc.obj colt.obj com0.obj coma.obj comf.obj copy.obj \
	cos0.obj cot0.obj csh0.obj defb.obj dele.obj detc.obj dfad.obj \
	dim0.obj dpnv.obj drea.obj dskd.obj dskr.obj dwrt.obj eftm.obj \
	enco.obj eofx.obj erro.obj exit.obj exme.obj exp0.obj exp1.obj \
	exp2.obj exp3.obj fint.obj

all: $(OBJS)
	cp $(OBJS) ../tslib1/

clean:
	rm -f $(OBJS) *.lst lib1.runcom
	rm -f $(TAPES) $(IMAGES1) $(IMAGES2)

lib1-1.tap: $(SRCS1) ../include/macro.inc ../include/misc.inc
	ls -1 $(SRCS1) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -t -f macro -e fap -a " M1416PITTS " -o macro.img ../include/macro.inc
	$(OBJ2IMG) -t -f misc -e fap -a " M1416PITTS " -o misc.img ../include/misc.inc
	ls -1 $(SRCS1) $(SRCS2) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >lib1.runcom
	$(OBJ2IMG) -t -f lib11 -e runcom -a " M1416PITTS " -o lib1-1.img lib1.runcom
	cat $(IMAGES1) ../EOF.BIN >lib1-1.tap

lib1-2.tap: $(SRCS2) ../include/macro.inc ../include/misc.inc
	ls -1 $(SRCS2) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -t -f macro -e fap -a " M1416PITTS " -o macro.img ../include/macro.inc
	$(OBJ2IMG) -t -f misc -e fap -a " M1416PITTS " -o misc.img ../include/misc.inc
	ls -1 $(SRCS2) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >lib1.runcom
	$(OBJ2IMG) -t -f lib12 -e runcom -a " M1416PITTS " -o lib1-2.img lib1.runcom
	cat $(IMAGES2) ../EOF.BIN >lib1-2.tap
