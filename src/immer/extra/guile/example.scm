;;
;; immer: immutable data structures for C++
;; Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
;;
;; This software is distributed under the Boost Software License, Version 1.0.
;; See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
;;

;; include:intro/start
(use-modules (immer)
             (rnrs base))

(let ((v1 (ivector 1 "hola" 3 'que #:tal)))
  (assert (eq? (ivector-ref v1 3) 'que))

  (let* ((v2 (ivector-set v1 3 'what))
         (v2 (ivector-update v2 2 (lambda (x) (+ 1 x)))))
    (assert (eq? (ivector-ref v1 2) 3))
    (assert (eq? (ivector-ref v1 3) 'que))
    (assert (eq? (ivector-ref v2 2) 4))
    (assert (eq? (ivector-ref v2 3) 'what))

    (let ((v3 (ivector-push v2 "hehe")))
      (assert (eq? (ivector-length v3) 6))
      (assert (eq? (ivector-ref v3 (- (ivector-length v3) 1)) "hehe")))))

(let ((v (apply ivector (iota 10))))
  (assert (eq? (ivector-length v) 10))
  (assert (eq? (ivector-length (ivector-drop v 3)) 7))
  (assert (eq? (ivector-length (ivector-take v 3)) 3))
  (assert (eq? (ivector-length (ivector-append v v)) 20)))

(let ((v1 (make-ivector 3))
      (v2 (make-ivector 3 ":)")))
  (assert (eq? (ivector-ref v1 2)
               (vector-ref (make-vector 3) 2)))
  (assert (eq? (ivector-ref v2 2) ":)")))
;; include:intro/end

;; Experiments

(let ((d (dummy)))
  (dummy-foo d)
  (dummy-bar d 42))
(gc)

(func1)
(func2)
(func3 (dummy) 12)
(foo-func1)
(gc)
