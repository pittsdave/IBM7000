.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

EXES = admin.exe apenda.exe archv.exe combin.exe comfil.exe delfib.exe \
	dial.exe edc.exe edl.exe extbss.exe
IMAGES = admin.img apenda.img archv.img combin.img comfil.img delfib.img \
	dial.img edc.img edl.img extbss.img
TAPES = com1.tap

LIBS = -L../tslib1
XLIBS = -L../xlibe

ADMINOBJS = admin.mdo adutil.obj deltem.obj 
APENDAOBJS = apenda.mdo archa.mdo daytim.obj dumio.obj
ARCHVOBJS = archv.mdo ../asclib/nsctss.obj
CHMODEOBJS = chmode.mdo getmd.obj 
COMBINOBJS = combin.mdo
COMFILOBJS = comfil.obj
DELFIBOBJS = delfib.mdo
DIALOBJS = dial.obj ../com3/gtprob.obj
EDOBJS = ed.obj settab.obj edutl.obj ediop.obj nclib.obj
EDCOBJS = edc.mdo binval.mdo concd.mdo ../com5/typ6.mdo \
	bin.obj bufdsk.obj buftty.obj clrnam.obj libr.obj \
	rename.obj
EDLOBJS = edl.mdo edlval.mdo pack.obj ../com5/typ6.mdo \
	bin.obj bufdsk.obj buftty.obj clrnam.obj libr.obj \
	rename.obj

EXTBSSOBJS = extbss.obj rdbss.obj svbss.obj wrbss.obj

OBJS = adutil.obj bin.obj blip.obj bufdsk.obj buftty.obj calls.obj clrnam.obj \
	comfil.obj crud.obj daytim.obj delete.obj deltem.obj dial.obj \
	dumio.obj ed.obj ediop.obj edutl.obj extbss.obj getmd.obj libr.obj \
	nclib.obj pack.obj prnter.obj rdbss.obj rename.obj settab.obj \
	svbss.obj wrbss.obj

all: $(OBJS) $(EXES) $(TAPES)

clean:
	rm -f $(OBJS) *.lst
	rm -f $(EXES) $(IMAGES) *.map *.tap

admin.exe: $(ADMINOBJS)
	$(LNK7090) -wxdmo ADMIN admin.map $@ $(ADMINOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f ADMN8A -o admin.img $@

apenda.exe: $(APENDAOBJS)
	$(LNK7090) -wxdmo APENDA apenda.map $@ $(APENDAOBJS) $(LIBS)
	$(OBJ2IMG) -f APENDA -o apenda.img $@

archv.exe: $(ARCHVOBJS)
	$(LNK7090) -wxdmo ARCHV archv.map $@ $(ARCHVOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f ARCHIV -o archv.img $@

chmode.exe: $(CHMODEOBJS)
	$(LNK7090) -wxdmo CHMODE chmode.map $@ $(CHMODEOBJS) $(LIBS)
	$(OBJ2IMG) -f DELETE -o chmode.img $@

combin.exe: $(COMBINOBJS)
	$(LNK7090) -Cwxdmo COMBIN combin.map $@ $(COMBINOBJS) $(LIBS)
	$(OBJ2IMG) -f COMBIN -o combin.img $@

comfil.exe: $(COMFILOBJS)
	$(LNK7090) -wxdmo COMFIL comfil.map $@ $(COMFILOBJS) $(LIBS)
	$(OBJ2IMG) -f COMFIL -o comfil.img $@

delfib.exe: $(DELFIBOBJS)
	$(LNK7090) -wxdmo DELFIB delfib.map $@ $(DELFIBOBJS) $(LIBS)
	$(OBJ2IMG) -f DELFIB -o delfib.img $@

dial.exe: $(DIALOBJS)
	$(LNK7090) -wxdmo DIAL dial.map $@ $(DIALOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f DIAL8A -o dial.img $@

ed.exe: $(EDOBJS)
	$(LNK7090) -wxdmo ED ed.map $@ $(EDOBJS) $(LIBS)
	$(OBJ2IMG) -f ED -o ed.img $@

edc.exe: $(EDCOBJS)
	$(LNK7090) -wxdmo EDC edc.map $@ $(EDCOBJS) $(LIBS)
	$(OBJ2IMG) -f EDC -o edc.img $@

edl.exe: $(EDLOBJS)
	$(LNK7090) -wxdmo EDL edl.map $@ $(EDLOBJS) $(LIBS)
	$(OBJ2IMG) -f EDL -o edl.img $@

extbss.exe: $(EXTBSSOBJS)
	$(LNK7090) -Cwxdmo EXTBSS extbss.map $@ $(EXTBSSOBJS) $(LIBS)
	$(OBJ2IMG) -f EXTBSS -o extbss.img $@

com1.tap : $(EXES)
	cat $(IMAGES) ../EOF.BIN >$@

extbss.obj: extbss.fap
	$(ASM7090) -C $(ASMFLAGS) -o $@ extbss.fap
rdbss.obj: rdbss.fap
	$(ASM7090) -C $(ASMFLAGS) -o $@ rdbss.fap
svbss.obj: svbss.fap
	$(ASM7090) -C $(ASMFLAGS) -o $@ svbss.fap
wrbss.obj: wrbss.fap
	$(ASM7090) -C $(ASMFLAGS) -o $@ wrbss.fap

