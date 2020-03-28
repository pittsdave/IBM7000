               TITLE  LOADER - SYSTEM ONE CARD LOADER                  LOADER001
     *                                                                 LOADER002
     * THIS LOADER IS SHOWN ON PAGE 33 OF THE IBM 705 AUTOCODER MANUAL LOADER003
     * IBM NUMBER: 22-6726-1 - 705 AUTOCODER SYSTEM                    LOADER003
     *                                                                 LOADER004
               LASN   @0                                               LOADER005
     NEXT      SEL    100                 0004    SEL     0100  20100  LOADER006
               NOP    18                  0009    NOP     0018  A0018  LOADER007
               SET   62                   0014    SET 6   0002  B0 -2  LOADER008
               SET   74                   0019    SET 7   0004  B0 +4  LOADER009
               RD     BUFFER              0024    RD      0080  Y0080  LOADER010
               TRA    STOP                0029    TRA     0079  I0079  LOADER011
               LOD   6COLUMNS             0034    LOD 6   0094  80 R4  LOADER012
               TRZ   6CODE+4              0039    TRZ 6   0099  N0 R9  LOADER013
               UNL   6LENGTH              0044    UNL 6   0059  70 N9  LOADER014
               RCV    WHERE-3             0049    RCV     0061  U0061  LOADER015
               TMT   7ADDRESS             0054    TMT 7   0089  90  9  LOADER016
     LENGTH    SET   8*-*                 0059    SET 8   0000  B0-00  LOADER017
     WHERE     RCV    *-*                 0064    RCV     0000  U0000  LOADER018
               TMT   8CODE                0069    TMT 8   0095  90-95  LOADER019
               TR     NEXT                0074    TR      0004  10004  LOADER020
     STOP      HLT    1000                0079    HLT     1000  J1000  LOADER021
     *                                                                 LOADER022
     BUFFER    RCD                                                     LOADER023
     IDENT           6A                                                LOADER024
     SERIAL          3A                                                LOADER025
     ADDRESS         4A                                                LOADER026
     COLUMNS         2N                                                LOADER027
     CODE           65A                                                LOADER028
               END                                                     LOADER029
