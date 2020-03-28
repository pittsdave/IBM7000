@echo off

rem
rem Run Disk IBSYS Job using tape as input, using 2 channels. Operator enabled.
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

s709 %3 -cibsys2d.cmd -m7094 r=reader p=print u=punch ^
      a1r=ASYS1D.BIN a2=sysin.bcd ^
      a3=sysut1.bin a4=sysut3.bin a5=sysck2.bin ^
      b1=sysou1.bcd b2=syspp1.bin b3=sysut2.bin b4=sysut4.bin ^
      dd00=IBSYS1.DSK dd01=IBSYS2.DSK dd02=IBSYS3.DSK 

rem
rem Convert printed output
rem

bcd2txt -p sysou1.bcd %2
del reader.* sysut*.bin sysck2.bin print.bcd 2>nul:
del date.txt sysou1.* sysin.* 2>nul:
goto END

:USAGE
echo usage: runibsys2d program listing 

:END
