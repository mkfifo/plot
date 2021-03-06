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
    (display "pass ") (println s)
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


;; test basic string methods
(if (= (string-length "hello") 5)
  (if (string=? "hello world" (string-copy "hello world"))
    (if (string-ci=? "yes" "YES")
      (pass "one")
      (fail "one case 3"))
    (fail "one case 2"))
  (fail "one case 1"))

(if (= (string-length "hello") 100)
  (fail "two case 1")
  (if (string=? "yes" "nope")
    (fail "two case 2")
    (if (string-ci=? "yes" "nope")
      (fail "two case 3")
      (pass "two"))))

((lambda (str)
  (if (string=? str "hello world")
    (if (= (string-length str) 11)
      (pass "three")
      (fail "three case 2"))
    (fail "three case 1")))
  (string-append "hello" " " "world"))

;; testing 6.3.5 string creation and manipulation procedures
(define four-test-str (make-string 5 #\a))
(string-set! four-test-str 2 #\X)
(if (string=? "ccccc" (make-string 5 #\c))
  (if (= 7 (string-length (make-string 7)))
    (if (char=? (string-ref (string #\a #\a #\a #\Q #\a) 3) #\Q)
      (if (char=? (string-ref four-test-str 2) #\X)
        (if (string=? "abc" (substring "qqqabcqqq" 3 6))
          (pass "four")
          (fail "four case 5"))
        (fail "four case 4"))
      (fail "four case 3"))
    (fail "four case 2"))
  (fail "four case 1"))

;; testing mutability and aliasing of strings
(define five-test-str-1 "hello")
(define five-test-str-2 five-test-str-1)
; five-test-str-1 and  five-test-str-2 now refer to the same storage location
; so modifications to either will be reflected in the other.
(string-set! five-test-str-1 4 #\c)
(if (char=? #\c (string-ref five-test-str-2 4))
  (begin
    (string-set! five-test-str-2 3 #\b)
    (if (char=? #\b (string-ref five-test-str-1 3))
      (pass "five")
      (fail "five case 2")))
  (fail "five case 1"))

;; testing 6.3.5 string procedures
(if (string-ci=? "hello" (list->string (string->list "HeLlO")))
  (if (char=? (string-ref "hello" 1) #\e)
    (if (string=? (make-string 4 #\q) "qqqq")
      (pass "six")
      (fail "six case 3"))
    (fail "six case 2"))
  (fail "six case 1"))

;; A < z, a > z (ascii)
(if #f
(if (string-ci>? "A" "z")
  (if (string-ci=? "hello" (string #\H #\E #\L #\L #\O))
    (pass "seven")
    (fail "seven case 2"))
  (fail "seven case 1"))
)



;; tests completed, print results
(println ">>> string test results")
(results)

