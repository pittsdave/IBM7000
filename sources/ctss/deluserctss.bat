@echo off

rem
rem Delete users from CTSS
rem
rem We leave behind the User's UFD and data.
rem

set status=0
set projectname=M1416

:NEXTOPT
set gotopt=0
if "%1" == "-p" ( 
   set gotopt=1
   set projectname=%2
   shift
   shift
)
if "%gotopt%" == "1" goto NEXTOPT

if "%1" == "" goto USAGE


rem
rem Get the current info.
rem

set operation=reading

set filename="UACCNT"
call extractctss B uaccnt timacc m1416 cmfl02 uaccnt.bcd >deluserctss.log
if not errorlevel 0 goto BADFILE
bcd2txt uaccnt.bcd uaccnt.data 168 168

set filename="TIMUSD"
call extractctss B timusd timacc m1416 cmfl02 timusd.bcd >deluserctss.log 
if not errorlevel 0 goto BADFILE
bcd2txt timusd.bcd timusd.data 168 168

rem
rem Call the program to do the work.
rem

userdelctss %1 %projectname% 
if not errorlevel 0 goto BADDEL

set operation=formatting
set filename="TIMUSD"
obj2img -t -r 168 -f TIMUSD -e TIMACC -o timusd.img timusd.data
if not errorlevel 0 goto BADFILE

set filename="UACCNT"
obj2img -t -r 168 -f UACCNT -e TIMACC -o uaccnt.img uaccnt.data
if not errorlevel 0 goto BADFILE

rem
rem Update.
rem

set operation=updating

set filename="TIMUSD"
call setupctss timusd.img >deluserctss.log 
if not errorlevel 0 goto BADFILE

set filename="UACCNT"
call setupctss uaccnt.img >deluserctss.log 
if not errorlevel 0 goto BADFILE

del timusd.* uaccnt.* deluserctss.log
goto END

:BADFILE
   echo Error %operation% %filename% file:
   type deluserctss.log
:BADDEL
   set status=1
   goto END

:USAGE
   echo deluserctss [options] programmername
   echo   options:
   echo    -p projectname
   set status=1

:END
exit /b %status%
