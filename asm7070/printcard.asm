     *
     *  CARD TO PRINTER
     *
     GO        TYP    HELLO		Type intro.
               HB     *
	       TYP    LOAD		Load cards message.
	       HB     *
	       HP			Wait for operator.
               ZA3    ONE		Initialize counts.
               ST3    COUNT
	       ST3    PAGE
               MSA    SPACER
               ZA2    SPACE		Set space in line.
               ST2    SPACER
	       BLX    32,HEADER     	Print heading.
     *
     NEXT      UR     1,CARD		Read a card.
               HB     *
               B      DONE		EOF.
               ZA1    COUNT		Set count into print line.
	       BLX    33,FMTNUM		Format the number.
	       ST1    SEQUENCE
	       UW     3,PRINT		Print line.
	       HB     *
	       BLX    32,HEADER	        New page.
               ZA3    COUNT		Increment count.
	       A3     ONE
	       ST3    COUNT
	       B      NEXT
     *
     DONE      TYP    FINISH		Type done.
               HB     *
               HB     *
	       EJECT
     *
     * Page header subroutine.
     *  Uses AC1, X1, X32 and X33.
     *
     HEADER    ZA1    PAGE		Get page number.
	       BLX    33,FMTNUM		Format for printing.
	       ST1    PAGENUM		Put into heading.
               UW     3,HEADING
               HB     *
	       NOP
               UW     3,SPACES         
               HB     *
               NOP
	       ZA1    PAGE
	       A1     ONE
	       ST1    PAGE
               B      0+X32
     *
     * Format number subroutine.
     *  Uses AC1, X1 and X33.
     *
     FMTNUM    ST1    CVTNUM+1
	       XL     1,CVTNUM
	       ENA    1,OUTNUM
	       ZA1    OUTNUM+2(2,9)
               B      0+X33
	       EJECT
     *
     CVTNUM    DRDW   -*+1,*+1
               DC     0
     OUTNUM    DRDW   -*+1,*+2
               DC     0
	       DC     0
     *
     SPACE     DC     @ @
     ONE       DC     +1
     COUNT     DC     0
     PAGE      DC     0
     *
     HEADING   DC     -RDW
                      @CARD TO PRINTER COPY          @
                      @                              @
		      @                     PAGE @
     PAGENUM          @    @
     SPACES    DC     -RDW
                      @                             @
     *
     HELLO     DC     -RDW
                      @CARD TO PRINTER COPY@
     *
     LOAD      DC     -RDW
                      @PUT CARDS IN READER, PUSH START@
     *
     FINISH    DC     -RDW
                      @DONE COPYING@
     *
     CARD      DRDW   -CARDIMG,CARDEND
     PRINT     DA     1,-RDW
     SEQUENCE         000,007@
     SPACER           008,009@
     CARDIMG          010,169@
     CARDEND   EQU    *-1
     *
     END       CNTRL  GO
