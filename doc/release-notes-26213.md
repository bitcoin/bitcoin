Low-level changes
=================

- Previously `setban`, `addpeeraddress`, `walletcreatefundedpsbt`, methods
  allowed non-boolean and non-null values to be passed as boolean parameters.
  Any string, number, array, or object value that was passed would be treated
  as false. After this change, passing any value except `true`, `false`, or
  `null` now triggers a JSON value is not of expected type error. (#26213)
