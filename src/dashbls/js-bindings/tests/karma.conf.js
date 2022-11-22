const testSource = './karma.test.js';
const mime = require('mime');
const fs = require('fs');

function webpackConfig() {
    const config = require('./webpack.config.js');
    delete config.context;
    delete config.entry;
    delete config.output;
    // delete config.devServer;

    return config;
}

function MimeTypeMiddleware(config) {
    return function (request, response, next) {
        if (request.url.endsWith('.wasm')) {
            const content = fs.readFileSync('blsjs.wasm');
            response.setHeader('Content-Type', 'application/wasm');
            response.writeHead(200);
            response.write(content);
            response.end();
            return next('route');
        } else {
            return next(request,response);
        }
    };
}

const karmaConfig = {
    frameworks: ['mocha', 'webpack'],
    files: [
        'node_modules/babel-polyfill/dist/polyfill.js',
        testSource
    ],
    preprocessors: {
        [testSource]: ['webpack']
    },
    webpack: webpackConfig(),
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
        'karma-webpack',
        {'middleware:mimetyper': ['factory', MimeTypeMiddleware]}
    ],
    middleware: [ 'mimetyper' ],
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
