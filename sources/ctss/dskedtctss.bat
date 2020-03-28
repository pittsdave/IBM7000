@echo off

rem
rem Run CTSS DSKEDT.
rem

del print.* 2>nul:

rem
rem Make CMD file
rem

if "%1" == "" goto NOCARDS
   copy %1 cmd.bcd
   goto RUN
:NOCARDS
   echo STOP >cmd.txt
:RUN
txt2bcd cmd.txt cmd.bcd 80 80
bcd2cbn cmd

rem
rem Run DSKEDT.
rem

del trace.bin 2>nul:
s709 -C8 -mCTSS -cdskedtctss.cmd r=cmd p=print ^
      a1r=dskedt.tap a3=sysprint.bcd a9=trace.bin ^
      b1=syscarry.bcd b2=syspunchid.bcd b4=syspunch.bcd ^
      cd00=DISK1.BIN cn01=DRUM1.BIN cd42=DISK2.BIN ^
      gh00=DRUM2.BIN gh01=DRUM3.BIN

del cmd.* print.bcd 2>nul:

