.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

EXES = init.exe login.exe logout.exe
IMAGES = ufd.img init.img login.img logout.img mad.img load.img prtygp.img \
	tssfil.img grpuni.img timusd.img uaccnt.img
TAPES = com3.tap comdata.tap quotas.tap

LIBS = -L../tslib1
XLIBS = -L../xlibe

INITOBJS = init.obj initut.obj 
LOGINOBJS = login.obj logout.obj gtpass.obj gtprob.obj report.obj \
	hrmin.obj abstim.obj seterr.obj svcpu.obj messg.obj logutl.obj \
	deltem.obj
LOGOUTOBJS = logomn.obj logout.obj gtpass.obj gtprob.obj report.obj \
	hrmin.obj abstim.obj seterr.obj svcpu.obj messg.obj logutl.obj \
	deltem.obj

OBJS = abstim.obj clogin.obj deltem.obj gtpass.obj gtprob.obj \
	hrmin.obj init.obj initut.obj load.obj login.obj logomn.obj \
	logout.obj logutl.obj mad01.obj mad02.obj mad03.obj messg.obj \
	report.obj seterr.obj svcpu.obj

all: $(OBJS) $(EXES) $(TAPES)

clean:
	rm -f $(OBJS) *.lst
	rm -f $(EXES) $(IMAGES) *.map *.tap *.img

init.exe: $(INITOBJS)
	$(LNK7090) -wxdmo INIT init.map $@ $(INITOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f INIT8A -o init.img $@

login.exe: $(LOGINOBJS)
	$(LNK7090) -wxdmo LOGIN login.map $@ $(LOGINOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f LOGN8A -o login.img $@

logout.exe: $(LOGOUTOBJS)
	$(LNK7090) -wxdmo LOGOUT logout.map $@ $(LOGOUTOBJS) $(XLIBS) $(LIBS)
	$(OBJ2IMG) -f LOGT8A -o logout.img $@

com3.tap : $(EXES) mad01.obj mad02.obj mad03.obj load.obj grpuni.timacc \
	prtygp.timacc tssfil.timacc timusd.timacc uaccnt.timacc ufd.dat
	$(OBJ2IMG) -u -f UFD -e DATA -o ufd.img ufd.dat
	$(OBJ2IMG) -f MAD -o mad.img mad01.obj mad02.obj mad03.obj
	$(OBJ2IMG) -f LOAD -o load.img load.obj
	$(OBJ2IMG) -t -f GRPUNI -e TIMACC -o grpuni.img grpuni.timacc
	$(OBJ2IMG) -t -f PRTYGP -e TIMACC -o prtygp.img prtygp.timacc
	$(OBJ2IMG) -t -f TSSFIL -e TIMACC -o tssfil.img tssfil.timacc
	$(OBJ2IMG) -t -r 168 -f TIMUSD -e TIMACC -o timusd.img timusd.timacc
	$(OBJ2IMG) -t -r 168 -f UACCNT -e TIMACC -o uaccnt.img uaccnt.timacc
	cat $(IMAGES) ../EOF.BIN >com3.tap

comdata.tap: ../mit8c0/com800.img ../mit8c0/coms00.img messag.today \
	../plibe.bss ../tslib1.bss ../tslib2.bss ../xlibe.bss
	$(OBJ2IMG) -t -f MESSAG -e TODAY -a " M1416CMFL04" -o messag.img \
		messag.today
	$(OBJ2IMG) -b -f plibe -e bss -a " M1416CMFL04" -o plibe.img \
		../plibe.bss
	$(OBJ2IMG) -b -f tslib1 -e bss -a " M1416CMFL04" -o tslib1.img \
		../tslib1.bss
	$(OBJ2IMG) -b -f tslib2 -e bss -a " M1416CMFL04" -o tslib2.img \
		../tslib2.bss
	$(OBJ2IMG) -b -f xlibe -e bss -a " M1416CMFL04" -o xlibe.img \
		../xlibe.bss
	cat ../mit8c0/com800.img ../mit8c0/coms00.img messag.img plibe.img \
		tslib1.img tslib2.img xlibe.img ../EOF.BIN >comdata.tap

quotas.tap: quotas.dat
	$(OBJ2IMG) -q -f QUOTA -e DATA -o quotas.img quotas.dat
	cat quotas.img ../EOF.BIN >quotas.tap
