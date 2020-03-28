@echo off

rem
rem Run CTSS Salvager *silently*.
rem

copy nul: cmd.cbn 2>nul:

del print.* trace.bin 2>nul:
s709 -C8 -mCTSS -ccleanupctss.cmd r=cmd p=print ^
      a1r=salv4.tap a3=salvage.bcd a9=trace.bin ^
      cd00=DISK1.BIN cn01=DRUM1.BIN cd42=DISK2.BIN ^
      gh00=DRUM2.BIN gh01=DRUM3.BIN >cleanup.lst 2>nul:

bcd2txt -p salvage.bcd salvage.lst
del salvage.bcd cmd.* print.bcd 2>nul:
