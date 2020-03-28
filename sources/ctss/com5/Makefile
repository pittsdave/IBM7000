.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

EXES = mail.exe mount.exe move.exe option.exe p.exe permit.exe prbin.exe \
	prbss.exe prfib.exe print.exe prnter.exe qed.exe revoke.exe rquest.exe \
	runcom.exe runoff.exe sdump.exe squash.exe tapfil.exe ttpeek.exe \
	typset.exe unlink.exe update.exe updbss.exe verify.exe who.exe \
	write.exe xpand.exe
IMAGES = mail.img mount.img move.img option.img p.img permit.img prbin.img \
	prbss.img prfib.img print.img prnter.img qed.img revoke.img rquest.img \
	runcom.img runoff.img sdump.img squash.img tapfil.img ttpeek.img \
	typset.img unlink.img update.img updbss.img verify.img who.img \
	write.img xpand.img
TAPES = com5.tap

LIBS = -L../tslib1
XLIBS = -L../xlibe

MAILOBJS = mail.mdo smail.mdo
MOUNTOBJS = mount.obj
MOVEOBJS = move.obj copy.obj
OPTIONOBJS = option.mdo
POBJS = p.mdo openf.obj openw.obj pntlin.obj ../asclib/nsctss.obj 
PERMITOBJS = permit.mdo
PRBINOBJS = prbin.mdo
PRBSSOBJS = prbss.obj ../com1/rdbss.obj echar.obj
PRFIBOBJS = prfib.mdo
PRINTOBJS = print.mdo paknr.obj unpknr.obj
PRNTROBJS = prnter.obj prntr.obj
QEDOBJS = qedmain.obj qed.obj canon.obj cnvtbl.obj devtbl.obj initdt.obj \
	zcmd.obj
REVOKEOBJS = revoke.mdo
RQUESTOBJS = rquest.mdo cardrq.obj
RUNCOMOBJS = runcom.obj
RUNOFFOBJS = rmain.mdo runoff.mdo adjust.mdo frame1.mdo hdft.mdo numcon.mdo \
	nxchar.mdo settyp.mdo symsto.mdo \
	assign.obj bin.obj frame.obj lmio.obj remap.obj runout.obj rlib.obj
SDUMPOBJS = sdump.obj octdmp.obj
SQUASHOBJS = squash.mdo iocard.mdo typ6.mdo assign.obj clrnam.obj \
	lmio.obj prword.obj
TAPFILOBJS = tapfil.obj delete.obj
TTPEEKOBJS = ttpeek.obj 
TYPSETOBJS = typset.mdo typval.mdo typ6.mdo assign.obj bin.obj clrnam.obj \
	lmio.obj prword.obj rlib.obj
UNLINKOBJS = unlink.mdo newmod.obj
UPDATEOBJS = update.obj copy.obj unique.obj
UPDBSSOBJS = updbss.obj ../com1/rdbss.obj ../com1/svbss.obj ../com1/wrbss.obj \
	delete.obj
VERIFYOBJS = verify.obj 
WHOOBJS = who.mdo ../com3/gtprob.obj
WRITEOBJS = write.obj ../com3/gtprob.obj typ6.mdo
XPANDOBJS = xpand.mdo iocard.mdo typ6.mdo assign.obj clrnam.obj \
	lmio.obj prword.obj

OBJS = assign.obj bin.obj canon.obj cardrq.obj clrnam.obj cnvtbl.obj copy.obj \
	delete.obj devtbl.obj echar.obj fix.obj frame.obj initdt.obj lmio.obj \
	move.obj mount.obj newmod.obj nocmmd.obj octdmp.obj openf.obj \
	openw.obj paknr.obj pntlin.obj prbss.obj prnter.obj prntr.obj \
	prword.obj qed.obj qedmain.obj remap.obj remain.obj rename.obj \
	reprb.obj rlib.obj runcom.obj runout.obj schain.obj sdspmn.obj \
	sdump.obj setpk.obj tapfil.obj ttpeek.obj unique.obj unpknr.obj \
	update.obj updbss.obj verify.obj write.obj wrflx.obj zcmd.obj

all: $(OBJS) $(EXES) $(TAPES)

clean:
	rm -f $(OBJS) *.lst
	rm -f $(EXES) $(IMAGES) *.map *.tap

mail.exe: $(MAILOBJS)
	$(LNK7090) -wxdmo MAIL mail.map $@ $(MAILOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f MAIL -o mail.img $@

mount.exe: $(MOUNTOBJS)
	$(LNK7090) -wxdmo MOUNT mount.map $@ $(MOUNTOBJS) $(LIBS)
	$(OBJ2IMG) -f MOUNT -o mount.img $@

move.exe: $(MOVEOBJS)
	$(LNK7090) -wxdmo MOVE move.map $@ $(MOVEOBJS) $(LIBS)
	$(OBJ2IMG) -f MOVE -o move.img $@

option.exe: $(OPTIONOBJS)
	$(LNK7090) -wxdmo OPTION option.map $@ $(OPTIONOBJS) $(LIBS)
	$(OBJ2IMG) -f OPTION -o option.img $@

p.exe: $(POBJS)
	$(LNK7090) -wxdmo P p.map $@ $(POBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f P -o p.img $@

permit.exe: $(PERMITOBJS)
	$(LNK7090) -wxdmo PERMIT permit.map $@ $(PERMITOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f PERMIT -o permit.img $@

prbin.exe: $(PRBINOBJS)
	$(LNK7090) -Cwxdmo PRBIN prbin.map $@ $(PRBINOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f PRBIN -o prbin.img $@

prbss.exe: $(PRBSSOBJS)
	$(LNK7090) -Cwxdmo PRBSS prbss.map $@ $(PRBSSOBJS) $(LIBS)
	$(OBJ2IMG) -f PRBSS -o prbss.img $@

prfib.exe: $(PRFIBOBJS)
	$(LNK7090) -wxdmo PRFIB prfib.map $@ $(PRFIBOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f PRFIB -o prfib.img $@

print.exe: $(PRINTOBJS)
	$(LNK7090) -wxdmo PRINT print.map $@ $(PRINTOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f PRINT -o print.img $@

prnter.exe: $(PRNTROBJS)
	$(LNK7090) -wxdmo PRNTER prnter.map $@ $(PRNTROBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f PRNTER -o prnter.img $@

qed.exe: $(QEDOBJS)
	$(LNK7090) -wxdmo QED qed.map $@ $(QEDOBJS)
	$(OBJ2IMG) -f QED -o qed.img $@

revoke.exe: $(REVOKEOBJS)
	$(LNK7090) -wxdmo REVOKE revoke.map $@ $(REVOKEOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f REVOKE -o revoke.img $@

rquest.exe: $(RQUESTOBJS)
	$(LNK7090) -C -wxdmo RQUEST rquest.map $@ $(RQUESTOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f RQUEST -o rquest.img $@

runcom.exe: $(RUNCOMOBJS)
	$(LNK7090) -wxdmo RUNCOM runcom.map $@ $(RUNCOMOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f RUNCOM -o runcom.img $@

runoff.exe: $(RUNOFFOBJS)
	$(LNK7090) -C -wxdmo RUNOFF runoff.map $@ $(RUNOFFOBJS) $(LIBS)
	$(OBJ2IMG) -f RUNOFF -o runoff.img $@

sdump.exe: $(SDUMPOBJS)
	$(LNK7090) -wxdmo SDUMP sdump.map $@ $(SDUMPOBJS) $(LIBS)
	$(OBJ2IMG) -f SDUMP -o sdump.img $@

squash.exe: $(SQUASHOBJS)
	$(LNK7090) -wxdmo SQUASH squash.map $@ $(SQUASHOBJS) $(LIBS)
	$(OBJ2IMG) -f SQUASH -o squash.img $@

tapfil.exe: $(TAPFILOBJS)
	$(LNK7090) -wxdmo TAPFIL tapfil.map $@ $(TAPFILOBJS) $(LIBS)
	$(OBJ2IMG) -f TAPFIL -o tapfil.img $@

ttpeek.exe: $(TTPEEKOBJS)
	$(LNK7090) -wxdmo TTPEEK ttpeek.map $@ $(TTPEEKOBJS) $(LIBS)
	$(OBJ2IMG) -f TTPEEK -o ttpeek.img $@

typset.exe: $(TYPSETOBJS)
	$(LNK7090) -wxdmo TYPSET typset.map $@ $(TYPSETOBJS) $(LIBS)
	$(OBJ2IMG) -f TYPSET -o typset.img $@

unlink.exe: $(UNLINKOBJS)
	$(LNK7090) -wxdmo UNLINK unlink.map $@ $(UNLINKOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f UNLINK -o unlink.img $@

update.exe: $(UPDATEOBJS)
	$(LNK7090) -wxdmo UPDATE update.map $@ $(UPDATEOBJS) $(LIBS)
	$(OBJ2IMG) -f UPDATE -o update.img $@

updbss.exe: $(UPDBSSOBJS)
	$(LNK7090) -Cwxdmo UPDBSS updbss.map $@ $(UPDBSSOBJS) $(LIBS)
	$(OBJ2IMG) -f UPDBSS -o updbss.img $@

verify.exe: $(VERIFYOBJS)
	$(LNK7090) -wxdmo VERIFY verify.map $@ $(VERIFYOBJS) $(LIBS)
	$(OBJ2IMG) -f VERIFY -o verify.img $@

who.exe: $(WHOOBJS)
	$(LNK7090) -wxdmo WHO who.map $@ $(WHOOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f WHO -o who.img $@

write.exe: $(WRITEOBJS)
	$(LNK7090) -wxdmo WRITE write.map $@ $(WRITEOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f WRITE -o write.img $@

xpand.exe: $(XPANDOBJS)
	$(LNK7090) -wxdmo XPAND xpand.map $@ $(XPANDOBJS) $(LIBS)
	$(OBJ2IMG) -f XPAND -o xpand.img $@

com5.tap : $(EXES)
	cat $(IMAGES) ../EOF.BIN >com5.tap

prbss.obj: prbss.fap
	$(ASM7090) -C $(ASMFLAGS) -o $@ prbss.fap
updbss.obj: updbss.fap
	$(ASM7090) -C $(ASMFLAGS) -o $@ updbss.fap
