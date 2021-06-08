const testSource = './karma.test.js';

const karmaConfig = {
    frameworks: ['mocha'],
    files: [
        testSource
    ],
    preprocessors: {
        [testSource]: ['webpack']
    },
    webpack: {
        mode: 'development',
        node: {
            fs: 'empty',
        },
    },
    reporters: ['mocha'],
    port: 9876,
    colors: true,
    autoWatch: false,
    browsers: ['FirefoxHeadless'],
    singleRun: false,
    concurrency: Infinity,
    plugins: [
        'karma-mocha',
        'karma-mocha-reporter',
        'karma-firefox-launcher',
        'karma-webpack'
    ],
    customLaunchers: {
        FirefoxHeadless: {
            base: 'Firefox',
            flags: ['-headless'],
        },
    },
};

module.exports = function (config) {
    config.set(karmaConfig);
};
