echo "Updating Tron..."
# install requirement 'jq'
sudo apt-get install jq
crown-cli stop
RELEASE=$(curl -s 'https://api.github.com/repos/Crowndev/crowncoin/releases/latest' | jq -r '.assets | .[] | select(.name=="crown-x86_64-unknown-linux-gnu.tar.gz") | .browser_download_url')
curl -sSL $RELEASE -o crown.tgz
tar xzf crown.tgz --no-anchored crownd crown-cli --transform='s/.*\///'
# use strip if installed
if hash strip 2>/dev/null; then
    strip crownd crown-cli
fi
sudo mv crownd crown-cli /usr/local/bin/
rm -rf crown*
crownd
echo "Update finished"
echo "Waiting for crownd to start up..."
sleep 15
VERSION=$(crown-cli getinfo | jq '.version')
PROTOCOL_VERSION=$(crown-cli getinfo | jq '.protocolversion')
WALLET_VERSION=$(crown-cli getinfo | jq '.walletversion')
ERRORS=$(crown-cli getinfo | jq '.errors')
if [ "$VERSION" = '""' ] || [ "$VERSION" = "null" ] || [ "$PROTOCOL_VERSION" = '""' ] || [ "$PROTOCOL_VERSION" = "null" ] || [ "$WALLET_VERSION" = '""' ] || [ "$WALLET_VERSION" = "null" ] || [ "$ERRORS" != '""' ]; then
    echo "An error has occured. Please run 'crown-cli getinfo' and check if the service is running."
    echo "If you encounter any problems, join us at https://mm.crownlab.eu/crown/channels/tron for support"
else
    echo "crownd is up and running!"
    echo "version: $VERSION"
    echo "protocolversion: $PROTOCOL_VERSION"
    echo "walletversion: $WALLET_VERSION"

fi