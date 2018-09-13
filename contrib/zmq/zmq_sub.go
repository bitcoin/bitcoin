// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Example client code making use of Bitcoin's ZMQ pubsub functionality. Only specialized handling
// for blockhash is implemented, which retrieves the raw block over RPC and then prints it.
//
// You can test this by starting bitcoind on regtest
//
//   ./src/bitcoind -zmqpubhashblock=tcp://127.0.0.1:38332 -rpcuser=foo -rpcpassword=bar -regtest -debug=1
//
// and running this script with
//
//   go get github.com/btcsuite/btcd
//   go get github.com/pebbe/zmq4
//   BTC_USER=foo BTC_PASS=bar BTC_RPCPORT=18443 ZMQ_ENDPOINT=tcp://127.0.0.1:38332 go run zmq_sub.go
//
// then generating some blocks with
//
//   ./src/bitcoin-cli -regtest -rpcuser=foo -rpcpassword=bar generate 1
//

package main

import (
	"encoding/hex"
	"fmt"
	"log"
	"os"

	"github.com/btcsuite/btcd/chaincfg/chainhash"
	"github.com/btcsuite/btcd/rpcclient"
	"github.com/btcsuite/btcd/wire"
	zmq "github.com/pebbe/zmq4"
)

var rpc *rpcclient.Client

// The ZMQ address to subscribe to.
var zmqEndpointStr = getEnv("ZMQ_ENDPOINT", "tcp://127.0.0.1:28332")

// The Bitcoin host address (for use by RPC)
var btcHost = getEnv("BTC_HOST", "127.0.0.1")

// The Bitcoin RPC port
var btcRPCPort = getEnv("BTC_RPCPORT", "8332")

// The Bitcoin RPC username
var btcRPCUser = os.Getenv("BTC_USER")

// The Bitcoin RPC password
var btcRPCPass = os.Getenv("BTC_PASS")

// Return the value of an environment variable if it's defined, otherwise return fallback.
func getEnv(key, fallback string) string {
	if value, found := os.LookupEnv(key); found {
		return value
	}
	return fallback
}

// Listen for Bitcoin notifications over ZMQ. Push matching messages into the provided channels
// upon receipt.
func listenForNotification(
	notifyBlockHash chan<- string,
	notifyBlockRaw chan<- string,
	notifyTxHash chan<- string,
	notifyTxRaw chan<- string,
	quit <-chan bool,
) {
	subscriber, _ := zmq.NewSocket(zmq.SUB)
	defer subscriber.Close()
	subscriber.Connect(zmqEndpointStr)

	log.Printf("Listening for zmq notifications on %s\n", zmqEndpointStr)

	// Optionally filter by message type
	//
	// subscriber.SetSubscribe("hashblock")
	//
	subscriber.SetSubscribe("")

	newmsg := make(chan [][]byte)

	go func() {
		for {
			msg, err := subscriber.RecvMessageBytes(0)
			if err != nil {
				log.Fatal(err)
				break
			}
			newmsg <- msg
		}
	}()

	for {
		select {
		case msg := <-newmsg:
			msgType := string(msg[0])
			encoded := hex.EncodeToString(msg[1])

			switch msgType {
			case "hashblock":
				notifyBlockHash <- encoded
			case "hashtx":
				notifyTxHash <- encoded
			case "rawblock":
				notifyBlockRaw <- encoded
			case "rawtx":
				notifyTxRaw <- encoded
			}
		case <-quit:
			log.Println("stopping listenForNotification")
			return
		}
	}
}

// Listen for blockhashes and when we get one, retrieve its raw format from RPC.
func handleBlockHash(recv <-chan string) {
	for {
		blockhash, more := <-recv

		if more {
			log.Printf("retrieving block from rpc: %s\n", blockhash)
			block, err := getBlockFromRPC(blockhash)
			if err != nil {
				log.Print(err)
				return
			}
			// Likely you'd do something more interesting here.
			log.Print(block)
		} else {
			log.Println("stopping handleBlockHash")
			return
		}
	}
}

// Listen for an event and print its payload.
func handleGeneric(recv <-chan string, name string) {
	for {
		payload, more := <-recv

		if more {
			log.Printf("got %s: %s\n", name, payload)
		} else {
			log.Printf("stopping handleGeneric(%s)\n", name)
			return
		}
	}
}

// Given a blockhash string, retrieve its raw block over RPC.
func getBlockFromRPC(blockhash string) (*wire.MsgBlock, error) {
	hash, err := chainhash.NewHashFromStr(blockhash)
	if err != nil {
		return nil, err
	}
	blockMsg, err := rpc.GetBlock(hash)
	if err != nil {
		return nil, err
	}
	return blockMsg, nil
}

func main() {
	connCfg := &rpcclient.ConnConfig{
		Host:         fmt.Sprintf("%s:%s", btcHost, btcRPCPort),
		User:         btcRPCUser,
		Pass:         btcRPCPass,
		HTTPPostMode: true,
		DisableTLS:   true,
	}

	var err error
	rpc, err = rpcclient.New(connCfg, nil)
	if err != nil {
		log.Fatal(err)
	}
	defer rpc.Shutdown()

	notifyBlockHash := make(chan string)
	notifyTxHash := make(chan string)
	notifyBlockRaw := make(chan string)
	notifyTxRaw := make(chan string)
	quit := make(chan bool)

	go handleBlockHash(notifyBlockHash)
	go handleGeneric(notifyBlockRaw, "rawblock")
	go handleGeneric(notifyTxHash, "hashtx")
	go handleGeneric(notifyTxRaw, "rawtx")

	listenForNotification(
		notifyBlockHash,
		notifyBlockRaw,
		notifyTxHash,
		notifyTxRaw,
		quit,
	)
}
