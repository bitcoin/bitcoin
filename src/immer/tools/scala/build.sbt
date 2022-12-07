/**  This is the simplest possible use of ScalaMeter.
  *  It allows running ScalaMeter benchmarks as part of the test suite.
  *  It means, that when the test command is run, ScalaMeter benchmarks are run along
  *  the tests from other test frameworks, such as ScalaTest or ScalaCheck.
  */
lazy val basic = Project(
  "basic",
  file("."),
  settings = Defaults.coreDefaultSettings ++ Seq(
    name := "immer-bechmarks",
    organization := "org.immer",
    scalaVersion := "2.11.1",
    scalacOptions ++= Seq("-deprecation", "-unchecked", "-feature", "-Xlint"),
    publishArtifact := false,
    libraryDependencies ++= Seq(
      "com.storm-enroute" %% "scalameter" % "0.8.2" % "test",
      "io.github.nicolasstucki" %% "scala-rrb-vector" % "0.1.1"
    ),
    resolvers ++= Seq(
      "Sonatype OSS Snapshots" at "https://oss.sonatype.org/content/repositories/snapshots",
      "Sonatype OSS Releases" at "https://oss.sonatype.org/content/repositories/releases"
    ),
    testFrameworks += new TestFramework("org.scalameter.ScalaMeterFramework"),
    parallelExecution in Test := false,
    logBuffered := false
  )
)
