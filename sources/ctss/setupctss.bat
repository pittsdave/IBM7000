@echo off

rem
rem Run CTSS Setup Job using tape to load
rem

del print.* 2>nul:
set status=0

if "%1" == "" goto USAGE

del trace.bin 2>nul:
s709 -C8 -mCTSS -cutilctss.cmd p=print ^
      a1r=setup.tap a9=trace.bin b2r=%1 ^
      cd00=DISK1.BIN cn01=DRUM1.BIN cd42=DISK2.BIN ^
      gh00=DRUM2.BIN gh01=DRUM3.BIN
if not errorlevel 0 goto BADFILE

del cmd.* print.bcd 2>nul:
goto END

:USAGE
   echo usage: setupctss setupimage
:BADFILE
   set status=1

:END
exit /b %status%
