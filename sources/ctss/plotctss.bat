@echo off

rem
rem Run CTSS PLOT Job.
rem

del print.* 2>nul:

if "%1" == "" goto USAGE
if "%2" == "" goto USAGE
if "%3" == "" goto USAGE
if "%4" == "" goto USAGE

genext P .PLOT. %1 %2 %3 >cmd.txt 2>nul:
txt2bcd cmd.txt cmd.bcd 84 84

del trace.bin sysplot.bcd 2>nul:
s709 -C8 -mCTSS -cutilctss.cmd p=print ^
      a1r=extract.tap a9=trace.bin ^
      b2r=cmd.bcd b3=sysplot.bcd ^
      cd00=DISK1.BIN cn01=DRUM1.BIN cd42=DISK2.BIN ^
      gh00=DRUM2.BIN gh01=DRUM3.BIN

del cmd.* print.bcd 2>nul:

echo CONVERT PLOT FILE sysplot.bcd INTO %4
plotcvt %5 sysplot.bcd
copy calcomp_099.ps %4 >nul:
goto END

:USAGE
echo usage: plotctss plotfile projname probname output [cvtopts]

:END
del calcomp_099.ps 2>nul:
