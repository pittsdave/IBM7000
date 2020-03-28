@echo off

rem
rem Run LISP Job using tape as input
rem
if "%1"=="" goto USAGE
del punch.* print.* sysout.* sys*.bin %2 2>nul:

rem
rem Make sysin tape.
rem

txt2bcd %1 sysin
copy nul: reader.cbn 2>nul:

rem
rem Run LISP
rem

s709 -p -clisp.cmd -m7090 r=reader pc=print u=punch ^
      a1=sysin.bcd a2=sysppt.bin a3=sysout.bcd a4=LISPTAPE.BIN ^
      b3=syscore.bin 

if "%2"=="" goto NOLIST
REM
REM Convert printed output
REM
bcd2txt -p sysout.bcd %2

:NOLIST
del reader.cbn sysin.bcd sysout.bcd 2>nul:
goto END

:USAGE
echo usage: runlisp program [listing] 

:END
