@echo off

rem
rem Run IBSYS Job using tape as input.
rem usage: runibsys program listing
rem

if "%1"=="" goto USAGE
if "%2"=="" goto USAGE
del punch.* print.* sysou1.* sys*.bin %2 2>nul:

rem
rem Make sysin tape.
rem

gendate >date.txt
copy date.txt+%1+eof.dat+ibsys.ctl sysin.txt >nul:
txt2bcd sysin

rem
txt2bcd ibsys.ctl reader.bcd 80 80
bcd2cbn reader

rem
rem Run IBSYS in non-panel mode.
rem

s709 %3 -cibsys.cmd -m7094 r=reader p=print u=punch ^
      a1r=ASYS1.BIN a2r=ASYS8.BIN a3=sysin.bcd a4=sysou1.bcd ^
      a5=sysut1.bin a6=sysut3.bin a7=sysut2.bin a8=syspp1.bin ^
      a9=sysut4.bin a10=sysck2.bin 

rem
rem Convert printed output
rem

bcd2txt -p sysou1.bcd %2
del reader.* sysut*.bin sysck2.bin print.bcd 2>nul:
del sysou1.* sysin.* date.txt 2>nul:
goto END

:USAGE
echo usage: runibsys program listing

:END
