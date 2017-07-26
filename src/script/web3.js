import Web3 from 'web3';
import Cache from 'node-cache' ;
import Promise from 'bluebird';
import config from '../../config.json';

const web3 = new Web3();
const co = Promise.coroutine;

let cache = new Cache();
cache = Promise.promisifyAll(cache);

const buildUri = (rpcport) => {
  return 'http://localhost:' + rpcport;
}

const setWeb3 = co(function* () {

  let ports = yield cache.getAsync('rpcports');

  if (!ports) {
    yield cache.setAsync('rpcports', config.nodes);
  }

  let rpcports = yield cache.getAsync('rpcports');
  let rpcport = rpcports.shift();
  rpcports.push(rpcport);

  yield cache.setAsync('rpcports', rpcports);

  let gethUri = buildUri(rpcport);
  const provider = new web3.providers.HttpProvider(gethUri);
  web3.setProvider(provider);
})

setWeb3();

export default web3
