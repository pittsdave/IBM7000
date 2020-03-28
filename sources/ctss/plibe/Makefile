.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2IMG = obj2img

ASMFLAGS = $(DEBUG) -C -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

TAPES = source.tap

FIMGS = minmax.img numb.img nump.img plotr.img sym5.img
MIMGS = axis1.img cmbplt.img dxdy1.img ernote.img fixup.img graph1.img \
	graph.img pictur.img scale1.img sclgph.img set.img \
	ops.img xr.img
IMAGES = $(FIMGS) $(MIMGS) plibe.img

FSRCS = minmax.fap numb.fap nump.fap plotr.fap sym5.fap
MSRCS = axis1.mad cmbplt.mad dxdy1.mad ernote.mad fixup.mad graph1.mad \
	graph.mad pictur.mad scale1.mad sclgph.mad set.mad

SRCS = $(FSRCS) $(MSRCS)

OBJS = minmax.obj numb.obj nump.obj plotr.obj sym5.obj

all: $(OBJS)

clean:
	rm -f $(OBJS) *.lst plibe.runcom
	rm -f $(TAPES) $(IMAGES)

source.tap: $(SRCS)
	ls -1 $(FSRCS) | awk -F . '{ print "obj2img -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/fapimg.sh
	ls -1 $(MSRCS) | awk -F . '{ print "obj2img -c -t -f " $$1 " -e " $$2 " -a \" m1416pitts \" -o " $$1 ".img " $$1 "."$$2 }' >>/tmp/fapimg.sh
	@sh -x /tmp/fapimg.sh
	@rm -f /tmp/fapimg.sh
	$(OBJ2IMG) -c -t -f ".OPS." -e mad -a " M1416PITTS " -o ops.img ops.mad
	$(OBJ2IMG) -c -t -f ".XR." -e mad -a " M1416PITTS " -o xr.img xr.mad
	ls -1 $(FSRCS) | awk -F . '{ print "FAP " $$1 " (FLAG) NO (LIST)" }' >plibe.runcom
	ls -1 $(MSRCS) | awk -F . '{ print "MAD " $$1 " (LIST)" }' >>plibe.runcom
	echo "COMBIN * PLIBE BSS ernote sclgph scale1 pictur graph graph1" >>plibe.runcom
	echo "COMBIN * PLIBE BSS PLIBE dxdy1 axis1 set cmbplt fixup numb" >>plibe.runcom
	echo "COMBIN * PLIBE BSS PLIBE nump sym5 minmax plotr" >>plibe.runcom
	$(OBJ2IMG) -t -f plibe -e runcom -a " M1416PITTS " -o plibe.img plibe.runcom
	cat $(IMAGES) ../EOF.BIN >source.tap
