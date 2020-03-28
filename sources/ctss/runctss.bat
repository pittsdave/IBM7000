@echo off

rem
rem Run CTSS.
rem

del print.* 2>nul:

rem
rem Make CMD file
rem

if "%1" == "" goto NOCARDS
   copy %1 cmd.txt
   goto RUN
:NOCARDS
   echo MIT8C0 >cmd.txt
:RUN
txt2bcd cmd.txt cmd.bcd 80 80
bcd2cbn cmd

rem
rem Run CTSS
rem

del trace.bin 2>nul:
s709 -C8 -mCTSS -crunctss.cmd r=cmd p=print ^
      a3=ctss.bcd a9=trace.bin ^
      cd00=DISK1.BIN cn01=DRUM1.BIN cd42=DISK2.BIN ^
      ec32=2023 gh00=DRUM2.BIN gh01=DRUM3.BIN

bcd2txt -p ctss.bcd ctss.map
del ctss.bcd cmd.* print.bcd 2>nul:

