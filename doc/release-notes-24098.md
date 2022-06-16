Notable changes
===============

Updated REST APIs
-----------------

- The `/headers/` and `/blockfilterheaders/` endpoints have been updated to use
  a query parameter instead of path parameter to specify the result count. The
  count parameter is now optional, and defaults to 5 for both endpoints. The old
  endpoints are still functional, and have no documented behaviour change.

  For `/headers`, use
  `GET /rest/headers/<BLOCK-HASH>.<bin|hex|json>?count=<COUNT=5>`
  instead of
  `GET /rest/headers/<COUNT>/<BLOCK-HASH>.<bin|hex|json>` (deprecated)

  For `/blockfilterheaders/`, use
  `GET /rest/blockfilterheaders/<FILTERTYPE>/<BLOCK-HASH>.<bin|hex|json>?count=<COUNT=5>`
  instead of
  `GET /rest/blockfilterheaders/<FILTERTYPE>/<COUNT>/<BLOCK-HASH>.<bin|hex|json>` (deprecated)

  (#24098)
