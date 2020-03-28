               TITLE  This is a test program of the IBM 7080 assembler.    00001
                      We try out lots of options and pseudo ops.           00002
                                                                           00003
     * This is a test program for the IBM 7080 assembler.                  00004
     *
0101           LASN   @200                                                 00007
     ENTRY     EEM                                                         00008
0102 START     ADD    @300                                                 00009
0103 LABEL1    SUB    I,@304                                               00010
               TRE    *+10                                                 00011
               DOA   1NUM0
     HOUSEKEEP TR     LABEL2
               TR     ZEROCARD
0104 LABEL2    MPY    NUM0                                                 00012
0105           RAD   1@309                                                 00013
0105           RAD    #12345#                                              00014
               ADD    #+314#                                               00015
               MPY    #+00+31415926#                                       00016
               DIV    #+00+3141#                                           00017
               TMTS   #HEY THERE, BIG GUY.#                                00018
0106           BSF                                                         00019
0106           SDH                                                         00020
0106           SDL                                                         00021
0106           HLT                                                         00022
               TCD
               ALTP   NUM0,37
               TR     HOUSEKEEP
               BSTP  46
     ZEROCARD  TR     @4
0107           LASN   @300                                                 00023
     *         LITOR  @6000                                                00024
               TYPE  1TEXT
0106 NUM0      CON   6+314159                                              00025
0106 NUM1      CON   6-314159                                              00026
0106 NUM2            48                                                    00027
0106 NUM3            6-64                                                  00028
0107 *                                                                     00029
0107 TEXT      CON  24THIS IS A TEXT MESSAGE.                              00030
0108 TEXT1          21THE QUICK BROWN FOX.                                 00031
               CHRCD 218
     NEWYORK          10
     BOSTON           06
     CHICAGO          12
     ATLANTA          27
     BUFFER2   ADCON  INBUFFER
     BUFFER3   ACON4  INBUFFER
     BUFFER4   ACON5  OUTBUFFER
     BUFFER4A  ACON5  S,OUTBUFFER-32
     BUFFER5   ACON5 +OUTBUFFER
     BUFFER6   ACON6  TAPEBUFFER
     BUFFER7   ACON6  S,TAPEBUFFER
0107 *                                                                     00032
     FLT1      FPN    -01+314159                                           00033
     FLT2      FPN    -10+314159                                           00034
     FLT3      FPN    +10+314159                                           00035
                      +03+58946782                                         00036
                      -02+25                                               00037
                      +04-43279                                            00038
                      -01-63                                               00039
                      +00-4792                                             00040
                      +05+1748218936                                       00041
0107 *                                                                     00042
0108 INBUFFER  RCD  80A                                                    00043
0108 OUTBUFFER     132A                                                    00044
0109 TAPEBUFFER    132A                                                    00045
     *
     FIELD1    RPT   7XXXZ.ZZ
     FIELD2          5XXXXZ
     FIELD3          9$X,XXZ.ZZ
     *
               CHRCD
     NEW              N
     REGULAR          R
     CANCELED         C
               BITCD  B
     IRS             1
     FICA            2
     STATE           4
     OTHER           A
               LASN   @2000
     COMPARE   CMP   1NEW
               CMP   2NEWYORK
               CMP    IRS
               CMP    FICA
               CHKT  3NUM0,ADDR1
0107           END                                                         00046
