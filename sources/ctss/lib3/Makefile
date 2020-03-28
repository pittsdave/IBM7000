.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -C -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

TAPES = lib3-1.tap lib3-2.tap

IMAGES1 = rann.img rbin.img rdat.img rdfb.img rdfl.img rdms.img rdtm.img \
	rdwt.img reco.img renm.img rjus.img rsrb.img rstf.img save.img \
	sch0.img schd.img schn.img setc.img sete.img setf.img setp.img \
	setu.img simc.img slee.img sli0.img slo0.img sloc.img snap.img \
	sph0.img spry.img \
	macro.img misc.img lib3-1.img
IMAGES2 = sqrt.img srch.img stb0.img stfl.img stom.img stor.img sypr.img \
	tanh.img timr.img tlck.img tran.img trfl.img tsh0.img tssf.img \
	updm.img vrea.img wait.img whoa.img wrfl.img wrmt.img xdet.img \
	xdim.img xecm.img xfix.img xloc.img xmax.img xmin.img xmod.img \
	zel0.img \
	macro.img misc.img lib3-2.img

SRCS1 = rann.fap rbin.fap rdat.fap rdfb.fap rdfl.fap rdms.fap rdtm.fap \
	rdwt.fap reco.fap renm.fap rjus.fap rsrb.fap rstf.fap save.fap \
	sch0.fap schd.fap schn.fap setc.fap sete.fap setf.fap setp.fap \
	setu.fap simc.fap slee.fap sli0.fap slo0.fap sloc.fap snap.fap \
	sph0.fap spry.fap 
SRCS2 = sqrt.fap srch.fap stb0.fap stfl.fap stom.fap stor.fap sypr.fap \
	tanh.fap timr.fap tlck.fap tran.fap trfl.fap tsh0.fap tssf.fap \
	updm.fap vrea.fap wait.fap whoa.fap wrfl.fap wrmt.fap xdet.fap \
	xdim.fap xecm.fap xfix.fap xloc.fap xmax.fap xmin.fap xmod.fap \
	zel0.fap

OBJS = rann.obj rbin.obj rdat.obj rdfb.obj rdfl.obj rdms.obj rdtm.obj \
	rdwt.obj reco.obj renm.obj rjus.obj rsrb.obj rstf.obj save.obj \
	sch0.obj schd.obj schn.obj setc.obj sete.obj setf.obj setp.obj \
	setu.obj simc.obj slee.obj sli0.obj slo0.obj sloc.obj snap.obj \
	sph0.obj spry.obj sqrt.obj srch.obj stb0.obj stfl.obj stom.obj \
	stor.obj sypr.obj tanh.obj timr.obj tlck.obj tran.obj trfl.obj \
	tsh0.obj tssf.obj updm.obj vrea.obj wait.obj whoa.obj wrfl.obj \
	wrmt.obj xdet.obj xdim.obj xecm.obj xfix.obj xloc.obj xmax.obj \
	xmin.obj xmod.obj zel0.obj

all: $(OBJS)
	cp $(OBJS) ../tslib1/

clean:
	rm -f $(OBJS) *.lst lib3.runcom
	rm -f $(TAPES) $(IMAGES1) $(IMAGES2)

lib3-1.tap: $(SRCS1) ../include/macro.inc ../include/misc.inc
	ls -1 $(SRCS1) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -t -f macro -e fap -a " M1416PITTS " -o macro.img ../include/macro.inc
	$(OBJ2IMG) -t -f misc -e fap -a " M1416PITTS " -o misc.img ../include/misc.inc
	ls -1 $(SRCS1) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >lib3.runcom
	$(OBJ2IMG) -t -f lib3 -e runcom -a " M1416PITTS " -o lib3-1.img lib3.runcom
	cat $(IMAGES1) ../EOF.BIN >lib3-1.tap

lib3-2.tap: $(SRCS2) ../include/macro.inc ../include/misc.inc
	ls -1 $(SRCS2) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -t -f macro -e fap -a " M1416PITTS " -o macro.img ../include/macro.inc
	$(OBJ2IMG) -t -f misc -e fap -a " M1416PITTS " -o misc.img ../include/misc.inc
	ls -1 $(SRCS2) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >lib3.runcom
	$(OBJ2IMG) -t -f lib3 -e runcom -a " M1416PITTS " -o lib3-2.img lib3.runcom
	cat $(IMAGES2) ../EOF.BIN >lib3-2.tap
