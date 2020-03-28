@echo off

rem
rem Run CTSS Extract Job.
rem

set plot=N
set print=N
set text=N
set status=0

if "%1" == "" goto USAGE
if "%2" == "" goto FILEMODE
if "%3" == "" goto FILEMODE
if "%4" == "" goto USAGE
if "%5" == "" goto USAGE

if "%1" == "B" goto MODEOK
if "%1" == "b" goto MODEOK
if "%1" == "L" goto LISTMODE
if "%1" == "l" goto LISTMODE
if "%1" == "P" goto PLOTMODE
if "%1" == "p" goto PLOTMODE
if "%1" == "T" goto TEXTMODE
if "%1" == "t" goto TEXTMODE

echo %1: Unsupported extract mode
goto USAGE

:LISTMODE
   set print=Y
   goto MODEOK
:PLOTMODE
   set plot=Y
   goto MODEOK
:TEXTMODE
   set text=Y
   goto MODEOK

:MODEOK
   genext %1 %2 %3 %4 %5 >cmd.txt
   set outfile=%6
   goto CONVERT

:FILEMODE
   if not exist %1 goto NOTTHERE
   copy %1 cmd.txt
   set outfile=%2
   goto CONVERT

:NOTTHERE
   echo %1: File not found
   set status=1
   goto END

:CONVERT
txt2bcd cmd.txt cmd.bcd 84 84

del print.* trace.bin 2>nul:
s709 -C8 -mCTSS -cutilctss.cmd p=print ^
      a1r=extract.tap a9=trace.bin ^
      b2r=cmd.bcd b3=sysout.bcd ^
      cd00=DISK1.BIN cn01=DRUM1.BIN cd42=DISK2.BIN ^
      gh00=DRUM2.BIN gh01=DRUM3.BIN

del cmd.* print.bcd 2>nul:

if "%outfile%" == "" goto END
if "%print%" == "Y" goto LIST
if "%plot%" == "Y" goto PLOT
if "%text%" == "Y" goto TEXT
copy sysout.bcd %outfile%
goto END

:LIST
   bcd2txt -p sysout.bcd %outfile%
   goto END
:PLOT
   plotcvt sysout.bcd
   copy calcomp_099.ps %outfile%
   del calcomp_099.ps 2>nul:
   goto END
:TEXT
   bcd2txt sysout.bcd %outfile%
   goto END

:USAGE
   echo usage: extractctss commandfile [outputfile]
   echo        extractctss mode name1 name2 projname progname [outputfile]
   echo   mode: 
   echo     B  - BSS file
   echo     L  - Listing file
   echo     P  - Plotter file
   echo     T  - Text file
   echo   example: 
   echo     extractctss L hello bcd m1416 guest hello.lst
   set status=1

:END
exit /b %status%
