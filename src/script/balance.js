import resource from 'resource-router-middleware';
import web3 from './utils/web3';

export default ({ config }) => resource({

  id: 'address',

  load(req, id, callback) {
    let address = id;
    let err = web3.isAddress(address) ? null : 'Address is invalid'
    callback(err, address);
  },

  index({ params }, res) {
    res.json({message: 'Ethereum Classic API - Get the balance for a given address'  })
  },
  
  read({ address }, res) {
    const balanceWei = web3.eth.getBalance(address, 'pending');
    const balanceEther = web3.fromWei(balanceWei, 'ether') || 0;
    res.json({ 
      address: address,
    	balance: { 
        wei: balanceWei, 
        ether: balanceEther  
      }
    });
  }
})
