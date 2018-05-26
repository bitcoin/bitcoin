#!/bin/sh

FILE_NAME=$1
PRIVATE_KEY=${FILE_NAME}_private.pem
PUBLIC_KEY=${FILE_NAME}_public.pem
BITCOIN_PRIVATE_KEY=bitcoin_${FILE_NAME}_private.key
BITCOIN_PUBLIC_KEY=bitcoin_${FILE_NAME}_public.key

echo "Generating private key"
openssl ecparam -genkey -name secp256k1 -rand /dev/urandom -out $PRIVATE_KEY

echo "Generating public key"
openssl ec -in $PRIVATE_KEY -pubout -out $PUBLIC_KEY

echo "Generating BitCoin private key"
openssl ec -in $PRIVATE_KEY -outform DER|tail -c +8|head -c 32|xxd -p -c 32 > $BITCOIN_PRIVATE_KEY

echo "Generating BitCoin public key"
openssl ec -in $PRIVATE_KEY -pubout -outform DER|tail -c 65|xxd -p -c 65 > $BITCOIN_PUBLIC_KEY

echo "Files created!"