       TAPE    SYSTMP,B3
       TAPE    SYSTAP,A4
       TAPE    SYSPOT,A3
       TAPE    SYSPPT,A2
       TEST  FACTORIAL

DEFINE ((
   (FACTORIAL (LAMBDA (X) 
                 (COND ((EQUAL X 0.0) 1.0)
                         (T (TIMES X (FACTORIAL (SUB1 X))))))
    )
))

FACTORIAL (33.0)

STOP)))   )))   )))   )))
       FIN      END OF LISP RUN