       REM
       REM     .... COMMON PACKAGE FOR LOGIN/LOGOUT .....   
       REM
       IFF     XBUILD,0,1                                        DGPHACK
       KOMMON  27            CARD IMAGE BUFFER
       IFF     XBUILD,0,1                                        DGPHACK
CARD   KOMMON  1
       IFF     XBUILD,1,1                                        DGPHACK
CARD   EQU     LOGCMN        CARD IMAGE BUFFER                   DGPHACK
       REM
       REM     CARD ENTRIES FOR UACCNT TIMACC   
       REM
UNAM   SYN     CARD          USER NAME    
UPROGN SYN     CARD+1        USER PROGRAMMER NO.
UPRTGP SYN     CARD+2        USER PARTY GROUP   
USTNBY SYN     CARD+3        IF NON-ZERO, USER MAY BE STANDBY     
UFD    SYN     CARD+4        USER FILE DIRECTORY
UNTGP  SYN     CARD+5        USER UNIT GROUP    
URCOD  SYN     CARD+6        USER RESTRICTION CODE    
UFLAGS SYN     CARD+7        INDICATORS   
UPASSW SYN     CARD+8        USER PASSWORD
UDRUM  SYN     CARD+14       USER DRUM ALLOTMENT
UDISK  SYN     CARD+15       USER DISK ALLOTMENT
UTAPE  SYN     CARD+16       USER TAPE ALLOTMENT
UTA1   SYN     CARD+17       SHIFT 1 TIME ALLOTTED    
UTA2   SYN     CARD+18       SHIFT 2 TIME ALLOTTED    
UTA3   SYN     CARD+19       SHIFT 3 TIME ALLOTTED    
UTA4   SYN     CARD+20       SHIFT 4 TIME ALLOTTED    
UTA5   SYN     CARD+21       SHIFT 5 TIME ALLOTTED (FIB)    
       REM
USTAR  SYN     CARD          PROBLEM MASTER CARD
UPROBN SYN     CARD+1        PROBLEM NUMBER     
       REM
       REM
       REM     CARD ENTRIES FOR TIMUSD TIMACC   
       REM
TPROBN SYN     CARD          USER PROBLEM NUMBER
TPROGN SYN     CARD+1        USER PROGRAMMER NO.
TNAME  SYN     CARD+2        USER NAME    
TLAST  SYN     CARD+14       DATE/TIME LAST LOGOUT    
TUNIT  SYN     CARD+16       UNITID LAST LOGOUT 
TTU1   SYN     CARD+18       SHIFT 1 TIME USED  
TTU2   SYN     CARD+19       SHIFT 2 TIME USED  
TTU3   SYN     CARD+20       SHIFT 3 TIME USED  
TTU4   SYN     CARD+21       SHIFT 4 TIME USED  
TTU5   SYN     CARD+22       SHIFT 5 TIME USED (FIB)  
TCON   SYN     CARD+23       CONSOLE TIME USED  
       REM
       REM
       IFF     XBUILD,0,1                                        DGPHACK
       KOMMON  6             VECTOR FOR 'WHOAMI'
       IFF     XBUILD,0,1                                        DGPHACK
WHO    KOMMON  1
       IFF     XBUILD,1,1                                        DGPHACK
WHO    EQU     CARD+28       VECTOR FOR 'WHOAMI'                 DGPHACK
       REM
WPROBN SYN     WHO           USER PROBLEM NO.   
WPROGN SYN     WHO+1         USER PROGRAMMER NO.
WSYSNM SYN     WHO+2         SYSTEM NAME  
WUNIT  SYN     WHO+3         CONSOLE UNITID     
WLOGNM SYN     WHO+4         NAME OF LOGIN COMMAND    
WUFDNM SYN     WHO+5         USER HOME FILE DIRECTORY 
WNAME  SYN     WHO+6         USER NAME    
       REM
       REM
       IFF     XBUILD,0,1                                        DGPHACK
       KOMMON  4             TIME ALLOTTED/USED
       IFF     XBUILD,0,1                                        DGPHACK
TU     KOMMON  1
       IFF     XBUILD,1,1                                        DGPHACK
TU     EQU     WHO+7         TIME ALLOTTED/USED                  DGPHACK
       IFF     XBUILD,0,1                                        DGPHACK
       KOMMON  4
       IFF     XBUILD,0,1                                        DGPHACK
TA     KOMMON  1             ..
       IFF     XBUILD,1,1                                        DGPHACK
TA     EQU     TU+5          ..                                  DGPHACK
       REM
       REM
       IFF     XBUILD,0,1                                        DGPHACK
       KOMMON  13
       IFF     XBUILD,0,1                                        DGPHACK
RCARD  KOMMON  1             REPORT CARD (REPORT TIMACC)
       IFF     XBUILD,1,1                                        DGPHACK
RCARD  EQU     TA+5          REPORT CARD (REPORT TIMACC)         DGPHACK
       REM
RPROB  SYN     RCARD         REPORTED PROBLEM NUMBER  
RPROG  SYN     RCARD+1       REPORTED PROGRAMMER NUMBER     
RUNIT  SYN     RCARD+2       REPORTED UNIT ID   
RDATE  SYN     RCARD+3       REPORTED DATE
RTIME  SYN     RCARD+4       REPORTED TIME
RFLAGS SYN     RCARD+5       REPORTED FLAGS     
RTU1   SYN     RCARD+6       REPORTED TIME USED - SHIFT 1   
RTU2   SYN     RCARD+7       REPORTED TIME USED - SHIFT 2   
RTU3   SYN     RCARD+8       REPORTED TIME USED - SHIFT 3   
RTU4   SYN     RCARD+9       REPORTED TIME USED - SHIFT 4   
RTU5   SYN     RCARD+10      REPORTED TIME USED - SHIFT 5   
RCON   SYN     RCARD+11      REPORTED CONSOLE TIME USED     
RNAME  SYN     RCARD+12      REPORTED USER'S NAME     
       IFF     XBUILD,1,1                                        DGPHACK
COMLEN EQU     RCARD-CARD+14                                     DGPHACK
