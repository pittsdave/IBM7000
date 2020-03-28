.SUFFIXES: .map .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

.map.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

OBJS = acept.obj _calog.obj _eras.obj fasc.obj fatn.obj fbcd_.obj fbin_.obj \
	fbst_.obj fbst.obj fcab.obj fcas.obj fclg.obj fcnv.obj fcsc.obj \
	fcsq.obj fcxp.obj fdas.obj fdat.obj fdlg.obj fdmd.obj fdmp.obj \
	fdsc.obj fdsq.obj fdvchk.obj fdx1.obj fdx2.obj fdxp.obj feft_.obj \
	feft.obj ferf.obj fgam.obj fiob.obj fioh.obj fios_.obj fios.obj \
	fiou.obj flog.obj fmtn.obj fout.obj foverf.obj fparst.obj fprn.obj \
	_fptrp.obj fpun.obj frcd.obj frdb_.obj frdb.obj frdd_.obj frdd.obj \
	frdu_.obj frdu.obj fret.obj frwt_.obj frwt.obj fsch.obj fscn.obj \
	fslbi.obj fslbo.obj fsldi.obj fsldo.obj fsli.obj fslite.obj fslo.obj \
	fsqr.obj fsswth.obj ftnc.obj ftnh.obj fvio_.obj fvio.obj fwrb_.obj \
	fwrb.obj fwrd_.obj fwrd.obj fwro.obj fwru_.obj fwru.obj fxem.obj \
	fxp1.obj fxp2.obj fxp3.obj fxpf.obj _iocsf.obj _lovry.obj _lxcon.obj \
	_lxsl.obj _rand.obj un01.obj un02.obj un03.obj un04.obj un05.obj \
	un06.obj un07.obj un08.obj unit06.obj _xcc_.obj xit.obj

all: $(OBJS)

clean:
	rm -f *.obj *.lst

