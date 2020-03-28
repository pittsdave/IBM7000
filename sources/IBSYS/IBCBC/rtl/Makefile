.SUFFIXES: .map .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

.map.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

OBJS = _carsn.obj _catan.obj _cbble.obj cbcdh.obj cbdcv.obj cbler.obj \
	cbncv.obj _ccabs.obj ccdty.obj _ccexp.obj _ccfmp.obj _cclog.obj \
	ccomp.obj _ccsin.obj _ccsqr.obj cctab.obj _cdatn.obj _cdexp.obj \
	cdif.obj cdisp.obj _cdlog.obj _cdmod.obj cdply.obj _cdsin.obj \
	_cdsqr.obj _cdxp1.obj _cdxp2.obj ceobp.obj _cerf.obj cexam.obj \
	_cexp.obj cexpr.obj cfpak.obj _cgama.obj _cgogo.obj chbcd.obj \
	ckeys.obj cmpak.obj copen.obj _csinh.obj _csin.obj _csqrt.obj \
	_ctanh.obj _ctan.obj _cxp1.obj _cxp2.obj _cxp3.obj

all: $(OBJS)

clean:
	rm -f *.obj *.lst

