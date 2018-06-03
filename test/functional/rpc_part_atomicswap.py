#!/usr/bin/env python3
# Copyright (c) 2018 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Test Decred atomic swap contracts

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose, getIndexAtProperty
from test_framework.util import *
from test_framework.script import *
from test_framework.messages import sha256
from test_framework.address import chars as __b58chars, script_to_p2sh
import binascii

from test_framework.test_particl import jsonDecimal


def script_to_p2sh_part(b):
    return script_to_p2sh(b, False, False)


def b58decode(v, length=None):
    long_value = 0
    for (i, c) in enumerate(v[::-1]):
        ofs = __b58chars.find(c)
        if ofs < 0:
            return None
        long_value += ofs * (58**i)
    result = bytes()
    while long_value >= 256:
        div, mod = divmod(long_value, 256)
        result = bytes((mod,)) + result
        long_value = div
    result = bytes((long_value,)) + result
    nPad = 0
    for c in v:
        if c == __b58chars[0]:
            nPad += 1
        else:
            break
    pad = bytes((0,)) * nPad
    result = pad + result
    if length is not None and len(result) != length:
        return None
    return result


def SerializeNum(n):
    rv = bytearray()

    if n == 0:
        return rv
    neg = n < 0
    absvalue = -n if neg else n
    while(absvalue):
        rv.append(absvalue & 0xff)
        absvalue >>= 8

    if rv[-1] & 0x80:
        rv.append(0x80 if neg else 0)
    elif neg:
        rv[-1] |= 0x80

    return rv


def CreateAtomicSwapScript(payTo, refundTo, lockTime, secretHash):
    script = CScript([
        OP_IF,
        # Normal redeem path
        OP_SIZE,
        bytes((32,)),
        OP_EQUALVERIFY,
        OP_SHA256,
        secretHash,
        OP_EQUALVERIFY,
        OP_DUP,
        OP_HASH160,
        payTo,

        OP_ELSE,
        # Refund path
        SerializeNum(lockTime),
        OP_CHECKLOCKTIMEVERIFY,
        OP_DROP,
        OP_DUP,
        OP_HASH160,
        refundTo,
        OP_ENDIF,

        OP_EQUALVERIFY,
        OP_CHECKSIG
        ])
    return script


def getOutputByAddr(tx, addr):
    for i, vout in enumerate(tx['vout']):
        try:
            if addr in vout['scriptPubKey']['addresses']:
                return i
        except:
            continue
    return -1


WITNESS_SCALE_FACTOR = 2
def getVirtualSizeInner(sizeNoWitness, sizeTotal):
    return ((sizeNoWitness*(WITNESS_SCALE_FACTOR-1) + sizeTotal) + WITNESS_SCALE_FACTOR - 1) // WITNESS_SCALE_FACTOR


# estimateRedeemSerializeSize returns a worst case serialize size estimates for
# a transaction that redeems an atomic swap P2SH output.
def estimateRedeemSerializeSize(contract, txhex):
    baseSize = len(txhex) // 2
    # varintsize(height of stack) + height of stack, redeemSig, redeemPubKey, secret, lenSecret, b1, contract
    witnessSize = 6 + 73 + 33 + 32 + 1 + 1 + len(contract)
    return getVirtualSizeInner(baseSize, baseSize+witnessSize)


def estimateRefundSerializeSize(contract, txhex):
    baseSize = len(txhex) // 2
    # varintsize(height of stack) + height of stack, refundSig, refundPubKey, b0, contract
    witnessSize = 5 + 73 + 33 + 1 + len(contract)
    return getVirtualSizeInner(baseSize, baseSize+witnessSize)


def createRefundTx(node, rawtx, script, lockTime, addrRefundFrom, addrRefundTo):
    p2sh = script_to_p2sh_part(script)
    ro = node.decoderawtransaction(rawtx)
    txnid = ro['txid']
    n = getOutputByAddr(ro, p2sh)

    scriptPubKey = ro['vout'][n]['scriptPubKey']['hex']

    amountIn = ro['vout'][n]['value']

    rawtxrefund = node.tx(['-create','in='+txnid+':'+str(n)+':1','outaddr='+str(amountIn)+':'+addrRefundTo,'locktime='+str(lockTime)])

    refundWeight = estimateRefundSerializeSize(script, rawtxrefund)

    ro = node.getnetworkinfo()
    feePerKB = ro['relayfee']
    fee = (feePerKB * refundWeight) / 1000

    amountOut = Decimal(amountIn) - Decimal(fee)

    rawtxrefund = node.tx([rawtxrefund,'delout=0','outaddr='+str(amountOut)+':'+addrRefundTo])


    scripthex = binascii.hexlify(script).decode("utf-8")
    prevtx = {
        'txid':txnid,
        'vout':n,
        'scriptPubKey':scriptPubKey,
        'redeemScript':scripthex,
        'amount':amountIn
    }
    refundSig = node.createsignaturewithwallet(rawtxrefund, prevtx, addrRefundFrom)

    addrRefundFromInfo = node.getaddressinfo(addrRefundFrom)

    witnessStack = [
        refundSig,
        addrRefundFromInfo['pubkey'],
        '00',
        scripthex
    ]

    rawtxrefund = node.tx([rawtxrefund,'witness=0:'+ ':'.join(witnessStack)])
    return rawtxrefund


def createClaimTx(node, rawtx, script, secret, addrClaimFrom, addrClaimTo):
    p2sh = script_to_p2sh_part(script)

    ro = node.decoderawtransaction(rawtx)
    txnid = ro['txid']
    n = getOutputByAddr(ro, p2sh)
    amountIn = ro['vout'][n]['value']
    scriptPubKey = ro['vout'][n]['scriptPubKey']['hex']
    inputs = [ {'txid': txnid, 'vout': n}, ]
    rawtxClaim = node.createrawtransaction(inputs, {addrClaimTo:amountIn})

    redeemWeight = estimateRedeemSerializeSize(script, rawtxClaim)

    ro = node.getnetworkinfo()
    feePerKB = ro['relayfee']
    fee = (feePerKB * redeemWeight) / 1000

    amountOut = Decimal(amountIn) - Decimal(fee)

    rawtxClaim = node.tx([rawtxClaim,'delout=0','outaddr='+str(amountOut)+':'+addrClaimTo])

    scripthex = binascii.hexlify(script).decode("utf-8")
    prevtx = {
        'txid':txnid,
        'vout':n,
        'scriptPubKey':scriptPubKey,
        'redeemScript':scripthex,
        'amount':amountIn
    }
    claimSig = node.createsignaturewithwallet(rawtxClaim, prevtx, addrClaimFrom)

    addrClaimFromInfo = node.getaddressinfo(addrClaimFrom)

    secrethex = binascii.hexlify(secret).decode("utf-8")
    witnessStack = [
        claimSig,
        addrClaimFromInfo['pubkey'],
        secrethex,
        '01',
        scripthex
    ]
    rawtxClaim = node.tx([rawtxClaim,'witness=0:'+ ':'.join(witnessStack)])

    assert(node.testmempoolaccept([rawtxClaim,])[0]['allowed'] == True)
    return rawtxClaim


def createRefundTxCT(node, rawtx, output_amounts, script, lockTime, privKeySign, pubKeySign, addrRefundTo):
    p2sh = script_to_p2sh_part(script)
    ro = node.decoderawtransaction(rawtx)
    txnid = ro['txid']
    n = getOutputByAddr(ro, p2sh)

    scriptPubKey = ro['vout'][n]['scriptPubKey']['hex']
    valueCommitment = ro['vout'][n]['valueCommitment']

    amountIn = output_amounts[str(n)]['value']
    blindIn = output_amounts[str(n)]['blind']

    inputs = [{'txid':txnid, 'vout':n}]

    outputs = [ {
        'address':addrRefundTo,
        'type':'blind',
        'amount':amountIn,
    }, ]
    ro = node.createrawparttransaction(inputs, outputs, lockTime)
    rawtxrefund = ro['hex']

    witnessSize = 5 + 73 + 33 + 1 + len(script)
    tempBytes = bytearray(witnessSize) # Needed for fee estimation

    # Set fee and commitment sum
    options = {'subtractFeeFromOutputs':[0,]}
    input_amounts = {'0': {
            'value':amountIn,
            'blind':blindIn,
            'witness':binascii.hexlify(tempBytes).decode("utf-8")
        }
    }
    ro = node.fundrawtransactionfrom('blind', ro['hex'], input_amounts, ro['amounts'], options)
    rawtxrefund = ro['hex']

    scripthex = binascii.hexlify(script).decode("utf-8")
    prevtx = {
        'txid':txnid,
        'vout':n,
        'scriptPubKey':scriptPubKey,
        'redeemScript':scripthex,
        'amount_commitment':valueCommitment
    }
    refundSig = node.createsignaturewithkey(rawtxrefund, prevtx, privKeySign)

    witnessStack = [
        refundSig,
        pubKeySign,
        '00',
        scripthex
    ]
    rawtxrefund = node.tx([rawtxrefund,'witness=0:'+ ':'.join(witnessStack)])

    ro = node.decoderawtransaction(rawtxrefund)
    assert(len(ro['vin'][0]['txinwitness']) == 4)

    return rawtxrefund


def createClaimTxCT(node, rawtx, output_amounts, script, secret, privKeySign, pubKeySign, addrClaimTo):
    p2sh = script_to_p2sh_part(script)

    ro = node.decoderawtransaction(rawtx)
    txnid = ro['txid']
    n = getOutputByAddr(ro, p2sh)

    scriptPubKey = ro['vout'][n]['scriptPubKey']['hex']
    valueCommitment = ro['vout'][n]['valueCommitment']

    amountIn = output_amounts[str(n)]['value']
    blindIn = output_amounts[str(n)]['blind']

    inputs = [ {'txid': txnid, 'vout': n}, ]
    outputs = [ {
        'address':addrClaimTo,
        'type':'blind',
        'amount':amountIn,
    }, ]
    ro = node.createrawparttransaction(inputs, outputs)
    rawtxClaim = ro['hex']

    # Need esitmated witnessSize for fee estimation
    witnessSize = 6 + 73 + 33 + 32 + 1 + 1 + len(script)
    tempBytes = bytearray(witnessSize)

    # Set fee and commitment sum
    options = {'subtractFeeFromOutputs':[0,]}
    input_amounts = {'0': {
            'value':amountIn,
            'blind':blindIn,
            'witness':binascii.hexlify(tempBytes).decode("utf-8")
        }
    }
    ro = node.fundrawtransactionfrom('blind', ro['hex'], input_amounts, ro['amounts'], options)
    rawtxClaim = ro['hex']

    scripthex = binascii.hexlify(script).decode("utf-8")
    prevtx = {
        'txid':txnid,
        'vout':n,
        'scriptPubKey':scriptPubKey,
        'redeemScript':scripthex,
        'amount_commitment':valueCommitment
    }
    claimSig = node.createsignaturewithkey(rawtxClaim, prevtx, privKeySign)

    secrethex = binascii.hexlify(secret).decode("utf-8")
    witnessStack = [
        claimSig,
        pubKeySign,
        secrethex,
        '01',
        scripthex
    ]
    rawtxClaim = node.tx([rawtxClaim,'witness=0:'+ ':'.join(witnessStack)])

    ro = node.decoderawtransaction(rawtxClaim)
    assert(len(ro['vin'][0]['txinwitness']) == 5)

    return rawtxClaim


class AtomicSwapTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [ ['-debug','-noacceptnonstdtxn'] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes_bi(self.nodes, 0, 1)
        self.is_network_split = False
        self.sync_all()


    def test_standardtx(self):
        nodes = self.nodes

        addrA_0 = nodes[0].getnewaddress() # party A
        addrB_0 = nodes[1].getnewaddress() # party B

        # Initiate A -> B
        # A has address (addrB_0) of B

        amountA = 5.0
        amountB = 5.0
        pkh0_0 = b58decode(addrA_0)[1:-4]
        pkh1_0 = b58decode(addrB_0)[1:-4]

        secretA = os.urandom(32)
        secretAHash = sha256(secretA)

        lockTime = int(time.time()) + 10000 # future locktime

        scriptInitiate = CreateAtomicSwapScript(payTo=pkh1_0, refundTo=pkh0_0, lockTime=lockTime, secretHash=secretAHash)
        p2sh_initiate = script_to_p2sh_part(scriptInitiate)
        rawtxInitiate = nodes[0].createrawtransaction([], {p2sh_initiate:amountA})
        rawtxInitiate = nodes[0].fundrawtransaction(rawtxInitiate)['hex']
        ro = nodes[0].signrawtransactionwithwallet(rawtxInitiate)
        assert(ro['complete'] == True)
        rawtxInitiate = ro['hex']

        rawtx1refund = createRefundTx(nodes[0], rawtxInitiate, scriptInitiate, lockTime, addrA_0, addrA_0)

        nodes[0].sendrawtransaction(rawtxInitiate)
        self.stakeBlocks(1)

        ro = nodes[0].getblockchaininfo()
        assert(ro['mediantime'] < lockTime)
        try:
            txnidrefund = nodes[0].sendrawtransaction(rawtx1refund)
            assert(False)
        except JSONRPCException as e:
            assert('non-final' in e.error['message'])

        # Party A sends B rawtxInitiate/txnid1 and script

        # auditcontract
        # Party B extracts the secrethash and verifies the txn:
        assert(len(scriptInitiate) == 97)
        extractedSecretAHash = scriptInitiate[7:7+32]
        assert(extractedSecretAHash == secretAHash)
        tx1 = nodes[1].decoderawtransaction(rawtxInitiate)
        self.log.info("Verify txn " + tx1['txid']) # TODO

        # Participate B -> A
        # needs address from A, amount and secretAHash

        lockTimeP = int(time.time()) + 10000 # future locktime
        scriptParticipate = CreateAtomicSwapScript(payTo=pkh0_0, refundTo=pkh1_0, lockTime=lockTimeP, secretHash=secretAHash)
        p2sh_participate = script_to_p2sh_part(scriptParticipate)

        rawtx_p = nodes[1].createrawtransaction([], {p2sh_participate:amountB})
        rawtx_p = nodes[1].fundrawtransaction(rawtx_p)['hex']

        ro = nodes[1].signrawtransactionwithwallet(rawtx_p)
        assert(ro['complete'] == True)
        rawtx_p = ro['hex']

        rawtxRefundP = createRefundTx(nodes[1], rawtx_p, scriptParticipate, lockTimeP, addrB_0, addrB_0)

        txnidParticipate = nodes[1].sendrawtransaction(rawtx_p)
        self.stakeBlocks(1)

        ro = nodes[0].getblockchaininfo()
        assert(ro['mediantime'] < lockTimeP)
        try:
            txnidrefund = nodes[1].sendrawtransaction(rawtxRefundP)
            assert(False)
        except JSONRPCException as e:
            assert('non-final' in e.error['message'])

        # auditcontract
        # Party A verifies the participate txn from B


        # Party A Redeem/Claim from participate txn
        # Party A spends the funds from the participate txn, and to do so must reveal secretA

        rawtxclaimA = createClaimTx(nodes[0], rawtx_p, scriptParticipate, secretA, addrA_0, addrA_0)
        txnidAClaim = nodes[0].sendrawtransaction(rawtxclaimA)


        # Party B Redeem/Claim from initiate txn
        # Get secret from txnidAClaim

        #ro = nodes[1].getrawtransaction(txnidAClaim, True)
        #print('ro', json.dumps(ro, indent=4, default=jsonDecimal))

        rawtxclaimB = createClaimTx(nodes[1], rawtxInitiate, scriptInitiate, secretA, addrB_0, addrB_0)
        txnidBClaim = nodes[0].sendrawtransaction(rawtxclaimB) # send from staking node to avoid syncing

        self.stakeBlocks(1)
        ftxB = nodes[1].filtertransactions()
        assert(ftxB[0]['confirmations'] == 1)
        assert(ftxB[0]['outputs'][0]['amount'] < 5.0 and ftxB[-1]['outputs'][0]['amount'] > 4.9)
        assert(isclose(ftxB[1]['outputs'][0]['amount'], -5.0))


        # Test Refund expired initiate tx
        lockTime = int(time.time()) - 100000 # past locktime

        scriptInitiate2 = CreateAtomicSwapScript(payTo=pkh1_0, refundTo=pkh0_0, lockTime=lockTime, secretHash=secretAHash)

        p2sh_initiate = script_to_p2sh_part(scriptInitiate2)
        rawtxInitiate = nodes[0].createrawtransaction([], {p2sh_initiate:6.0})
        rawtxInitiate = nodes[0].fundrawtransaction(rawtxInitiate)['hex']
        ro = nodes[0].signrawtransactionwithwallet(rawtxInitiate)
        assert(ro['complete'] == True)
        rawtxInitiate = ro['hex']


        rawtx2refund = createRefundTx(nodes[0], rawtxInitiate, scriptInitiate2, lockTime, addrA_0, addrA_0)
        txnid2 = nodes[0].sendrawtransaction(rawtxInitiate)

        self.stakeBlocks(1)

        ro = nodes[0].getblockchaininfo()
        assert(ro['mediantime'] > lockTime)

        txnidrefund = nodes[0].sendrawtransaction(rawtx2refund)

        ftxA = nodes[0].filtertransactions()
        n = getIndexAtProperty(ftxA, 'txid', txnidrefund)
        assert(n > -1)
        assert(ftxA[n]['outputs'][0]['amount'] > 5.9 and ftxA[n]['outputs'][0]['amount'] < 6.0)


    def test_cttx(self):
        nodes = self.nodes
        # Test confidential transactions
        addrA_sx = nodes[0].getnewstealthaddress() # party A
        addrB_sx = nodes[1].getnewstealthaddress() # party B

        outputs = [ {
            'address':addrA_sx,
            'type':'blind',
            'amount':100,
        }, {
            'address':addrB_sx,
            'type':'blind',
            'amount':100,
        }, ]
        ro = nodes[0].createrawparttransaction([], outputs)

        ro = nodes[0].fundrawtransactionfrom('standard', ro['hex'], {}, ro['amounts'])
        rawtx = ro['hex']

        ro = nodes[0].signrawtransactionwithwallet(rawtx)
        assert(ro['complete'] == True)

        ro = nodes[0].sendrawtransaction(ro['hex'])
        txnid = ro

        self.stakeBlocks(1)

        ro = nodes[0].getwalletinfo()
        assert(isclose(ro['blind_balance'], 100.0))

        ro = nodes[0].filtertransactions()
        n = getIndexAtProperty(ro, 'txid', txnid)
        assert(n > -1)
        assert(isclose(ro[n]['amount'], -100.00581000))

        ro = nodes[1].getwalletinfo()
        assert(isclose(ro['blind_balance'], 100.0))

        ro = nodes[1].filtertransactions()
        n = getIndexAtProperty(ro, 'txid', txnid)
        assert(n > -1)
        assert(isclose(ro[n]['amount'], 100.0))


        # Initiate A -> B
        # A has address (addrB_sx) of B

        amountA = 7.0
        amountB = 7.0

        secretA = os.urandom(32)
        secretAHash = sha256(secretA)
        lockTime = int(time.time()) + 10000 # future locktime


        destA = nodes[0].derivefromstealthaddress(addrA_sx)
        pkh0_0 = b58decode(destA['address'])[1:-4]

        ro = nodes[0].derivefromstealthaddress(addrA_sx, destA['ephemeral_pubkey'])
        privKeyA = ro['privatekey']
        pubKeyA = ro['pubkey']


        destB = nodes[0].derivefromstealthaddress(addrB_sx)
        pkh1_0 = b58decode(destB['address'])[1:-4]

        ro = nodes[1].derivefromstealthaddress(addrB_sx, destB['ephemeral_pubkey'])
        privKeyB = ro['privatekey']
        pubKeyB = ro['pubkey']


        scriptInitiate = CreateAtomicSwapScript(payTo=pkh1_0, refundTo=pkh0_0, lockTime=lockTime, secretHash=secretAHash)
        p2sh_initiate = script_to_p2sh_part(scriptInitiate)

        outputs = [ {
            'address':p2sh_initiate,
            'pubkey':destB['pubkey'],
            'type':'blind',
            'amount':amountA,
        }, ]
        ro = nodes[0].createrawparttransaction([], outputs)
        ro = nodes[0].fundrawtransactionfrom('blind', ro['hex'], {}, ro['amounts'])
        output_amounts_i = ro['output_amounts']
        ro = nodes[0].signrawtransactionwithwallet(ro['hex'])
        assert(ro['complete'] == True)
        rawtx_i = ro['hex']


        rawtx_i_refund = createRefundTxCT(nodes[0], rawtx_i, output_amounts_i, scriptInitiate, lockTime, privKeyA, pubKeyA, addrA_sx)
        ro = nodes[0].testmempoolaccept([rawtx_i_refund,])
        assert('64: non-final' in ro[0]['reject-reason'])

        txnid1 = nodes[0].sendrawtransaction(rawtx_i)
        self.stakeBlocks(1)


        # Party A sends B rawtx_i/txnid1, script and output_amounts_i


        # auditcontract
        # Party B extracts the secrethash and verifies the txn:

        ro = nodes[1].decoderawtransaction(rawtx_i)
        n = getOutputByAddr(ro, p2sh_initiate)
        valueCommitment = ro['vout'][n]['valueCommitment']

        amount = output_amounts_i[str(n)]['value']
        blind = output_amounts_i[str(n)]['blind']

        assert(nodes[1].verifycommitment(valueCommitment, blind, amount)['result'] == True)



        # Participate B -> A
        # needs address and publickey from A, amount and secretAHash

        lockTimeP = int(time.time()) + 10000 # future locktime
        scriptParticipate = CreateAtomicSwapScript(payTo=pkh0_0, refundTo=pkh1_0, lockTime=lockTimeP, secretHash=secretAHash)
        p2sh_participate = script_to_p2sh_part(scriptParticipate)

        outputs = [ {
            'address':p2sh_participate,
            'pubkey':destA['pubkey'],
            'type':'blind',
            'amount':amountB,
        }, ]

        ro = nodes[1].createrawparttransaction([], outputs)
        ro = nodes[1].fundrawtransactionfrom('blind', ro['hex'], {}, ro['amounts'])
        output_amounts_p = ro['output_amounts']
        ro = nodes[1].signrawtransactionwithwallet(ro['hex'])
        rawtx_p = ro['hex']

        rawtx_p_refund = createRefundTxCT(nodes[1], rawtx_p, output_amounts_p, scriptParticipate, lockTime, privKeyB, pubKeyB, addrB_sx)
        ro = nodes[1].testmempoolaccept([rawtx_p_refund,])
        assert('64: non-final' in ro[0]['reject-reason'])

        txnid_p = nodes[0].sendrawtransaction(rawtx_p)
        self.stakeBlocks(1)

        # B sends output_amounts to A

        # Party A Redeem/Claim from participate txn
        # Party A spends the funds from the participate txn, and to do so must reveal secretA

        rawtxclaimA = createClaimTxCT(nodes[0], rawtx_p, output_amounts_p, scriptParticipate, secretA, privKeyA, pubKeyA, addrA_sx)
        txnidAClaim = nodes[0].sendrawtransaction(rawtxclaimA)


        # Party B claims from initiate txn
        # Get secret from txnidAClaim

        #ro = nodes[1].getrawtransaction(txnidAClaim, True)
        #print('ro', json.dumps(ro, indent=4, default=jsonDecimal))

        rawtxclaimB = createClaimTxCT(nodes[1], rawtx_i, output_amounts_i, scriptInitiate, secretA, privKeyB, pubKeyB, addrB_sx)
        txnidBClaim = nodes[0].sendrawtransaction(rawtxclaimB)


        # Test Refund expired initiate tx
        secretA = os.urandom(32)
        secretAHash = sha256(secretA)
        lockTime = int(time.time()) - 100000 # past locktime

        amountA = 7.1
        scriptInitiate = CreateAtomicSwapScript(payTo=pkh1_0, refundTo=pkh0_0, lockTime=lockTime, secretHash=secretAHash)
        p2sh_initiate = script_to_p2sh_part(scriptInitiate)

        outputs = [ {
            'address':p2sh_initiate,
            'pubkey':destB['pubkey'],
            'type':'blind',
            'amount':amountA,
        }, ]
        ro = nodes[0].createrawparttransaction([], outputs)
        ro = nodes[0].fundrawtransactionfrom('blind', ro['hex'], {}, ro['amounts'])

        r2 = nodes[0].verifyrawtransaction(ro['hex'])
        assert(r2['complete'] == False)

        output_amounts_i = ro['output_amounts']
        ro = nodes[0].signrawtransactionwithwallet(ro['hex'])
        assert(ro['complete'] == True)
        rawtx_i = ro['hex']

        r2 = nodes[0].verifyrawtransaction(rawtx_i)
        assert(r2['complete'] == True)


        rawtx_i_refund = createRefundTxCT(nodes[0], rawtx_i, output_amounts_i, scriptInitiate, lockTime, privKeyA, pubKeyA, addrA_sx)
        ro = nodes[0].testmempoolaccept([rawtx_i_refund,])
        assert('missing-inputs' in ro[0]['reject-reason'])


        txnid1 = nodes[0].sendrawtransaction(rawtx_i)
        ro = nodes[0].getwalletinfo()
        assert(ro['unconfirmed_blind'] > 6.0 and ro['unconfirmed_blind'] < 7.0)

        txnidRefund = nodes[0].sendrawtransaction(rawtx_i_refund)

        ro = nodes[0].getwalletinfo()
        assert(ro['unconfirmed_blind'] > 14.0 and ro['unconfirmed_blind'] < 14.1)


    def run_test(self):
        nodes = self.nodes

        # Stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        nodes[1].extkeyimportmaster('pact mammal barrel matrix local final lecture chunk wasp survey bid various book strong spread fall ozone daring like topple door fatigue limb olympic', '', 'true')
        nodes[1].getnewextaddress('lblExtTest')
        nodes[1].rescanblockchain()
        assert(nodes[1].getwalletinfo()['total_balance'] == 25000)

        self.test_standardtx()

        self.test_cttx()


if __name__ == '__main__':
    AtomicSwapTest().main()
