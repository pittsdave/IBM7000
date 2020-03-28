     *
     *  99 BOTTLES OF BEER
     *
     GO        ZA3    COUNT     GET COUNT
               XL     1,CONVERT
     NEXT      ENB    1,CONDATA	CONVERT NUMBER FOR PRINT
               ZA1    CONDATA+2(6,9)
               ST1    BOT1      STORE DIGITS IN MESSAGE
               ST1    BOT2
               UW     3,BOTTLES PRINT MESSAGE
               HB     *
               NOP
               S3     ONE       DECREMENT COUNT
               BZ3    LAST      ZERO?
               ST3    COUNT
               B      NEXT      NO, NEXT BOTTLE
     *
     LAST      ZA2    NO        LAST BOTTLE, MOVE NO
               ST2    BOT1
               ST2    BOT2
	       XL     1,DOWHAT  MOVE GO GET MORE
               RG     1,NOMORE
               UW     3,BOTTLES PRINT MESSAGE
               HB     *
               NOP
               HB     *
     *
     COUNT     DC     +99
     ONE       DC     +1
     *
     CONVERT   DRDW   -COUNT,COUNT
     CONDATA   DRDW   -*+1,*+2
               DC     0
                      0
     *
     NO        DC     @NO@
     NOMORE    DC     -RDW
               DC     @, GO TO THE STORE AND GET SOME MORE.@
     DOWHAT    DRDW   -DOMSG,DOEND
     *
     BOTTLES   DC     -RDW
     BOT1             @00@
                      @ BOTTLES OF BEER ON THE WALL, @
     BOT2             @00@
                      @ BOTTLES OF BEER, @
     DOMSG            @TAKE ONE DOWN AND PASS IT AROUND  @
     DOEND     EQU    *-1
     *
     END       CNTRL  GO
