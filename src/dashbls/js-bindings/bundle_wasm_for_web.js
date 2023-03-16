// The code manipulation from this file needs to be done for Chrome, as
// it requires wasm to be loaded asynchronously, and it doesn't
// work when bundling complex projects. With this solution, wasm is
// included right into the bundle itself in a form of base64 string
// and compiled asynchronously, just as Chrome requires

const fs = require('fs');

const outputPath = './blsjs.js';

const wasm = fs.readFileSync('./blsjstmp.wasm');
const wasmBase = wasm.toString('base64');

const codeToPrepend = `
if (typeof window === "object") {
    var buf = Buffer.from("${wasmBase}", "base64");
    var blob = new Blob([buf], { type: "application/wasm" });
    var wasmUrl = URL.createObjectURL(blob);
}
`;

const originalSourceCode = fs.readFileSync('./blsjstmp.js', 'utf-8');
const modifiedSourceCode = originalSourceCode
    .replace(/fetch\(.,/g, "fetch(wasmUrl,");

const modifiedSourceBuffer = Buffer.from(modifiedSourceCode, 'utf-8');

const bundleFileDescriptor = fs.openSync(outputPath, 'w+');

const bufferToPrepend = Buffer.from(codeToPrepend);

fs.writeSync(bundleFileDescriptor, bufferToPrepend, 0, bufferToPrepend.length, 0);
fs.writeSync(bundleFileDescriptor, modifiedSourceBuffer, 0, modifiedSourceBuffer.length, bufferToPrepend.length);

fs.close(bundleFileDescriptor, (err) => {
    if (err) throw err;
});
