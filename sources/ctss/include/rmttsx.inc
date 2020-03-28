MTSX   MACRON  NAME,TAG      GENERATE TSX WITH DEFAULT TAG OF 4
       PMC     OFF
       IFF     0,,TAG        IF TAG SPECIFIED,
       TSX     NAME,TAG      ASSEMBLE WITH TAG GIVEN
       IFF     1,,TAG        .. NO TAG SPECIFIED, USE TAG OF 4
       TSX     NAME,4        FOR STANDARD SUBROUTINE CALL
       RMT
       PMC     ON
       IFF     NAME,0,1      IF ARGUMENT IS UNDEFINED, ADD TO RMT
NAME   TIA     =H'NAME
       PMC     OFF
       RMT
MTSX   END                   ..
