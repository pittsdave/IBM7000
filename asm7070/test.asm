     *
     * ASM7070 test program. 
     *
0101 ORIGIN    CNTRL  2000                                                 00010
0102 START     ZA1    NUM1                                                 00020
               A1     NUMA
               A1     NUMB
0103           S1     NUM2                                                 00030
               XA     12,NUMS
               ZA2    NUMS+X12
               S2     NUMB+INDEX
               A2     +45
               BAS    2,CH1
               FZA    PI
	       FS     F6
	       FM     F1
	       ZA3    ES3
     * Interval timer
     CH1       ITZ    TIMER
               ITS    TIMER
     * SHIFT
               SR2    3
               SRR3   2
               SL1    7
               SLC2   91
     * COUPLED SHIFT
               SR     12
               SRR    5
               SL     17
               SLC    47
               SRS    2(5)
               SRS    2(15)
               SLS    15(14)
     *
               BES    27,CH1
               ESN    22
               ESF    9
               BSN    20,CH1+X17
               BSF    5,3110
     * Stacking latch
               BAL    1340
               BUL    A,2662
               BQL    2,2946
               BTL    14,1505
               BDCL   1,4154
     * Stacking latch on
               DCLN   1
     * Stacking latch off
               ULF    B
     * Priority release
               PR     97
               PR     2272+X17
     * Unit record
               UR     1,RECORD
               US     2
               UP     1,RECORD
               UPIV   2,RECORD
               TYP    HELLO
     * Tape operations
               PTR    23,1715
               TR     10,2310
               TRR    34,1234
               PTRR   25,2341+X37
               TW     10,3321
               PTW    11,4341
               TWR    25,3635
               PTWR   22,1567
               TWZ    14,1400
               PTWZ   21,0440
               PTWC   41,1311
               TWC    13,1066
               TSF    15,1645
               PTSF   15,1730
               PTSB   12,1414
               TSB    13,3841
               TSEL   23
               TM     24
               PTM    12
               TRW    20
               TRU    10
               TRB    14
               PTSM   23
               TSM    12
               TSK    25
               TEF    11
               TSLD   25
               TSHD   12
     *
01031          HB     *                                                    00031
     BRANCH    CNTRL  CH1
               EJECT
0104 *                                                                     00040
     * Constants
     *
     PI        DC     3.1415926F
     F1        DC     -3.4F
     F2        DC     +34.567893F-2
     F3        DC     -31.92F+7
     F4        DC     -29567.1F-3
     F5        DC     +12546F+15
     F6        DC     +0.000045F
               DC     +0.1F
               DC     +0.01F
               DC     +0.001F
               DC     +0.0001F
     F7        DC     +0.1F,+0.01F,+0.001F,+0.0001F
     F8        DC     +0.1F,+0.1F-1,+0.1F-2,+0.1F-3
     D1        DC     +1,+10,+100,+1000
     D2        DC     +1+10+100+1000
     P2        DC     +1(0,1)+10(2,3)+20(4,5)+30(6,7)+40(8,9)
0105 NUM1      DC     42                                                   00050
0106 NUM2      DC     37                                                   00060
     COUNT     DC     +99(0,2)
     ZERODIG   DC     +90(3,5)
     ONE       DC     +1(6,8)
     HELLO     DC     -RDW
01061TEXT1     DC     @HELLO THERE BIG GUY@                                00061
01062TEXT2     DC     @THIS A TEST OF THE EMERGENCY NETWORK@               00062
01063TEXT3     DC     @ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@  
01064TEXT4     DC     @)g&+$*-/,%(s#=!t^_r@
0107 NUMS      DC     +1+2+3+4+5+6+7+8+9                                   00070
     TIMER     DC     0
     SWITCH    DSW    ES1,-ES2,ES3
     *
     * Areas
     *
     RECORD    DRDW   -RECDATA,RECEND
     RECDATA   DA     4,RDW
     IMAGE            00,151
     SEQUENCE         152,159
     RECEND    EQU    *-1
     *
     INDEXWORD EQU    1,X
     AREA1     DA     1
     		      12,25
               ZA1    AREA1
     *
     AREA2     DA     2
     		      12,25
               ZA1    AREA2
     *
     AREA3     DA     2,RDW
     		      12,25
               ZA1    AREA3
     *
     AREA4     DA     2,RDW,0+INDEXWORD
     		      12,25
               ZA1    AREA4
     *
     AREA5     DA     2,,0+X15
     		      12,25
               ZA1    AREA5
     *
               DA     1
     FIELDA	      12,25
               ZA1    FIELDA
     *
               DA     2
     FIELDB	      12,25
               ZA1    FIELDB
     *
               DA     2,RDW
     FIELDC	      12,25
               ZA1    FIELDC
     *
               DA     2,RDW,0+INDEXWORD
     FIELDD	      12,25
               ZA1    FIELDD
     *
               DA     2,,0+X15
     FIELDE	      12,25
               ZA1    FIELDE
     *
     * Equates
     *
     NUMA      EQU    NUMS(8,9)
     NUMB      EQU    NUMS(8,9)+1
     SW1       EQU    1,S
     SW2       EQU    2,S
     INDEX     EQU    25,X
     *
0108 END       CNTRL                                                       00080
