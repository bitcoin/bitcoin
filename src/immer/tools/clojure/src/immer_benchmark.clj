(ns immer-benchmark
  (:require [criterium.core :as c]
            [clojure.core.rrb-vector :as fv]))

(defn h0 [& args]
  (apply println "\n####" args))

(defn h1 [& args]
  (apply println "\n====" args))

(defn h2 [& args]
  (apply println "\n----" args))

(defn run-benchmarks [N S]
  (h0 "Running benchmarks:  N =" N " S =" S)

  (def c-steps 10)

  (defn vector-push-f [v]
    (loop [v v
           i 0]
      (if (< i N)
        (recur (fv/catvec (fv/vector i) v)
               (inc i))
        v)))

  (defn vector-push [v]
    (loop [v v
           i 0]
      (if (< i N)
        (recur (conj v i)
               (inc i))
        v)))

  (defn vector-push! [v]
    (loop [v (transient v)
           i 0]
      (if (< i N)
        (recur (conj! v i)
               (inc i))
        (persistent! v))))

  (defn vector-concat [v vc]
    (loop [v v
           i 0]
      (if (< i c-steps)
        (recur (fv/catvec v vc)
               (inc i))
        v)))

  (defn vector-update [v]
    (loop [v v
           i 0]
      (if (< i N)
        (recur (assoc v i (+ i 1))
               (inc i))
        v)))

  (defn vector-update-random [v]
    (loop [v v
           i 0]
      (if (< i N)
        (recur (assoc v (rand-int N) i)
               (inc i))
        v)))

  (defn vector-update! [v]
    (loop [v (transient v)
           i 0]
      (if (< i N)
        (recur (assoc! v i (+ i 1))
               (inc i))
        (persistent! v))))

  (defn vector-update-random! [v]
    (loop [v (transient v)
           i 0]
      (if (< i N)
        (recur (assoc! v (rand-int N) i)
               (inc i))
        (persistent! v))))

  (defn vector-iter [v]
    (reduce + 0 v))

  (defn the-benchmarks [empty-v]
    (def full-v (into empty-v (range N)))
    (h2 "iter")
    (c/bench (vector-iter full-v) :samples S)
    (if (> N 100000)
      (h2 "skipping updates 'cuz N > 100000 (would run out of mem)")
      (do
        (h2 "update!")
        (c/bench (vector-update! full-v) :samples S)
        (h2 "update-random!")
        (c/bench (vector-update-random! full-v) :samples S)
        (h2 "push!")
        (c/bench (vector-push! empty-v) :samples S)
        (h2 "update")
        (c/bench (vector-update full-v) :samples S)
        (h2 "update-random")
        (c/bench (vector-update-random full-v) :samples S)
        (h2 "push")
        (c/bench (vector-push empty-v) :samples S))))

  (defn the-rrb-benchmarks [empty-v]
    (if (> N 1000)
      (h2 "skipping relaxed test 'cuz N > 1000 (rrb-vector bug)")
      (do
        (def full-v (vector-push-f empty-v))
        (h2 "iter/f")
        (c/bench (vector-iter full-v) :samples S)
        (h2 "update/f")
        (c/bench (vector-update full-v) :samples S)
        (h2 "update-random/f")
        (c/bench (vector-update-random full-v) :samples S)))
    (def short-v (into empty-v (range (/ N c-steps))))
    (h2 "concat")
    (c/bench (vector-concat empty-v short-v)) :samples S)

  (h1 "vector")
  (the-benchmarks (vector))

  (h1 "rrb-vector")
  (the-benchmarks (fv/vector))
  (the-rrb-benchmarks (fv/vector)))

(defn -main []
  (run-benchmarks 1000 100)
  (run-benchmarks 100000 20)
  (run-benchmarks 10000000 3))
