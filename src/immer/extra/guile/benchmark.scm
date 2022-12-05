;;
;; immer: immutable data structures for C++
;; Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
;;
;; This software is distributed under the Boost Software License, Version 1.0.
;; See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
;;

(use-modules (immer)
             (fector) ; https://wingolog.org/pub/fector.scm
             (srfi srfi-1)
             (srfi srfi-43)
             (ice-9 vlist)
             (ice-9 pretty-print)
             (rnrs bytevectors))

(define-syntax display-eval
  (syntax-rules ()
    ((_ expr)
     (begin (pretty-print 'expr
                          #:max-expr-width 72)
            expr))))

(display-eval (define bench-size 1000000))
(display-eval (define bench-samples 10))

(define (average . ns)
  (/ (apply + ns) (length ns)))

(define (generate-n n fn)
  (unfold (lambda (x) (= x n))
          (lambda (x) (fn))
          (lambda (x) (+ x 1))
          0))

(define-syntax benchmark
  (syntax-rules ()
    ((_ expr)
     (begin
       (display "; evaluating:    ") (newline)
       (pretty-print 'expr
                     #:max-expr-width 72
                     #:per-line-prefix "      ")
       (let* ((sample (lambda ()
                        (gc)
                        (let* ((t0 (get-internal-real-time))
                               (r  expr)
                               (t1 (get-internal-real-time)))
                          (/ (- t1 t0) internal-time-units-per-second))))
              (samples (generate-n bench-samples sample))
              (result (apply average samples)))
         (display "; average time: ")
         (display (exact->inexact result))
         (display " seconds")
         (newline))))))

(display ";;;; benchmarking creation...") (newline)

(display-eval
 (define (fector . args)
   (persistent-fector (fold (lambda (v fv) (fector-push! fv v))
                            (transient-fector)
                            args))))

(benchmark (apply ivector (iota bench-size)))
(benchmark (apply ivector-u32 (iota bench-size)))
(benchmark (iota bench-size))
(benchmark (apply vector (iota bench-size)))
(benchmark (apply u32vector (iota bench-size)))
(benchmark (list->vlist (iota bench-size)))
(benchmark (apply fector (iota bench-size)))

(display ";;;; benchmarking iteration...") (newline)

(display-eval (define bench-ivector (apply ivector (iota bench-size))))
(display-eval (define bench-ivector-u32 (apply ivector-u32 (iota bench-size))))
(display-eval (define bench-list (iota bench-size)))
(display-eval (define bench-vector (apply vector (iota bench-size))))
(display-eval (define bench-u32vector (apply u32vector (iota bench-size))))
(display-eval (define bench-vlist (list->vlist (iota bench-size))))
(display-eval (define bench-fector (apply fector (iota bench-size))))
(display-eval (define bench-bytevector-u32
                (uint-list->bytevector (iota bench-size)
                                       (native-endianness)
                                       4)))

(benchmark (ivector-fold + 0 bench-ivector))
(benchmark (ivector-u32-fold + 0 bench-ivector-u32))
(benchmark (fold + 0 bench-list))
(benchmark (vector-fold + 0 bench-vector))
(benchmark (vlist-fold + 0 bench-vlist))
(benchmark (fector-fold + bench-fector 0))

(display ";;;; benchmarking iteration by index...") (newline)

(display-eval
 (define-syntax iteration-by-index
   (syntax-rules ()
     ((_ *length *ref *vector *step)
      (let ((len (*length *vector)))
        (let iter ((i 0) (acc 0))
          (if (< i len)
              (iter (+ i *step)
                    (+ acc (*ref *vector i)))
              acc)))))))

(display-eval
 (define-syntax iteration-by-index-truncate
   (syntax-rules ()
     ((_ *length *ref *vector *step)
      (let ((len (*length *vector)))
        (let iter ((i 0) (acc 0))
          (if (< i len)
              (iter (+ i *step)
                    (logand #xffffffffFFFFFFFF
                            (+ acc (*ref *vector i))))
              acc)))))))

(benchmark (iteration-by-index ivector-length
                               ivector-ref
                               bench-ivector 1))
(benchmark (iteration-by-index ivector-u32-length
                               ivector-u32-ref
                               bench-ivector-u32 1))
(benchmark (iteration-by-index vector-length
                               vector-ref
                               bench-vector 1))
(benchmark (iteration-by-index u32vector-length
                               u32vector-ref
                               bench-u32vector 1))
(benchmark (iteration-by-index vlist-length
                               vlist-ref
                               bench-vlist 1))
(benchmark (iteration-by-index fector-length
                               fector-ref
                               bench-fector 1))
(benchmark (iteration-by-index bytevector-length
                               bytevector-u32-native-ref
                               bench-bytevector-u32 4))

(benchmark (iteration-by-index-truncate ivector-length
                                        ivector-ref
                                        bench-ivector 1))
(benchmark (iteration-by-index-truncate ivector-u32-length
                                        ivector-u32-ref
                                        bench-ivector-u32 1))
(benchmark (iteration-by-index-truncate vector-length
                                        vector-ref
                                        bench-vector 1))
(benchmark (iteration-by-index-truncate u32vector-length
                                        u32vector-ref
                                        bench-u32vector 1))
(benchmark (iteration-by-index-truncate vlist-length
                                        vlist-ref
                                        bench-vlist 1))
(benchmark (iteration-by-index-truncate fector-length
                                        fector-ref
                                        bench-fector 1))
(benchmark (iteration-by-index-truncate bytevector-length
                                        bytevector-u32-native-ref
                                        bench-bytevector-u32 4))

(display ";;;; benchmarking concatenation...") (newline)
(benchmark (ivector-append bench-ivector bench-ivector))
(benchmark (ivector-u32-append bench-ivector-u32 bench-ivector-u32))
(benchmark (append bench-list bench-list))
(benchmark (vector-append bench-vector bench-vector))
(benchmark (vlist-append bench-vlist bench-vlist))
