DEFINE ((
   (FACTORIAL (LAMBDA (X) 
                 (COND ((EQUAL X 0.0) 1.0)
                         (T (TIMES X (FACTORIAL (SUB1 X))))))
    )
))

FACTORIAL (33.0)
STOP
