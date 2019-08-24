
This Scheme Interprter is based on Peter Michaux Scheme Interpreter:
https://github.com/petermichaux/bootstrap-scheme/tree/master
-->floats still remain to be added, Garbage Collector is not bug free yet.

Tested with gcc Compiker on Windows.



```
/**Features*/ //
//Automatic Garbage Collector --> not totally bug free yet
/*
> #t
#t
> 1
1
> #\c
#\c
> "aaaaa"
"aaaaa"
> (quote (0 . 1))
(0 . 1)
> (define a 3)
ok
> a
3
> (set! a 1)
ok
> a
1
> (if #t 0 1)
0
> (+ 1 2 3)
6
> ((lambda (x y) (+ x y))
   1 2)
3
> (define (mul x y) (* x y))
ok
> (mul 2 3)
6
> (begin 1 2)
2
> (cond (#f          1)
        ((eq? #t #t) 2)
        (else        3))
2
> (let ((x (+ 1 1))
        (y (- 5 2)))
    (+ x y))
5
*/
```
Object Structure:
![alt text](https://github.com/Johannes0Horn/scheme_interpreter/blob/master/ObjectStructuresScheme.png)
