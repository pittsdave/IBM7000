@echo off

rem
rem Report on CTSS users
rem

set status=0

set projectname=M1416
set programmername=___ALL
set detail=0

:NEXTOPT
set gotopt=0
if "%1" == "-d" ( 
   set detail=1
   set gotopt=1
   shift
)
if "%1" == "-p" ( 
   set gotopt=1
   set projectname=%2
   shift
   shift
)
if "%gotopt%" == "1" goto NEXTOPT

if "%1" == "" goto ALLUSER
   set programmername=%1
:ALLUSER

rem
rem Get the current info.
rem

set filename="UACCNT"
call extractctss B uaccnt timacc m1416 cmfl02 uaccnt.bcd >rptuserctss.log 
if not errorlevel 0 goto BADFILE
bcd2txt uaccnt.bcd uaccnt.data 168 168

set filename="TIMUSD"
call extractctss B timusd timacc m1416 cmfl02 timusd.bcd >rptuserctss.log
if not errorlevel 0 goto BADFILE
bcd2txt timusd.bcd timusd.data 168 168

set filename="REPORT"
call extractctss T report timacc m1416 cmfl02 report.data >rptuserctss.log
if not errorlevel 0 goto BADFILE

rem
rem Go generate the report
rem

userreportctss %programmername% %projectname% %detail%

del timusd.* uaccnt.* report.* 
goto END

:BADFILE
   echo Error reading %filename% file:
   type rptuserctss.log
   set status=1
   goto END

:USAGE
   echo reportuserctss [options] [programmername]
   echo   options:
   echo    -d
   echo    -p projectname
   set status=1

:END
del rptuserctss.log
exit /b %status%
