       REM
EXTLST MACRO   LIST
       IRP     LIST
       STA     LIST
       ACL     =432
       IRP
EXTLST END
       REM
EXTEND MACRO   LIST
       IFF     0,,LIST
       EXTLST  (LIST)
       TSX     SETMEM,4
EXTEND END
       REM
MSGOUT MACRO   LOC,N,MESG
       RMT
       PMC     ON
       IFF     LOC,0,1
LOC    BCI     N,MESG
       PMC     OFF
       RMT
MSGOUT END
WRFLXA MACRO
       TSX     WRFLXA,4
       RMT
       PMC     ON
       IFF     WRFLXA,0,1
WRFLXA TIA     =HWRFLXA
       PMC     OFF
       RMT
WRFLXA END
WRFLX  MACRO
       TSX     WRFLX,4
       RMT
       PMC     ON
       IFF     WRFLX,0,1
WRFLX  TIA     =HWRFLX
       PMC     OFF
       RMT
WRFLX  END
       REM
COMENT MACRO   LOC,N,MESG,CR
       PMC     OFF
       IFF     1,A,CR
       WRFLXA
       IFF     0,A,CR
       WRFLX
       PAR     LOC,,N
       IFF     0,,(MESG)
       MSGOUT  LOC,N,(MESG)
COMENT END
*
* Assignment macros
*
       MACRO
V      =       F,A
       PMC     ON
       MTSX    F,,
       IRP     A
       CALL1   A,,,
       IRP
       STO     V
       PMC     OFF
       END
*
       MACRO
V      =L      F,A
       PMC     ON
       MTSX    F,,
       IRP     A
       CALL1   A,,,
       IRP
       SLW     V
       PMC     OFF
       END
