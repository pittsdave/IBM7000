@echo off

rem
rem Run CTSS diagnostics.
rem

del print.* 2>nul:

if "%1" == "" goto USAGE

rem
rem Make CMD file
rem

if "%2" == "" goto NOCARDS
   txt2bcd %2 cmd.bcd 80 80
   bcd2cbn cmd
   goto RUN
:NOCARDS
   copy nul: cmd.cbn 2>nul:
:RUN

rem
rem Run the diagnostic
rem

del trace.bin 2>nul:
s709 -C8 -mCTSS -cdiagctss.cmd r=cmd p=print ^
      a1r=%1 a2r=loader.tap a9=trace.bin ^
      cd00=DISK1.BIN cn01=DRUM1.BIN cd42=DISK2.BIN ^
      ec32=2023 gh00=DRUM2.BIN gh01=DRUM3.BIN

del cmd.* print.bcd 2>nul:
goto END

:USAGE
   echo usage: diagctss ctssdiagtape [commands]

:END
