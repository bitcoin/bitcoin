package org.immer

import org.scalameter.api._
import scala.collection.immutable.Vector
import scala.collection.immutable.rrbvector.RRBVector

class BenchmarkBase extends Bench.ForkedTime {
  val runs = 20
  val size = 1000
  val step = 10
  val sizes = Gen.range("size")(size, size, 1)

  //def relaxedV(v : RRBVector[Int], n : Int) : RRBVector[Int] = {
  //  var r = v
  //  for (i <- 0 to n) {
  //    r = (RRBVector.empty[Int] :+ i) ++ r
  //  }
  //  r
  //}

  var vectors     = for { s <- sizes } yield Vector.empty[Int] ++ (0 until s)
  var rrbvectors  = for { s <- sizes } yield RRBVector.empty[Int] ++ (0 until s)
  //var rrbvectorsF = for { s <- sizes } yield relaxedV(RRBVector.empty[Int], s)
  var rrbsteps    = for { s <- sizes } yield RRBVector.empty[Int] ++ (0 until s / step)
}

class PushBenchmark extends BenchmarkBase {
  performance of "push" in {
    measure method "Vector" in { using(sizes) config(exec.benchRuns -> runs) in { s => {
      var v = Vector.empty[Int]
      for (a <- 0 to s) {
        v = v :+ a
      }
      v
    }}}
    measure method "RRBVector" in { using(sizes) config(exec.benchRuns -> runs) in { s => {
      var v = RRBVector.empty[Int]
      for (a <- 0 to s) {
        v = v :+ a
      }
      v
    }}}
  }
}

class UpdateBenchmark extends BenchmarkBase {
  performance of "update" in {
    measure method "Vector" in { using(vectors) config(exec.benchRuns -> runs) in { v0 => {
      var v = v0
      for (a <- 0 to v.size - 1) {
        v = v.updated(a, a + 1)
      }
      v
    }}}
    measure method "RRBVector" in { using(rrbvectors) config(exec.benchRuns -> runs) in { v0 => {
      var v = v0
      for (a <- 0 to v.size - 1) {
        v = v.updated(a, a + 1)
      }
      v
    }}}
    //measure method "RRBVector/relaxed" in { using(rrbvectorsF) config(exec.benchRuns -> runs) in { v0 => {
    //  var v = v0
    //  for (a <- 0 to v.size - 1) {
    //    v = v.updated(a, a + 1)
    //  }
    //  v
    //}}}
  }
  performance of "update/random" in {
    measure method "Vector" in { using(vectors) config(exec.benchRuns -> runs) in { v0 => {
      var v = v0
      var r = new scala.util.Random
      for (a <- 0 to v.size - 1) {
        v = v.updated(r.nextInt(v.size), a + 1)
      }
      v
    }}}
    measure method "RRBVector" in { using(rrbvectors) config(exec.benchRuns -> runs) in { v0 => {
      var v = v0
      var r = new scala.util.Random
      for (a <- 0 to v.size - 1) {
        v = v.updated(r.nextInt(v.size), a + 1)
      }
      v
    }}}
    //measure method "RRBVector/relaxed" in { using(rrbvectorsF) config(exec.benchRuns -> runs) in { v0 => {
    //  var v = v0
    //  var r = new scala.util.Random
    //  for (a <- 0 to v.size - 1) {
    //    v = v.updated(r.nextInt(v.size), a + 1)
    //  }
    //  v
    //}}}
  }
}

object IterBenchmark extends BenchmarkBase {
  performance of "access/reduce" in {
    measure method "Vector" in { using(vectors) config(exec.benchRuns -> runs) in { v => {
      v.reduceLeft(_ + _)
    }}}
    measure method "RRBVector" in { using(rrbvectors) config(exec.benchRuns -> runs) in { v => {
      v.reduceLeft(_ + _)
    }}}
    //measure method "RRBVector/relaxed" in { using(rrbvectorsF) config(exec.benchRuns -> runs) in { v => {
    //  v.reduceLeft(_ + _)
    //}}}
  }
  performance of "access/idx" in {
    measure method "Vector" in { using(vectors) config(exec.benchRuns -> runs) in { v => {
      var r = 0
      for (a <- 0 to v.size - 1) {
        r += v(a)
      }
      r
    }}}
    measure method "RRBVector" in { using(rrbvectors) config(exec.benchRuns -> runs) in { v => {
      var r = 0
      for (a <- 0 to v.size - 1) {
        r += v(a)
      }
      r
    }}}
    //measure method "RRBVector/relaxed" in { using(rrbvectorsF) config(exec.benchRuns -> runs) in { v => {
    //  var r = 0
    //  for (a <- 0 to v.size - 1) {
    //    r += v(a)
    //  }
    //  r
    //}}}
  }
  performance of "access/iter" in {
    measure method "Vector" in { using(vectors) config(exec.benchRuns -> runs) in { v => {
      var r = 0
      for (a <- v) {
        r += a
      }
      r
    }}}
    measure method "RRBVector" in { using(rrbvectors) config(exec.benchRuns -> runs) in { v => {
      var r = 0
      for (a <- v) {
        r += a
      }
      r
    }}}
    //measure method "RRBVector/relaxed" in { using(rrbvectorsF) config(exec.benchRuns -> runs) in { v => {
    //  var r = 0
    //  for (a <- v) {
    //    r += a
    //  }
    //  r
    //}}}
  }
}

class ConcatBenchmark extends BenchmarkBase {
  performance of "concat" in {
    measure method "RRBVector ++" in { using(rrbsteps) config(exec.benchRuns -> runs) in { v => {
      var r = RRBVector.empty[Int]
      for (_ <- 0 to step) {
        r = r ++ v
      }
      r
    }}}
  }
}
