## Ravencoin Metadata Specification

Additional fields may be added, but will be ignored by Ravencoin.

```
{

   "contract_url": "[https://yoursite.com/more-info-about-the-coin.pdf](https://yoursite.com/more-info-about-the-coin.pdf)",

   "contract_hash": "<SHA256 hash in hex of contract_url contents>",

   "contract_signature": "<Ravencoin signed contract_hash>",

   "contract_address": "R9x4u22ru3zm5v8suWiXNji4ENWSG7eYkx",

   "symbol": "LEMONADE",

   "name": "Lemonade Gift",

   "issuer": "Lemonade Stands, Inc.",

   "description": "This coin is worth one lemonade.",

   "description_mime": "text/x-markdown; charset=UTF-8",

   "type": "Points",

   "website_url": "https://lemonadestands.com/redemption_instructions",

   "icon": "<base64 encoded png image at 32x32>",

   "image_url": "[https://yoursite.com/coin-image.jpg](https://yoursite.com/coin-image.jpg)",

   "contact_name": "Joe Schmoe",

   "contact_email": "[joe_schmoe@gmail.com](mailto:joe_schmoe@gmail.com)",

   "contact_address": "Lemonade HQ|1234 Nowhere Street|Billings, MT  83982",

   "contact_phone": "207-388-3838",

   "forsale": true,

   "forsale_price": "5000 RVN"

}
```

All fields are optional. Clients, explorers, and wallets are not obligated to display or use any metadata.

### Supported Attributes

**contract_url** - Specifies the url of a document.  Might be the agreement related to the coin's purpose.

**contract_hash** - A SHA256 hash in ascii hex of the contract_url document (pdf, txt, saved html, etc).   This is so the contract, which is a link, cannot be changed without evidence of the change.  This acts as a message digest for signing.

**contract_signature** - Signed contract_hash (message digest).   Sign the contract_hash with the private key of the address that issued the asset.

**contract_address** - Address that signed contract.  Used in conjunction with the contract_signature to prove that a specific address signed the contract.  For better security, this should be the address to which the original tokens were issued.

**symbol** - The symbol.  If included, it should match the issued symbol.

**name** - The name given to the symbol.  Example: name: "Bitcoin Cash"  Symbol: "BCH"

**description** - The description of what the symbol represents.

**description_mime** - The mime type of the description.  This may or may not be honored, depending on the client, explorer, etc.

**type** - The type that the qty of the token represents.  Examples: (Tokens, Points, Shares, Tickets).  This may or may not be displayed by the client.

**website_url** - The website for this token.  The client or software may or may not display this.

**icon** - Base64 encoded png at 32x32

**image_url** - Link to URL image for the coin.  Explorers or clients may not wish to display these images automatically.

**contact_name** - Name of the person or organization that owns or issued the token.

**contact_email** - The e-mail of the person or organization that owns or issued the token.

**contact_address** - The mailing address of the person or organization that owns or issued the token.  Lines should be separated by the pipe ("|") character.

**contact_phone** - The phone number of the person or organization that owns or issued the token.

**forsale** - Should be true or false.  Used by desirable token names that have been left as reissuable.  This is not for the cost of buying one token, but rather for buying the rights to own, control, and reissue the entire asset token.  This might be parsed by token broker websites.

**forsale_price** - To give buyers an idea of the cost to own and admin the asset token.   Price followed by a space, followed by the currency.  Examples: "10000 RVN" or "0.3 BTC" or "50000 USD"  This might be parsed by token broker websites.

