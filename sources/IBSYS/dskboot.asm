       ORG     0
       IOCD    RSCQ,,21
       TCOA    *
       TRA     RSCQ
*
       ORG     65
RSCQ   RSCD    SMSQ             Do seek to location of IBSYS
SCDQ   SCDD    0                Store diagnostic
       LDI     0
       LFT     7100             Check if good...
       TRA     *+3              No, Blast and try again
TCOQ   TCOD    SCDQ             Loop
       TRA     3                Enter IBSYS cold start
RICQ   RICD    **               Blast 7909
LDVCY  OCT     500512121212,121222440000
LDSEK  OCT     501212120000,0
SMSQ   SMS     14               Inhibit ATT, U.E.
       CTL     LDSEK            Seek
       TCM     *                Wait
       CTLR    LDVCY            Verify IBSYS Cyl.
       CPYP    *+1,,1           Scatter load
       WTR     *                Wait and or load
       TCH     *-2
       CPYD    0,,0             Syscyd actual IBNUC loc
       TWT     *                SYSTWT in IBNUC
       END     RSCQ
