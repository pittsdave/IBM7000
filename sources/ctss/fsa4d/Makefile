.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

SRCS = bucm2e.fap ddap4c.fap ddst2i.fap fcor2o.fap iocm2e.fap push2a.fap \
	qman2b.fap scnd4a.fap sear2b.fap srch2a.fap stik2e.fap tman4a.fap \
	tpap2h.fap tpsm2q.fap trac2a.fap

OBJS = bucm2e.obj ddap4c.obj ddst2i.obj fcor2o.obj iocm2e.obj push2a.obj \
	qman2b.obj scnd4a.obj sear2b.obj srch2a.obj stik2e.obj tman4a.obj \
	tpap2h.obj tpsm2q.obj trac2a.obj

all: $(OBJS)

clean:
	rm -f $(OBJS) *.lst
	rm -f *.runcom source.tap *.img

source.tap: $(SRCS) ../include/cma.d.inc ../include/cmb.c.inc \
	../include/cmm.d.inc ../include/cmq.c.inc ../include/cmv.c.inc \
	../include/ioequ.inc ../include/sm1a.i.inc ../include/sm1b.g.inc \
	../include/sm1e.d.inc ../include/sm1m.c.inc ../include/sm1v.f.inc \
	../include/stmequ.inc ../include/stmmac.inc ../include/stm.mc.inc \
	../include/tsmequ.inc ../include/ioequb.inc
	ls -1 $(SRCS) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -t -f cma.d -e fap -a " M1416PITTS " -o cma.d.img ../include/cma.d.inc
	$(OBJ2IMG) -t -f cmb.c -e fap -a " M1416PITTS " -o cmb.c.img ../include/cmb.c.inc
	$(OBJ2IMG) -t -f cmm.d -e fap -a " M1416PITTS " -o cmm.d.img ../include/cmm.d.inc
	$(OBJ2IMG) -t -f cmq.c -e fap -a " M1416PITTS " -o cmq.c.img ../include/cmq.c.inc
	$(OBJ2IMG) -t -f cmv.c -e fap -a " M1416PITTS " -o cmv.c.img ../include/cmv.c.inc
	$(OBJ2IMG) -t -f ioequa -e fap -a " M1416PITTS " -o ioequ.img ../include/ioequ.inc
	$(OBJ2IMG) -t -f ioequb -e fap -a " M1416PITTS " -o ioequb.img ../include/ioequb.inc
	$(OBJ2IMG) -t -f sm1a.i -e fap -a " M1416PITTS " -o sm1a.i.img ../include/sm1a.i.inc
	$(OBJ2IMG) -t -f sm1b.g -e fap -a " M1416PITTS " -o sm1b.g.img ../include/sm1b.g.inc
	$(OBJ2IMG) -t -f sm1e.d -e fap -a " M1416PITTS " -o sm1e.d.img ../include/sm1e.d.inc
	$(OBJ2IMG) -t -f sm1m.c -e fap -a " M1416PITTS " -o sm1m.c.img ../include/sm1m.c.inc
	$(OBJ2IMG) -t -f sm1v.f -e fap -a " M1416PITTS " -o sm1v.f.img ../include/sm1v.f.inc
	$(OBJ2IMG) -t -f stmequ -e fap -a " M1416PITTS " -o stmequ.img ../include/stmequ.inc
	$(OBJ2IMG) -t -f stmmac -e fap -a " M1416PITTS " -o stmmac.img ../include/stmmac.inc
	$(OBJ2IMG) -t -f stm.mc -e fap -a " M1416PITTS " -o stm.mc.img ../include/stm.mc.inc
	$(OBJ2IMG) -t -f tsmequ -e fap -a " M1416PITTS " -o tsmequ.img ../include/tsmequ.inc
	echo "MOVE IOEQUA FAP IOEQU FAP" >mkfsa.runcom
	#ls -1 $(SRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nUPDATE 1 " $$1 " BSS\nDELETE " $$1 " BSS\nDELETE " $$1 " SYMTB" }' >>mkfsa.runcom
	ls -1 $(SRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >>mkfsa.runcom
	$(OBJ2IMG) -t -f mkfsa -e runcom -a " M1416PITTS " -o mkfsa.img mkfsa.runcom
	echo "MOVE IOEQUB FAP IOEQU FAP" >mkfsb.runcom
	ls -1 $(SRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO\nDELETE " $$1 " SYMTB" }' >>mkfsb.runcom
	$(OBJ2IMG) -t -f mkfsb -e runcom -a " M1416PITTS " -o mkfsb.img mkfsb.runcom
	cat *.img ../EOF.BIN >source.tap
