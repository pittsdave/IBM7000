@echo off

REM Install the CTSS tapes

set status=0

call setupctss cmd3.tap
if not errorlevel 0 goto BADFILE
call setupctss cmd1.tap
if not errorlevel 0 goto BADFILE
call setupctss cmd2.tap
if not errorlevel 0 goto BADFILE
call setupctss cmd4.tap
if not errorlevel 0 goto BADFILE
call setupctss cmd5.tap
if not errorlevel 0 goto BADFILE
call setupctss lisp.tap
if not errorlevel 0 goto BADFILE
call setupctss comlinks.tap
if not errorlevel 0 goto BADFILE
call cleanupctss
call setupctss comdata.tap
if not errorlevel 0 goto BADFILE
call setupctss quotas.tap
if not errorlevel 0 goto BADFILE
call setupctss fibmon.tap
if not errorlevel 0 goto BADFILE
call setupctss util.tap
if not errorlevel 0 goto BADFILE
call setupctss cmfl01.tap
if not errorlevel 0 goto BADFILE
call setupctss guest.tap
if not errorlevel 0 goto BADFILE
call setupctss progs.tap
if not errorlevel 0 goto BADFILE
call cleanupctss
exit /b 0

:BADFILE
exit /b 1
