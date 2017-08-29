:man_page: bson_errors

Handling Errors
===============

Description
-----------

Many libbson functions report errors by returning ``NULL`` or -1 and filling out a :symbol:`bson_error_t` structure with an error domain, error code, and message.

* ``error.domain`` names the subsystem that generated the error.
* ``error.code`` is a domain-specific error type.
* ``error.message`` describes the error.

Some error codes overlap with others; always check both the domain and code to determine the type of error.

=====================  ======================================  ==================================================================================================
``BSON_ERROR_JSON``    ``BSON_JSON_ERROR_READ_CORRUPT_JS``     :symbol:`bson_json_reader_t` tried to parse invalid MongoDB Extended JSON.
                       ``BSON_JSON_ERROR_READ_INVALID_PARAM``  Tried to parse a valid JSON document that is invalid as MongoDBExtended JSON.
                       ``BSON_JSON_ERROR_READ_CB_FAILURE``     An internal callback failure during JSON parsing.
``BSON_ERROR_READER``  ``BSON_ERROR_READER_BADFD``             :symbol:`bson_json_reader_new_from_file` could not open the file.
=====================  ======================================  ==================================================================================================

