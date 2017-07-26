import resource from 'resource-router-middleware';

import web3 from './utils/web3';
import models from './utils/models';

export default ({ config }) => resource({

  mergeParams: true,

  id: 'address',

  load(req, id, callback) {
    let address = id;
    let err = web3.isAddress(address) ? null : 'Address is invalid'
    callback(err, address);
  },

  create({ body }, res) {
    let data = new models.data();
    try {
      data.data = web3.eth.sendRawTransaction(body.rawtx.toString())
    } catch(error) {
      if (!data.data) {
        data = {
          error: !!error,
          message: error.toString().replace('Error: ', '')
        } 
      } else {
        // Catch faulty errors thrown by geth 
        data.data = web3.eth.getTransaction(data.data)
      }
    }
    res.json(data);
  },

  read({ address }, res) {
    let data = new models.data();
    try {
      const balanceWei = web3.eth.getBalance(address, 'pending');
      const balanceEther = web3.fromWei(balanceWei, 'ether');
      const nonce = web3.eth.getTransactionCount(address, 'pending');
      const gasPrice = web3.eth.gasPrice;
      
      data = {
        address: address,
        balance: {
          wei: balanceWei,
          ether: balanceEther
        },
        nonce: nonce,
        gasPrice: gasPrice,
      }
    } catch (error) {
      data = {
        error: !!error,
        message: error.toString()
      }    
    }
    res.json(data);
  }
})
