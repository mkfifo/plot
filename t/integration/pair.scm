;; I do love my new lines
(define println
  (lambda (s) ;; an inline comment :o
    (display s)
    (newline)))

(define test-count 0)
(define tests-passed 0)
(define tests-failed 0)

(define pass
  (lambda (s)
    (display "pass ")
    (println s)
    (set! test-count (+ test-count 1))
    (set! tests-passed (+ tests-passed 1))))

(define fail
  (lambda (s)
    (display "fail ")
    (println s)
    (set! test-count (+ test-count 1))
    (set! tests-failed (+ tests-failed 1))))

(define (results)
  (display tests-passed)
  (display " / ")
  (display test-count)
  (println " tests passed")

  (display tests-failed)
  (println " tests failed")
  (newline)
  ;; indicate failure
  (if (> tests-failed 0)
    (exit 1)))


;; testing 6.3.2 pair functionality
(if (pair? (cons 1 2))
  (if (not (pair? 14))
    (if (= 14 (car (cons 14 0)))
      (if (= 3 (cdr (cons 0 3)))
        (pass "one")
        (fail "one case 4"))
      (fail "one case 3"))
    (fail "one case 2"))
  (fail "one case 1"));

(define tmp-list (cons 1 0))
(set-car! tmp-list 12)
(set-cdr! tmp-list 21)
(if (equal? (car tmp-list) 12)
  (if (equal? (cdr tmp-list) 21)
    (pass "two")
    (fail "two case 2"))
  (fail "two case 1"))

;; basic (list) testing
(if (null? (list))
  (if (null? (cdr (list 1)))
    (if (null? (cdr (cdr (list 1 2))))
      (pass "three")
      (fail "three case 3"))
    (fail "three case 2"))
  (fail "three case 1"))

;; list? testing
(if (list? (list))
  (if (list? (list 1 2 3))
    (if (not (list? #t))
      (pass "four")
      (fail "four case 3"))
    (fail "four case 2"))
  (fail "four case 1"))

;; length testing
(if (= 7 (length (list 1 2 3 4 5 6 7)))
  (if (= 0 (length (list)))
    ;; FIXME this is probably incorrect, may want to throw a runtime error
    (if (= 0 (length #t))
      (pass "five")
      (fail "five case 3"))
    (fail "five case 2"))
  (fail "five case 1"))

(if (= 4 (car (reverse (list 1 2 3 4))))
  (if (= 3 (length (reverse (list 1 2 3))))
    (if (= 9 (car (cdr (reverse (list 5 6 7 8 9 10)))))
      (pass "six")
      (fail "six case 3"))
    (fail "six case 2"))
  (fail "six case 1"))

(define seven (make-list 12 "hhmmmm"))
(if (= 12 (length seven))
  (if (string=? "hhmmmm" (car (cdr (cdr (cdr seven)))))
    (pass "seven")
    (fail "seven case 2"))
  (fail "seven case 1"))

(if (char=? #\o (car (cdr (cdr (cdr (cdr (string->list "hellow")))))))
  (pass "eight")
  (fail "eight"))

(define nine (list 1 2 3 4))
(if (equal? (car (cdr (cdr nine))) (car (cdr (cdr (list-copy nine)))))
  (pass "nine")
  (fail "nine"))

(if (equal?
      (car (cdr (cdr '(1 2 3 4))))
      (car (cdr (cdr (append '(1) '(2 3 4))))))
  (pass "ten")
  (fail "ten"))

(define eleven '(0 1 2 3))
(list-set! eleven 0 'a)
(list-set! eleven 3 'c)
(if (equal? (car eleven) 'a)
  (if (equal? (list-ref eleven 3) 'c)
    (if (equal? (list-ref eleven 1) 1)
      (pass "eleven")
      (fail "eleven case 3"))
    (fail "eleven case 2"))
  (fail "eleven case 1"))

(define twelve '(0 1 2 3 4 5))
(if (equal? 4 (length (list-tail twelve 2)))
  (pass "twelve")
  (fail "twelve"))

;; testing member
(define thirteen '(0 1 2 3))
; member func that will always return f
; this means member will always fail
(define (thirteen-f a b)
  #f)

(if (equal? 1 (car (member 1 thirteen)))
  (if (equal? 2 (length (member 2 thirteen)))
    (if (not (pair? (member 4 thirteen)))
      (if (not (pair? (member 0 thirteen thirteen-f)))
        (if (equal? 2 (length (memv 2 thirteen)))
          (if (equal? 3 (length (memq 1 thirteen)))
            (pass "thirteen")
            (fail "thirteen case 6"))
          (fail "thirteen case 5"))
        (fail "thirteen case 4"))
      (fail "thirteen case 3"))
    (fail "thirteen case 2"))
  (fail "thirteen case 1"))

;; testing assoc
(define fourteen '((0 #\a) (1 #\b) (2 #\c) (3 #\d)))
; assoc func that will always return f
; this means assoc will always fail
(define (fourteen-f a b)
  #f)

(if (equal? #\b (car(cdr (assoc 1 fourteen))))
  (if (equal? 2 (length (assoc 0 fourteen)))
    (if (not (pair? (assoc 4 fourteen)))
      (if (not (pair? (assoc 1 fourteen fourteen-f)))
        (if (equal? #\d (car (cdr (assv 3 fourteen))))
          (if (equal? 2 (length (assq 0 fourteen)))
            (pass "fourteen")
            (fail "fourteen case 6"))
          (fail "fourteen case 5"))
        (fail "fourteen case 4"))
      (fail "fourteen  case 3"))
    (fail "fourteen case 2"))
  (fail "fourteen case 1"))

;; more thorough testing of assoc family now that we have quoting
(define fifteen
  '((a 0)
    (b 1)
    (c 2)))

(if (equal? '(b 1) (assq 'b fifteen))
  (if (equal? #f (assv 'z fifteen))
    (if (equal? '(c 2) (assoc 'c fifteen))
      (pass "fifteen")
      (fail "fifteen case 3"))
    (fail "fifteen case 2"))
  (fail "fifteen case 1"))


;; more thorough testing of member family now that we have quoting
(define sixteen '(a b #f))

(if (equal? '(a b #f) (member 'a sixteen))
  (if (equal? #f (memv 'z sixteen))
    (if (equal? '(#f) (memq #f sixteen))
      (pass "sixteen")
      (fail "sixteen case 3"))
    (fail "sixteen case 2"))
  (fail "sixteen case 1"))


(define (seventeen-filter pred lst)
  (define new-lst '())
  (for-each (lambda (l)
    (if (pred l)
      (set! new-lst (cons l new-lst)))) lst)
  (reverse new-lst))

(define (seventeen-odd x)
  (= (remainder x 2) 1))

(if (equal?
      '(1 3 5)
      (seventeen-filter seventeen-odd '(0 1 2 3 4 5 6)))
  (pass "seventeen")
  (fail "seventeen"))


;; tests completed, print results
(println ">>> pair test results")
(results)

