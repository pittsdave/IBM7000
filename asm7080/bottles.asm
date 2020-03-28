               TITLE  99 BOTTLES OF BEER
     *
     GO        SEL    400       SELECT THE PRINTER
               SET   22         SET TWO DIGITS OF SIGNIFICANCE.
               LOD   2COUNT     GET THE COUNT
     NEXT      UNL   2BOT1      PUT INTO MESSAGE
               SET   22         *** WHY AGAIN ? ***
               UNL   2BOT2
               WR     BOTTLES   PRINT MESSAGE
               SUB   2ONE       DECREMENT COUNT
               TRZ   2LAST      ZERO?
               TR     NEXT      NO, NEXT BOTTLE
     LAST      RCV    BOT1-1    LAST BOTTLE MOVE NO
               TMT   2NO
               RCV    BOT2-1
               TMT   2NO
               SET   334        SET GO TO STORE
               RCV    DOWHAT
               TMT   3NOMORE
               WR     BOTTLES   PRINT THE MESSAGE
               HLT    1
     *
     COUNT     CON   2+99
     ONE       CON   2+01
     NO        CON   2NO
     NOMORE    CON  34GO TO THE STORE AND GET SOME MORE.
     *
     BOTTLES   RCD  
     BOT1      CON   200
               CON  30 BOTTLES OF BEER ON THE WALL, 
     BOT2      CON   200
               CON  18 BOTTLES OF BEER,  
     DOWHAT    CON  32TAKE ONE DOWN AND PASS IT AROUND
     *
               END
