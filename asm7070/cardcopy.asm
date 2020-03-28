     *
     *  CARD TO CARD WITH NEW SEQUENCE NUMBERS
     *
     GO        TYP    HELLO		Type intro.
	       HB     *
	       TYP    LOAD		Load cards message.
	       HB     *
	       HP			Wait for operator.
               ZA3    ONE		Initialize count.
	       ST3    COUNT
     *
     NEXT      UR     1,CARD		Read a card.
               HB     *
               B      DONE
	       XL     1,CVTNUM
	       ENA    1,OUTNUM		Convert number.
	       ZA1    OUTNUM+2(2,9)
	       ST1    SEQUENCE		Set count into sequence.
	       UP     2,CARD		Punch new card.
	       HB     *
	       NOP
               ZA3    COUNT		Increment count.
	       A3     ONE
	       ST3    COUNT
	       B      NEXT
     *
     DONE      TYP    FINISH		Type done.
               HB     *
               HB     *
     *
     CVTNUM    DRDW   -COUNT,COUNT
     OUTNUM    DRDW   -*+1,*+2
               DC     0
	       DC     0
     *
     ONE       DC     +1
     COUNT     DC     0
     *
     HELLO     DC     -RDW
                      @CARD TO CARD COPY@
     *
     LOAD      DC     -RDW
                      @PUT CARDS IN READER, PUSH START@
     *
     FINISH    DC     -RDW
                      @DONE COPYING@
     *
     CARD      DA     1,-RDW
     CARDIMG          000,151@
     SEQUENCE         152,159@
     *
     END       CNTRL  GO
