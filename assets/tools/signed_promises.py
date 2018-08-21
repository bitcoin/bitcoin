#!/usr/bin/env python3
# Script to find signed contract_urls
# Reads from a Ravencoin node - make sure its running
# Runs through the assets looking for ones with meta data
# Checks the meta data for contract_url
# Downloads the documents - (named by asset)
# Checks the signature.
# In order to use metadata, you must install be running IPFS
# Steps:
#   1. Get IPFS - https://ipfs.io/
#   2. Run the ipfs daemon
#   3. pip install ipfsapi
#
# If you're signing contract_url
# pip install python-bitcoinrpc


import random
import os
import subprocess
import csv
import json
import hashlib


cli = "raven-cli"
mode =  "-testnet"
rpc_user = 'rpcuser'
rpc_pass = 'rpcpass555'


def rpc_call(params):
    process = subprocess.Popen([cli, mode, params], stdout=subprocess.PIPE)
    out, err = process.communicate()
    return(out)

def get_rpc_connection():
    from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
    rpc_connection = AuthServiceProxy("http://%s:%s@127.0.0.1:18766"%(rpc_user, rpc_pass))
    return(rpc_connection)


def get_asset_list(filter):
    rpc_connection = get_rpc_connection()
    assets = rpc_connection.listassets("", True)
    return(assets)

def get_assets_with_ipfs_filter(assets):
    #Build a list of assets without ipfs_hash
    list_of_assets_without_ipfs = []
    for key,value in assets.iteritems():
        if (value['has_ipfs'] == 0):
            list_of_assets_without_ipfs.append(key)

    #Remove all the assets without ipfs_hash
    for name in list_of_assets_without_ipfs:       
        del assets[name]

    return(assets)

def get_ipfs_files(assets):
    list_of_bad_assets = []
    for key,value in assets.iteritems():
        if (len(value['ipfs_hash']) > 0):
            success = get_ipfs_file_wget(key+'.json', value['ipfs_hash'])
            if not success:
                list_of_bad_assets.append(key)
    #Remove all the assets without valid metadata files
    for name in list_of_bad_assets:       
        del assets[name]

    return(assets)


def get_ipfs_file(filename, hash):
    import ipfsapi
    api = ipfsapi.connect('127.0.0.1', 5001)

    print("Downloading: " + hash + " as " + filename)
    try:
        api.get(hash)
    except:
        print("ipfs.cat failed.")
    # print("Size:" + str(len(res)))
    # if (len(res) < 100):
    #     print(res)
    # return(res)

def get_signed_assets(assets):
    list_of_unsigned_assets = []
    print(assets)
    for key,value in assets.iteritems():
        if key == 'RAVEN_WITH_METADATA':
            print("Key " + key)
            #print("Value " + value)
            print("Reading metadata for " + str(key))
            metadata = read_metadata(str(key))
            print("Validating signed contract for " + key)
            #print("Contract A: " + value.get('contract_url', None))
            print("Contract B: " + metadata.get('contract_url', 'Not found'))
            success = validate_contract(metadata.get('contract_url', None), metadata.get('contract_hash', None), metadata.get('contract_address', None), metadata.get('contract_signature', None))
            if not success:
                list_of_unsigned_assets.append(key)
    #Remove all the assets without valid signatures
    for name in list_of_unsigned_assets:       
        del assets[name]

    return(assets)

def read_metadata(asset_name):
    with open(asset_name + '.json') as handle:
        metadata = json.loads(handle.read())
    return(metadata)    


#Takes a url, hash (SHA256 of url contents), address of signer, signature of signed hash (in text)
def validate_contract(url, hash256, address, signature):
    if url == None or hash256 == None or address == None or signature == None:
        print("Missing info for validating contract.")
        print("Requires url, hash256, address, signature")
        print("url: " + url)
        print("hash256: " + hash256)
        print("address: " + address)
        print("signature: " + signature)
        return 0

    print("Downloading: " + url + " and validating hash")
    try:
        filedata = urllib2.urlopen(url, timeout=30)  
        rawdata = filedata.read()

        hexdigest = hashlib.sha256(rawdata).hexdigest()
        if (hexdigest != hash):
            print("contract_url contents did not match contract_hash")
            return 0

        if not verify_message(address, signature, hash256):
            print("contract_hash did not match signature")
            return 0

    except urllib2.URLError as e:
        print type(e)
        return 0
    except:
        print("Uncaught error while downloading url")    #not catch
        return 0

    print("Contract Validated")
    return 1

def verify_message(address, signature, hash256):
    rpc_connection = get_rpc_connection()
    result = rpc_connection.verifymessage(address, signature, hash256)
    print("Verify result: " + result)
    return(result)    

def get_ipfs_file_wget(filename, hash):
    import urllib2

    print("Downloading: " + hash + " as " + filename)
    try:
        filedata = urllib2.urlopen('https://ipfs.io/ipfs/' + hash, timeout=20)  
        datatowrite = filedata.read()

        datatowrite.strip()
        if (datatowrite[0] != '{'):
            print("Not a valid metadata file")
            return


        with open(filename, 'wb') as f:  
            f.write(datatowrite)
        print("Saving metadata file")
    except urllib2.URLError as e:
        print type(e)
        return 0
    except:
        print("Uncaught error while downloading")    #not catch
        return 0

    return 1



def go(filter):
    assets = get_asset_list(filter)
    get_assets_with_ipfs_filter(assets)
    #get_ipfs_files(assets)
    get_signed_assets(assets)

    print(assets)


go("")
    


