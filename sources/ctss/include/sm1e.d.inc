       EJECT                                                            SM1E0001
       REM     DEFINITION OF ADDITIONAL ACTIVE FILE STATUS TABLE ELEMENTSM1E0002
       REM                                                              SM1E0003
NXTREC SYN     SMTMP1 (A)    NEXT RECORD FOR TRAP PROCESSOR             SM1E0004
LSTREC SYN     SMTMP2 (A)    POSITION OF LAST RECORD IN PROCESS         SM1E0005
THSTRK SYN     SMTMP1 (L)    POSITION OF THIS TRACK                     SM1E0006
LSTTRK SYN     SMTMP2 (L)    POSITION OF LAST TRACK                     SM1E0007
EOFSW  SYN     SMTMP1 (T)    NON-ZERO ON LAST TRACK                     SM1E0008
SEQSW  SYN     SMTMP2 (T)    NON-ZERO WHEN POINTER CHECKING IS SUSPENDEDSM1E0009
       REM                                                              SM1E0010
       REM                                                              SM1E0011
       REM     DEFINITION OF ADDITIONAL QUEUE COMPONENTS                SM1E0012
       REM                                                              SM1E0013
QEOF   EQU     QPRF1 (P)     INDICATE LAST RECORD WILL INCLUDE E.O.F.   SM1E0014
       REM                                                              SM1E0015
TRQST  EQU     QWORDS (W)    POSITION OF REQUESTED FILE ON SEARCH       SM1E0016
       REM                                                              SM1E0017
QLABEL EQU     QWORDS (W)    RELATIVE LOCATION OF REC. ID               SM1E0018
QLIST  EQU     QWORDS+1 (W)  START OF I/O LIST IN QUEUE BEAD            SM1E0019
       REM                                                              SM1E0020
       REM                                                              SM1E0021
       REM     DEFINITION OF QUEUE COMPONENTS FOR DELETE RING           SM1E0022
       REM                                                              SM1E0023
DELTRQ EQU     0 (W)         DELETE REQUEST IN DELETE QUEUE             SM1E0024
DELFRP EQU     1 (W)         FORWARD POINTER FOR VALIDITY CHECK         SM1E0025
DELREC EQU     2 (A)         DELETED RECORD IDENTIFICATION              SM1E0026
DELFIN EQU     2 (D)         SEQ. NO. OF LAST RECORD TO DELETE          SM1E0027
DLFNSW EQU     2 (T)         TESTED BY WDELET                           SM1E0028
       REM                                                              SM1E0029
       REM                                                              SM1E0030
       REM     PARAMETERS ASSOCIATED WITH DELETE RING                   SM1E0031
       REM                                                              SM1E0032
DELNUM EQU     DELFIN+1      NUMBER OF WORDS USED BY DELETE REQUEST     SM1E0033
       REM                                                              SM1E0034
DELACT EQU     20            MAXIMUM NUMBER OF ACTIVE DELETE REQUESTS   SM1E0035
       REM                                                              SM1E0036
DELLNG EQU     DELNUM*DELACT DEFINE LENGTH OF DELETE RING               SM1E0037
       REM                                                              SM1E0038
       REM                                                              SM1E0039
       REM     DEFINITION OF QUEUE FUNCTION CODES                       SM1E0040
       REM                                                              SM1E0041
CRANKP EQU     0             CRANK UP FROM SCRATCH                      SM1E0042
BEGINF EQU     1             POSITION FILE AT BEGINNING                 SM1E0043
ENDF   EQU     2             POSITION FILE AT END                       SM1E0044
SRCFWD EQU     3             MOVE FILE FORWARD AND SEARCH               SM1E0045
SRCBKW EQU     4             MOVE FILE BACKWARD AND SEARCH (FIRST REC.) SM1E0046
READF  EQU     5             READ THE FILE                              SM1E0047
WRITEF EQU     6             WRITE THE FILE                             SM1E0048
REWRTF EQU     7             READ PREFIX FOR RE-WRITE                   SM1E0049
RWRT1  EQU     8             WRITE HALF OF RE-WRITE                     SM1E0050
SRCBK. EQU     9             SUBSEQUENT RECORDS OF BACKWARD SEARCH      SM1E0051
       REM                                                              SM1E0052
       REM                                                              SM1E0053
       REM     DEFINE LENGTH OF DRUM/DISC RECORDS                       SM1E0054
       REM                                                              SM1E0055
NWDSPR EQU     432           NUMBER OF WORDS/RECORD, DISK/DRUM          SM1E0056
       REM                                                              SM1E0057
       REM                                                              SM1E0058
       REM     NUMBER OF I/O COMMANDS IN IOLIST (INCL. 'IOD')           SM1E0059
       REM                                                              SM1E0060
MAXLST EQU     4             MAXIMUM NUMBER OF I/O COMMANDS             SM1E0061
       REM                                                              SM1E0062
       REM                                                              SM1E0063
       REM     DECREMENT DEFINITION FOR INDIRECT ADDRESSING             SM1E0064
       REM                                                              SM1E0065
II     BOOL    60            INDIRECT FLAG IN DECREMENT                 SM1E0066
       REM                                                              SM1E0067
       REM                                                              SM1E0068
       REM     STATUS-BIT DEFINITIONS IN RWSTAT                         SM1E0069
       REM                                                              SM1E0070
W.STAT BOOL    200000        FILE IN WRITE-STATUS                       SM1E0071
R.STAT BOOL    100000        FILE IN  READ-STATUS                       SM1E0072