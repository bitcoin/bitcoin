(defproject immer-benchmarks "0.1.0-SNAPSHOT"
  :description "Benchmarks for immutable data structures"
  :url "http://example.com/FIXME"
  :dependencies [[org.clojure/clojure "1.8.0"]
                 [org.clojure/core.rrb-vector "0.0.11"]
                 [criterium "0.4.4"]]
  :plugins [[lein-git-deps "0.0.1-SNAPSHOT"]]
  ;;:git-dependencies [["https://github.com/clojure/core.rrb-vector.git"]]
  :source-paths [;;".lein-git-deps/core.rrb-vector/src/main/clojure"
                 "src"]

  :main immer-benchmark)
