@echo off

rem
rem Run CTSS Salvager.
rem

del print.* 2>nul:

rem
rem Make CMD file
rem

copy nul: cmd.cbn 2>nul:

rem
rem Run Salvager image
rem

del trace.bin 2>nul:
s709 -C8 -mCTSS -csalvagectss.cmd r=cmd p=print ^
      a1r=salv4.tap a3=salvage.bcd a9=trace.bin ^
      cd00=DISK1.BIN cn01=DRUM1.BIN cd42=DISK2.BIN ^
      gh00=DRUM2.BIN gh01=DRUM3.BIN

bcd2txt -p salvage.bcd salvage.lst
del salvage.bcd cmd.* print.bcd 2>nul:

