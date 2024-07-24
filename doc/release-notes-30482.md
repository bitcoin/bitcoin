Updated REST APIs
-----------------
- Parameter validation for `/rest/getutxos` has been improved by rejecting
  truncated or overly large txids and malformed outpoint indices by raising an
  HTTP_BAD_REQUEST "Parse error". Previously, these malformed requests would be
  silently handled. (#30482, #30444)
