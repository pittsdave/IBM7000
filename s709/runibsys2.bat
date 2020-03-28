@echo off

rem
rem Run IBSYS Job using tape as input, using 2 channels.
rem

if "%1" == "" goto USAGE
if "%2" == "" goto USAGE
del punch.* print.* sysou* %2 2>nul:

rem
rem Make sysin file
rem

gendate >date.txt
copy date.txt+%1+eof.dat+ibsys.ctl sysin.txt >nul:
txt2bcd sysin 

txt2bcd ibsys.ctl reader.bcd 80 80
bcd2cbn reader

rem
rem Run IBSYS
rem

s709 %3 -cibsys2.cmd -m7094 r=reader p=print u=punch ^
     a1r=ASYS12.BIN a2r=ASYS8.BIN a3=sysin.bcd a4=syslb4.bin ^
     a5=sysut1.bin a6=sysut3.bin a7=sysck2.bin ^
     b1=sysou1.bcd b2=sysou2.bcd b3=syspp1.bin b4=syspp2.bin ^
     b5=sysut2.bin b6=sysut4.bin 

rem
rem Convert printed output
rem

bcd2txt -p sysou1.bcd %2
del reader.* sysut*.bin sysck2.bin print.bcd 2>nul:
del date.txt sysou1.* sysin.* 2>nul:
goto END

:USAGE
echo usage: runibsys2 program listing 

:END
