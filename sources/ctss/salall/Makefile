.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -I ../include -xwc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

EXES = salv4.exe sort4.exe stat4.exe
TAPES = salv4.tap sort4.tap stat4.tap

LIBS = -L../tslib1

SALV4OBJS = salv4.mdo cdmp.mdo cent4.mdo cvdt4.mdo pter4.mdo rfil4.mdo \
	rtut4.mdo smfd4.mdo dioi.obj indx.obj keys.obj mprt.obj rst.obj \
	setup.obj stop.obj xpnd.obj \
	../s.util/ddio4c.obj ../s.util/ddpac.obj

SORT4OBJS = sortm.mdo sort.mdo cdmp.mdo cvdt4.mdo pter4.mdo rfil4.mdo \
	rtut4.mdo smfd4.mdo dioi.obj indx.obj keys.obj mprt.obj rdin.obj \
	rst.obj setup.obj stop.obj xpnd.obj \
	../s.util/ddio4c.obj ../s.util/ddpac.obj

STAT4OBJS = stat4.mdo cdmp.mdo cvdt4.mdo pter4.mdo rfil4.mdo \
	rtut4.mdo smfd4.mdo dioi.obj indx.obj keys.obj mprt.obj rst.obj \
	setup.obj stop.obj xpnd.obj \
	../s.util/ddio4c.obj ../s.util/ddpac.obj

SALV4MSRCS = salv4.mad cdmp.mad cent4.mad cvdt4.mad pter4.mad rfil4.mad \
	rtut4.mad smfd4.mad 
SALV4FSRCS = dioi.fap indx.fap keys.fap mprt.fap rst.fap \
	setup.fap stop.fap xpnd.fap

OBJS = dioi.obj diot.obj indx.obj keys.obj mprt.obj rdin.obj rst.obj setup.obj \
	stop.obj xpnd.obj

all: $(OBJS) $(TAPES)

clean:
	rm -f $(OBJS) *.lst
	rm -f $(TAPES) $(EXES) *.map *.bin *.img
	rm -f source.tap

salv4.exe: $(SALV4OBJS)
	$(LNK7090) -xdmo SALV4 salv4.map salv4.exe $(SALV4OBJS) $(LIBS)

sort4.exe: $(SORT4OBJS)
	$(LNK7090) -xdmo sort4 sort4.map sort4.exe $(SORT4OBJS) $(LIBS)

stat4.exe: $(STAT4OBJS)
	$(LNK7090) -xdmo STAT4 stat4.map stat4.exe $(STAT4OBJS) $(LIBS)

salv4a.tap: salv4a.ctssld
	ls -1 *.bss | awk -F . '{ print "obj2img -b -f " $$1 " -e " $$2 " -a \" m1416cmfl01\" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/bssimg.sh
	@sh -x /tmp/bssimg.sh
	@rm -f /tmp/bssimg.sh
	$(OBJ2IMG) -t -f salv4a -e ctssld -a " M1416CMFL01" -o salv4a.img salv4a.ctssld
	cat *.img ../EOF.BIN >salv4a.tap

salv4.tap: salv4.exe
	$(OBJ2BIN) -l 64 salv4.exe salv4.bin
	cat ../loader.bin salv4.bin ../EOF.BIN >$@

sort4.tap: sort4.exe
	$(OBJ2BIN) -l 64 sort4.exe sort4.bin
	cat ../loader.bin sort4.bin ../EOF.BIN >$@

stat4.tap: stat4.exe
	$(OBJ2BIN) -l 64 stat4.exe stat4.bin
	cat ../loader.bin stat4.bin ../EOF.BIN >$@

source.tap: $(SALV4MSRCS) $(SALV4FSRCS)
	ls -1 $(SALV4MSRCS) | awk -F . '{ print "obj2img -c -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	ls -1 $(SALV4MSRCS) | awk -F . '{ print "MAD " $$1 "\nUPDATE 1 " $$1 " BSS\nDELETE " $$1 " BSS" }' >mksal.runcom
	#ls -1 $(SALV4MSRCS) | awk -F . '{ print "MAD " $$1 }' >mksal.runcom
	$(OBJ2IMG) -c -t -f salequ -e mad -a " M1416PITTS " -o salequ.img salequ.mad
	ls -1 $(SALV4FSRCS) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	ls -1 $(SALV4FSRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nUPDATE 1 " $$1 " BSS\nDELETE " $$1 " BSS\nDELETE " $$1 " SYMTB" }' >>mksal.runcom
	#ls -1 $(SALV4FSRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >>mksal.runcom
	$(OBJ2IMG) -t -f ddio4c -e fap -a " M1416PITTS " -o ddio4c.img ../s.util/ddio4c.fap
	echo "FAP DDIO4C (FLAG) NO" >>mksal.runcom
	echo "UPDATE 1 DDIO4C BSS" >>mksal.runcom
	echo "DELETE DDIO4C BSS" >>mksal.runcom
	echo "DELETE DDIO4C SYMTB" >>mksal.runcom
	$(OBJ2IMG) -t -f ddpac -e fap -a " M1416PITTS " -o ddpac.img ../s.util/ddpac.fap
	echo "FAP DDPAC (FLAG) NO" >>mksal.runcom
	echo "UPDATE 1 DDPAC BSS" >>mksal.runcom
	echo "DELETE DDPAC BSS" >>mksal.runcom
	echo "DELETE DDPAC SYMTB" >>mksal.runcom
	#echo "LINK TSLIB1 BSS M1416 CMFL04" >>mksal.runcom
	#echo "EXTBSS TSLIB1 BCDEC" >>mksal.runcom
	#echo "EXTBSS TSLIB1 BZEL" >>mksal.runcom
	#echo "EXTBSS TSLIB1 DEFBC" >>mksal.runcom
	#echo "EXTBSS TSLIB1 OCABC" >>mksal.runcom
	#echo "UNLINK TSLIB1 BSS" >>mksal.runcom
	#echo "COMBIN * SALLIB BSS BCDEC BZEL DEFBC OCABC" >>mksal.runcom
	echo "UPDATE 1 SALLIB BSS" >>mksal.runcom
	$(OBJ2IMG) -t -f salv4a -e ctssld -a " M1416PITTS " -o salv4ald.img salv4a.ctssld
	echo "UPDATE 1 SALV4A CTSSLD" >>mksal.runcom
	$(OBJ2IMG) -t -f mksal -e runcom -a " M1416PITTS " -o mksal.img mksal.runcom
	cat *.img ../EOF.BIN >source.tap
