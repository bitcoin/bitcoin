New RPCs
--------

- A new REST API endpoint (`/rest/txfromblock/BLOCKHASH-OFFSET`) has been introduced for
  efficiently fetching a specific transaction for a given block and offset (#32541).
- An optional transaction locations' index (`-locationsindex`)  has been introduced for
  improving transaction read performance using the new REST API (#32541).
