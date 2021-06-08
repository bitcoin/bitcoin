// This file is needed to compile tests with webpack into one file for the browser tests

const testsContext = require.context('./', true, /spec\.js$/);

testsContext.keys().forEach(testsContext);