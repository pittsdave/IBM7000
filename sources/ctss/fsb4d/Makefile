.SUFFIXES: .fap .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090

ASMFLAGS = $(DEBUG) -I . -I ../include -wc CTSS -l $(subst obj,lst,$@)

.fap.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

OBJS = bucm2e.obj ddapb.obj ddst2i.obj fcor2o.obj iocm2e.obj push2a.obj \
	qman2b.obj scnd4a.obj sear2b.obj srch2a.obj stik2e.obj tman4a.obj \
	tpap2h.obj tpapdm.obj tpsm2q.obj tpst2a.obj tracb.obj

all: $(OBJS)

clean:
	rm -f $(OBJS) *.lst

