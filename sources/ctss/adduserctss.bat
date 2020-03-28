@echo off

rem
rem Add users to CTSS
rem

set projectname=M1416
set diskquota=4000
set drumquota=0
set tapequota=0
set comment=""
set privs=1

:NEXTOPT
set gotopt=0
if "%1" == "-c" ( 
   set comment=%2
   set gotopt=1
   shift
   shift
)
if "%1" == "-d" ( 
   set diskquota=%2
   set gotopt=1
   shift
   shift
)
if "%1" == "-p" ( 
   set gotopt=1
   set projectname=%2
   shift
   shift
)
if "%1" == "-r" ( 
   set drumquota=%2
   set gotopt=1
   shift
   shift
)
if "%1" == "-t" ( 
   set tapequota=%2
   set privs=101
   set gotopt=1
   shift
   shift
)
if "%gotopt%" == "1" goto NEXTOPT

set status=0
if "%1" == "" goto USAGE
if "%2" == "" goto USAGE
if "%3" == "" goto USAGE


rem
rem Get the current info.
rem

set operation=reading

set filename="UACCNT"
call extractctss B uaccnt timacc m1416 cmfl02 uaccnt.bcd >adduserctss.log 
if not errorlevel 0 goto BADFILE
bcd2txt uaccnt.bcd uaccnt.data 168 168

set filename="TIMUSD"
call extractctss B timusd timacc m1416 cmfl02 timusd.bcd >adduserctss.log
if not errorlevel 0 goto BADFILE
bcd2txt timusd.bcd timusd.data 168 168
del adduserctss.log

rem
rem Call the program to do the work.
rem

useraddctss %1 %2 %projectname% %3 %diskquota% %drumquota% %tapequota% ^
	%comment%
if not errorlevel 0 goto BADADD

set operation=formatting
set filename="UFD"
obj2img -u -f UFD -e DATA -o ufd.img ufd.data
if not errorlevel 0 goto BADFILE

set filename="QUOTA"
obj2img -q -f QUOTA -e DATA -o quota.img quota.data
if not errorlevel 0 goto BADFILE

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

set filename="UFD"
call setupctss ufd.img >adduserctss.log
if not errorlevel 0 goto BADFILE

set filename="QUOTA"
call setupctss quota.img >adduserctss.log
if not errorlevel 0 goto BADFILE

set filename="TIMUSD"
call setupctss timusd.img >adduserctss.log
if not errorlevel 0 goto BADFILE

set filename="UACCNT"
call setupctss uaccnt.img >adduserctss.log
if not errorlevel 0 goto BADFILE

del ufd.* quota.* timusd.* uaccnt.* adduserctss.log
goto END

:BADFILE
   echo Error %operation% %filename% file:
   type adduserctss.log
   set status=1
   goto END

:USAGE
   echo adduserctss [options] programmername programmernumber password
   echo   options:"
   echo    -c "comment"
   echo    -p projectname
   echo    -d diskquota
   echo    -r drumquota
   echo    -t tapequota
:BADADD
   set status=1

:END
exit /b %status%

