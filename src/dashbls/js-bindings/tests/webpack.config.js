const webpack = require('webpack');

module.exports = {
    mode: 'development',
    plugins: [
        new webpack.ProvidePlugin({
            process: 'process',

        })
    ],
    resolve: {
        fallback: {
            "path": require.resolve("path-browserify"),
            "crypto": require.resolve("crypto-browserify"),
            "stream": require.resolve("stream-browserify"),
            "fs": false,
        }
    }
};
