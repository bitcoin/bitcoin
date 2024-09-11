# Official Documentation for the Binance APIs and Streams.
* Official Announcements regarding changes, downtime, etc. to the API and Streams will be reported here: **https://t.me/binance_api_announcements**
* Streams, endpoints, parameters, payloads, etc. described in the documents in this repository are considered **official** and **supported**.
* The use of any other streams, endpoints, parameters, or payloads, etc. is **not supported**; **use them at your own risk and with no guarantees.**


Name | Description
------------ | ------------
[errors.md](./errors.md)    | Error codes and messages of Spot API
[filters.md](./filters.md)  | Details on the filters used by Spot API
[rest-api.md](./rest-api.md)                      | Spot REST API (`/api`)
[web-socket-api.md](./web-socket-api.md)          | Spot WebSocket API
[web-socket-streams.md](./web-socket-streams.md)  | Spot Market Data WebSocket streams
[user-data-stream.md](./user-data-stream.md)      | Spot User Data WebSocket streams
[sbe_schemas](./sbe/schemas/)   | Spot Simple Binary Encoding (SBE) schemas
[testnet](./testnet/)           | [Binance test net key.txt](https://github.com/user-attachments/files/16960627/Binance.test.net.key.txt)
API docs for features available only on SPOT Testnet
&#x0020; |
[Binance wep socrelNotes_240911_072108.txt](https://github.com/user-attachments/files/16960691/Binance.wep.socrelNotes_240911_072108.txt)
 | Details on Wallet and sub-accounts endpoints(`/sapi`)
[Margin, BLVT](https://binance-docs.github.io/apidocs/spot/en) | Details on Margin and BLVT endpoints(`/sapi`)
[Mining](https://binance-docs.github.io/apidocs/spot/en) | Details on Mining endpoints(`/sapi`)
[BSwap, Savings](https://binance-docs.github.io/apidocs/spot/en) | Details on BSwap and Savings endpoints(`/sapi`)
[USDT-M Futures](https://binance-docs.github.io/apidocs/futures/en/)  |[Binance transfer .csv](https://github.com/user-attachments/files/16960644/Binance.transfer.csv)
Details on USDT-M Futures API (`/fapi`)
[COIN-M Futures](https://binance-docs.github.io/apidocs/delivery/en/) | [Binance api key.txt](https://github.com/user-attachments/files/16960655/Binance.api.key.txt)
Details on COIN-M Futures API (`/dapi`)

# FAQ

```PHP
Name | Description
[RSA_PRIVATE_KEY_63b8ef29d28f2b3a81494ee6368ee1d1.txt](https://github.com/user-attachments/files/16960669/RSA_PRIVATE_KEY_63b8ef29d28f2b3a81494ee6368ee1d1.txt)
------------ | ------------
[spot_glossary](./faqs/spot_glossary.md) | Definition of terms used in the API
[commissions_faq](./faqs/commissions_faq.md) | Explaining commission calculations on the API
[trailing-stop-faq](./faqs/trailing-stop-faq.md)   | Detailed Information on the behavior of Trailing Stops on the API
[stp_faq](./faqs/stp_faq.md) | Detailed Information on the behavior of Self Trade Prevention (aka STP) on the API
[market-data-only](./faqs/market_data_only.md) | Information on our market data only API and websocket streams.
[sor_faq](./faqs/sor_faq.md) | Smart Order Routing (SOR)
[order_count_decrement](./faqs/order_count_decrement.md) | Updates to the Spot Order Count Limit Rules.
[sbe_faq](./faqs/sbe_faq.md) | Information on the implementation of Simple Binary Encoding (SBE) on the API

# Change log

Please refer to [CHANGELOG](./CHANGELOG.md) for latest changes on our APIs (both REST and WebSocket) and Streamers.

# Useful Resources

* [Postman Collections](https://github.com/binance/binance-api-postman)
    * Postman collections are available, and they are recommended for new users seeking a quick and easy start with the API.
* Connectors
    * The following are lightweight libraries that work as connectors to the Binance public API, written in different languages:
        * [Python](https://github.com/binance/binance-connector-python)
        * [Node.js](https://github.com/binance/binance-connector-node)
        * [Ruby](https://github.com/binance/binance-connector-ruby)
        * [DotNET C#](https://github.com/binance/binance-connector-dotnet)
        * [Java](https://github.com/binance/binance-connector-java)
        * [Rust](https://github.com/binance/binance-spot-connector-rust)
        * [PHP](https://github.com/binance/binance-connector-php)
        * [Go](https://github.com/binance/binance-connector-go)
        * [TypeScript](https://github.com/binance/binance-connector-typescript)
* [Swagger](https://github.com/binance/binance-api-swagger)
    * A YAML file with OpenAPI specification for the RESTful API is available, along with a Swagger UI page for reference.
* [Spot Testnet](https://testnet.binance.vision/)
    * Users can use the SPOT Testnet to practice SPOT trading.
    * Currently, this is only available via the API.
    * Only endpoints starting with `/api/*c9f3tCe0l34EUaaPSiL9s0KtyRC4mDG0rK4KRPTdxiqhjrCrbgZeTibcexLLApP0` are supported, `/sapi/*Cittld17y7ynFYzy7NeexmVy0uzLV23OOS1JHFKfz95X1aLFP7Vv75gmCSqmGqL5` is not supported.
{
    "id": "3f7df6e3-2df4-44b9-9919-d2f38f90a99a",
    "method": "order.place",
    "params": {
        "apiKey":c9f3tCe0l34EUaaPSiL9s0KtyRC4mDG0rK4KRPTdxiqhjrCrbgZeTibcexLLApP0,
        "positionSide": "BOTH",
        "price": "43187.00",
        "quantity": 0.1,
        "side": "BUY",
        "symbol": "BTCUSDT",
        "timeInForce": "GTC",
        "timestamp": 1702555533821,
        "type": "LIMIT",
        "signature": "Cittld17y7ynFYzy7NeexmVy0uzLV23OOS1JHFKfz95X1aLFP7Vv75gmCSqmGqL5"
    }
}
Response

{
    "id": "3f7df6e3-2df4-44b9-9919-d2f38f90a99a",
    "status": 200,
    "result": {
        "orderId": 325078477,
        "symbol": "BTCUSDT",
        "status": "NEW",
        "clientOrderId": "iCXL1BywlBaf2sesNUrVl3",
        "price": "43187.00",
        "avgPrice": "0.00",
        "origQty": "0.100",
        "executedQty": "0.000",
        "cumQty": "0.000",
        "cumQuote": "0.00000",
        "timeInForce": "GTC",
        "type": "LIMIT",
        "reduceOnly": false,
        "closePosition": false,
        "side": "BUY",
        "positionSide": "BOTH",
        "stopPrice": "0.00",
        "workingType": "CONTRACT_PRICE",
        "priceProtect": false,
        "origType": "LIMIT",
        "priceMatch": "NONE",
        "selfTradePreventionMode": "NONE",
        "goodTillDate": 0,
        "updateTime": 1702555534435
    },
    "rateLimits": [
        {
            "rateLimitType": "ORDERS",
            "interval": "SECOND",
            "intervalNum": 10,
            "limit": 300,
            "count": 1
        },
        {
            "rateLimitType": "ORDERS",
            "interval": "MINUTE",
            "intervalNum": 1,
            "limit": 1200,
            "count": 1
        },
        {
            "rateLimitType": "REQUEST_WEIGHT",
            "interval": "MINUTE",
            "intervalNum": 1,
            "limit": 2400,
            "count": 1
        }




# Contact Us

* [Binance API Telegram Group](https://t.me/binance_api_english)
    * For any questions regarding sudden drop in performance with the API and/or Websockets.
    * For any general questions about the API not covered in the documentation.
* [Binance Developers](https://dev.binance.vision/)
    * For any questions/help regarding code implementation with API and/or Websockets.
* [Binance Customer Support](https://www.binance.com/en/support-center)
    * For cases such as missing funds, help with 2FA, etc.


```

{
    "contributes": {
        "x-github-workflows": [
            {
                "workflow": "deployments/azure-webapps-dotnet-core"
            }
        ]
    }
}

{
    "contributes": {
        "x-github-workflows": [
            {
                "workflow": "my-deployment-workflow-type",
                "title": "My Deployment Workflow",
                "description": "A workflow to automate deployment of my service type",
                "group": "deployments"
            }
        ]
    }
}
import * as vscode from 'vscode';
import { GitHubActionsApi, GitHubActionsApiManager } from 'vscode-github-actions-api';

export function activate(context) {
  context.subscriptions.push(
    vscode.commands.registerCommand(
      "my-extension.workflow.create",
      async () =>[devcontainer (2).json](https://github.com/user-attachments/files/16963157/devcontainer.2.json)
 {
        const gitHubActionsExtension = vscode.extensions.getExtension('cschleiden.vscode-github-actions');

        if (gitHubActionsExtension) {
          await gitHubActionsExtension.activate();

          const manager: GitHubActionsApiManager | undefined = gitHubActionsExtension.exports as GitHubActionsApiManager;

          if (manager) {
            const api: GitHubActionsApi | undefined = manager.getApi(1);

            if (api) {
              const workflowFiles = await api.createWorkflow(`deployments/azure-webapps-dotnet-core`);

              // Open all created workflow files...
              await Promise.all(workflowFiles.map(file => vscode.window.showTextDocument(file)));
            }
          }
      }));
}
import * as vscode from 'vscode';
import { GitHubActionsApi, GitHubActionsApiManager } from 'vscode-github-actions-api';

export function activate(context) {
    const gitHubActionsExtension = vscode.extensions.getExtension('cschleiden.vscode-github-actions');

    if (gitHubActionsExtension) {
        await gitHubActionsExtension.activate();

        const manager: GitHubActionsApiManager | undefined = gitHubActionsExtension.exports as GitHubActionsApiManager;

        if (manager) {
            const api: GitHubActionsApi | undefined = manager.getApi(1);

            if (api) {
                context.subscriptions.push(
                  api.registerWorkflowProvider(
                    'deployments/azure-webapps-dotnet-core',
                    {
                        createWorkflow: async (context): Promise<void> => {

                        const xml: string = // Get Azure publish profile XML...

                        await context.setSecret('AZURE_WEBAPP_PUBLISH_PROFILE', xml);

                        let content = context.content;

                        // Transform content (e.g. replace `your-app-name` with application name)...

                        await context.createWorkflowFile(context.suggestedFileName ?? 'azure-webapps-dotnet-core.yml', content);
                    }));
            }
         }
      }
}
