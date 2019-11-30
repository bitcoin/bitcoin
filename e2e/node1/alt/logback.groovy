import ch.qos.logback.classic.encoder.PatternLayoutEncoder

import static ch.qos.logback.classic.Level.DEBUG

def logRootPath = System.getenv('ALT_INTEGRATION_LOG_PATH') ?: ''

appender("STDOUT", ConsoleAppender) {
    encoder(PatternLayoutEncoder) {
        pattern = "%date{YYYY-MM-dd HH:mm:ss.SSSXX} [%thread] %-5level %logger{36} - %msg%n"
    }
}
appender("FILE", RollingFileAppender) {
    file = "${logRootPath}alt-integration.log"
    rollingPolicy(SizeAndTimeBasedRollingPolicy) {
        fileNamePattern = "${logRootPath}alt-integration.%d{yyyy-MM-dd}.%i.log"
        maxHistory = 30
        maxFileSize = "10MB"
        totalSizeCap = "1GB"
    }
    encoder(PatternLayoutEncoder) {
        pattern = "%date{YYYY-MM-dd HH:mm:ss.SSSXX} %level [%thread] %logger{10} [%file:%line] %msg%n"
    }
}

logger("org.veriblock", DEBUG, ["STDOUT"])

root(DEBUG, ["FILE"])
