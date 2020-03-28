LKP    MACRO   LOC,IX,OP
       PMC     ON
       AXT     N'LOC,IX
       LAS     LOC+N'LOC,IX
       TRA     *+2
       OP      1+LOC+N'LOC,IX
       TIX     *-3,IX,2
       PMC     OFF
LKP    END
