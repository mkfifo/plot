(define port (open-output-file "test"))
(display "hello")
(newline)
(display "hello" port)
(newline port)
(close-port port)
