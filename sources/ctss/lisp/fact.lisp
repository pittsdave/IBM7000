DEFINE ((
   (FACTORIAL (LAMBDA (X) 
                 (COND ((EQUAL X 0) 1)
                         (T (TIMES X (FACTORIAL (SUB1 X))))))
    )
))
FACTORIAL (10)
STOP
