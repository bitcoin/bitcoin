# BlockStore: a helper class that keeps a map of blocks and implements
#             helper functions for responding to getheaders and getdata,
#             and for constructing a getheaders message
#

from mininode import *
import dbm

class BlockStore(object):
    def __init__(self, datadir):
        self.blockDB = dbm.open(datadir + "/blocks", 'c')
        self.currentBlock = 0L
    
    def close(self):
        self.blockDB.close()

    def get(self, blockhash):
        serialized_block = None
        try:
            serialized_block = self.blockDB[repr(blockhash)]
        except KeyError:
            return None
        f = cStringIO.StringIO(serialized_block)
        ret = CBlock()
        ret.deserialize(f)
        ret.calc_sha256()
        return ret

    # Note: this pulls full blocks out of the database just to retrieve
    # the headers -- perhaps we could keep a separate data structure
    # to avoid this overhead.
    def headers_for(self, locator, hash_stop, current_tip=None):
        if current_tip is None:
            current_tip = self.currentBlock
        current_block = self.get(current_tip)
        if current_block is None:
            return None

        response = msg_headers()
        headersList = [ CBlockHeader(current_block) ]
        maxheaders = 2000
        while (headersList[0].sha256 not in locator.vHave):
            prevBlockHash = headersList[0].hashPrevBlock
            prevBlock = self.get(prevBlockHash)
            if prevBlock is not None:
                headersList.insert(0, CBlockHeader(prevBlock))
            else:
                break
        headersList = headersList[:maxheaders] # truncate if we have too many
        hashList = [x.sha256 for x in headersList]
        index = len(headersList)
        if (hash_stop in hashList):
            index = hashList.index(hash_stop)+1
        response.headers = headersList[:index]
        return response

    def add_block(self, block):
        block.calc_sha256()
        try:
            self.blockDB[repr(block.sha256)] = bytes(block.serialize())
        except TypeError as e:
            print "Unexpected error: ", sys.exc_info()[0], e.args
        self.currentBlock = block.sha256

    def get_blocks(self, inv):
        responses = []
        for i in inv:
            if (i.type == 2): # MSG_BLOCK
                block = self.get(i.hash)
                if block is not None:
                    responses.append(msg_block(block))
        return responses

    def get_locator(self, current_tip=None):
        if current_tip is None:
            current_tip = self.currentBlock
        r = []
        counter = 0
        step = 1
        lastBlock = self.get(current_tip)
        while lastBlock is not None:
            r.append(lastBlock.hashPrevBlock)
            for i in range(step):
                lastBlock = self.get(lastBlock.hashPrevBlock)
                if lastBlock is None:
                    break
            counter += 1
            if counter > 10:
                step *= 2
        locator = CBlockLocator()
        locator.vHave = r
        return locator

class TxStore(object):
    def __init__(self, datadir):
        self.txDB = dbm.open(datadir + "/transactions", 'c')

    def close(self):
        self.txDB.close()

    def get(self, txhash):
        serialized_tx = None
        try:
            serialized_tx = self.txDB[repr(txhash)]
        except KeyError:
            return None
        f = cStringIO.StringIO(serialized_tx)
        ret = CTransaction()
        ret.deserialize(f)
        ret.calc_sha256()
        return ret

    def add_transaction(self, tx):
        tx.calc_sha256()
        try:
            self.txDB[repr(tx.sha256)] = bytes(tx.serialize())
        except TypeError as e:
            print "Unexpected error: ", sys.exc_info()[0], e.args

    def get_transactions(self, inv):
        responses = []
        for i in inv:
            if (i.type == 1): # MSG_TX
                tx = self.get(i.hash)
                if tx is not None:
                    responses.append(msg_tx(tx))
        return responses
