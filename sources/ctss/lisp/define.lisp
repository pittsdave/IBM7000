
EVAL ((PROG (TEMPDEF1)
  (SETQ TEMPDEF1
    (QUOTE
     (LAMBDA (OB PNAME PVAL)
      (PROG NIL
       (RPLACA (PROP
                OB
                PNAME
                (FUNCTION
                 (LAMBDA NIL
                  (CDDR (RPLACD OB (CONS PNAME (CONS NIL (CDR OB))))))))
               PVAL)
       (RETURN OB)))))
  (APPLY TEMPDEF1 (LIST (QUOTE DEF1) (QUOTE EXPR) TEMPDEF1))
))


def1 (deflist expr 
      (LAMBDA (L PRO) 
        (maplist l 
                 (function
                  (lambda (j)
                  (def1 (caar j) pro (cadar j)))))))

def1 (DEFine expr (LAMBDA (J) (DEFlist J (QUOTE EXPR))))

define ((
  (member (lambda (x l)
            (prog ()
              loop (cond ((null l) (return nil))
                         ((eq (car l) x) (return t)))
                   (setq l (cdr l))
                   (go loop))))
  (printprop (lambda (a)
    (prog ()
      (setq a (cdr a))
      loop (cond ((null a) (return t)))
           (print (car a))
           (cond ((or (eq (car a) (quote pname))
                      (eq (car a) (quote subr))
                      (eq (car a) (quote fsubr)))
                  (prog ()
                    (print (quote internalformat))
                    (setq a (cdr a)))))
           (setq a (cdr a))
           (go loop))))
  (flag1 (lambda (sym fl)
           (prog ()
             (cond ((not (member fl (cdr sym)))
                    (rplacd sym (cons fl (cdr sym))))))))
  (flag (lambda (l fl)
          (prog ()
            loop (cond ((null l) (return nil)))
                 (flag1 (car l) fl)
                 (setq l (cdr l))
                 (go loop))))
  (rem1flag (lambda (l fl)
              (prog ()
                loop (cond ((null l) (return nil))
                           ((eq (cadr l) fl)
                            (prog ()
                              (rplacd l (cddr l))
                              (return nil))))
                     (setq l (cdr l))
                     (go loop))))
  (remflag (lambda (l fl)
             (prog ()
               loop (cond ((null l) (return nil)))
                    (rem1flag (car l) fl)
                    (setq l (cdr l))
                    (go loop))))
  (trace (lambda (l) (flag l (quote trace))))
  (untrace (lambda (l) (remflag l (quote trace))))
  (cset (lambda (sym val)
          (prog (tostore)
            (remprop sym (quote apval))
            (setq tostore (list val))
            (attrib sym (list (quote apval) tostore))
            (return tostore))))
  (opdefine (lambda (x)
              (prog ()
                loop (cond ((null x) (return nil)))
                     (remprop (caar x) (quote sym))
                     (attrib (caar x) (list (quote sym) (cadar x)))
                     (setq x (cdr x))
                     (go loop))))
))

deflist ((
  (conc (lambda (args alist)
          (mapcon args 
                  (function 
                    (lambda (l) 
                      (eval (car l) alist))))))
  (csetq (lambda (args alist)
           (cset (car args) (eval (cadr args) alist))))
             
) fexpr)

STOP
