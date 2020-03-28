..     EQU     **            MODIFICATION SYMBOL.     
SETF   OPSYN   STL           SET FLAG.    
CLEAR  OPSYN   STZ           CLEAR FLAG.  
PAR    OPSYN   BRN           PARAMETER POINTER. 
BLK    OPSYN   TIX           BLOCK POINTER.     
FMT    OPD     500011120000  FORMAT POINTER.    
IOP    OPD     500011120000  I/O PARAMETER POINTER.   
ENDIO  OPSYN   STR           END OF I/O LIST.   
SHOW   OPSYN   PMC           SAVE PRESENT DEFINITION OF PMC.
*     
TSS    MACRO   A             CALL SUPERVISOR SUBROUTINE.    
       GEN     ,TSX(A,4)     GENERATE CALL INSTRUCTION.     
       RMT  
       IFF     A,X           IF ARGUMENT IS UNDEFINED,
       GEN     (A),TIA(=H'A) INSERT THE TRAP INSTRUCTION.   
       RMT  
TSS    END
*     
TYPE   MACRO   N,COM,LOC     TYPE COMMENT.
       GEN     ,TSX(TYPE,4)  CALL SUBPROGRAM.   
       GEN     ,PAR(LOC)     POINTER TO BCI STRING.   
       RMT  
       IFF     LOC,X         IF LOCATION SYMBOL UNDEFINED, GENERATE BCI.
       GEN     LOC,BCI(N,COM) GENERATE THE BCI STRING.
       RMT  
TYPE   END
*     
TYPA   MACRO   N,COM,LOC
       GEN     ,TSX(TYPA,4)  CALL SUBPROGRAM.   
       GEN     ,PAR(LOC)     POINTER TO BCI STRING.   
       RMT  
       IFF     LOC,X         IF LOCATION SYMBOL UNDEFINED, GENERATE BCI.
       GEN     LOC,BCI(N,COM) GENERATE THE BCI STRING.
       RMT  
TYPA   END
*     
GEN    MACRO   LOC,OP,AD     SURROUND GENERATED WORD WITH PMC ON, OFF.  
       PMC     ON            PRINT THE WORD.    
LOC    OP      AD            ..     
       PMC     OFF           RETURN TO NORMAL MODE.   
GEN    END
*     
OUTSUB MACRO   NAME          DEFINE SUBROUTINE CALL.  
       IRP     NAME          CONSIDER EACH NAME IN LIST.    
NAME   MACRO   ARG           DEFINE IT.   
       GEN     ,CLA(ARG)     GENERATE A CLA.    
       GEN     ,TSX(NAME,4)  CALL THE SUBROUTINE.     
NAME   END                   THAT'S IT FOR THIS DEFINITION. 
       IRP                   ANY MORE.Q   
OUTSUB END                   NO.    
*     
       OUTSUB  (TYO,BCDOUT,NAMOUT,INTOUT,OCTOUT,FLDPRT)     
*     
TEQ    MACRO   LOC,SAMPLE    TRANSFER IF EQUAL. 
       GEN     ,LAS(SAMPLE)   COMPARE C(AC) WITH C(SAMPLE). 
       GEN     ,TRA(*+2)     C(AC) .NE. C(SAMPLE), SKIP.    
       GEN     ,TRA(LOC)     C(AC) .E. C(SAMPLE), TRANSFER. 
TEQ    END
*     
TNE    MACRO   LOC,SAMPLE    TRANSFER IF NOT EQUAL.   
       TEQ     *+2,(SAMPLE)  IF C(AC) .E. C(SAMPLE), SKIP.  
       GEN     ,TRA(LOC)     C(AC) .NE. C(SAMPLE), TRANSFER.
TNE    END
*     
TBL    MACRO   LOC           TRANSFER IF BLANK. 
       TEQ     (LOC),=H00000 ..     
TBL    END
*     
TNB    MACRO   LOC           TRANSFER IF NOT BLANK.   
       TNE     (LOC),=H00000 ..     
TNB    END
*     
TCR    MACRO   LOC           TRANSFER IF CARRIAGE RETURN.   
       TEQ     (LOC),=O55    ..     
TCR    END
*     
TNCR   MACRO   LOC           TRANSFER IF NOT CARRIAGE RETURN.     
       TNE     (LOC),=O55    ..     
TNCR   END
*     
MCALL  MACRO   NAME,ARG      GENERATE MAD FUNCTION CALL.    
       TSS     NAME          CALL THE FUNCTION. 
       IRP     ARG           EXPAND THE ARGUMENT LIST.
       PTR     ARG           ..     
       IRP  
MCALL  END
*     
PTR    MACRO   A,B,C         GENERATE ARGUMENT POINTER.     
...    SET     *             REMEMBER CURRENT LOCATION.     
       IFF     1,...,B       IS B '...'.Q 
       GEN     ,BLK(A,,C)    YES, USE MAD BLOCK NOTATION.   
       IFF     1,,B          IS B VOID.Q  
       GEN     ,PAR(A,,C)    YES, USE FAP BLOCK NOTATION.   
       IFF     *-...,X       DID EITHER OF THE ABOVE TURN OUT TRUE.Q    
       GEN     ,PAR(A)       NO, USE SINGLE ARGUMENT POINTER.     
PTR    END
*     
FNC    MACRO   NAME          DEFINE FUNCTION NAME.    
       IRP     NAME          CONSIDER EACH NAME IN LIST.    
NAME   MACRO   ARG           DEFINE IT..  
       MCALL   NAME(ARG)
NAME   END
       IRP  
FNC    END
*     
       FNC     (OPEN,BUFFER,RDWAIT,CLOSE,IODIAG)
