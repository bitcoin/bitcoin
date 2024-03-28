JSON-RPC
--------

The JSON-RPC server now recognizes JSON-RPC 2.0 requests and responds with
strict adherence to the specification (https://www.jsonrpc.org/specification):

- Returning HTTP "204 No Content" responses to JSON-RPC 2.0 notifications instead of full responses.
- Returning HTTP "200 OK" responses in all other cases, rather than 404 responses for unknown methods, 500 responses for invalid parameters, etc.
- Returning either "result" fields or "error" fields in JSON-RPC responses, rather than returning both fields with one field set to null.
