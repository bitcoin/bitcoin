:man_page: mongoc_find_and_modify_opts_set_bypass_document_validation

mongoc_find_and_modify_opts_set_bypass_document_validation()
============================================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_find_and_modify_opts_set_bypass_document_validation (
     mongoc_find_and_modify_opts_t *opts, bool bypass);

This option is only available when talking to MongoDB 3.2 and later.

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.
* ``bypass``: If the schema validation rules should be ignored.

Description
-----------

Adds bypassDocumentValidation argument to the builder.

When authentication is enabled, the authenticated user must have either the "dbadmin" or "restore" roles to bypass document validation.

Returns
-------

Returns ``true`` if it successfully added the option to the builder, otherwise ``false`` and logs an error.

Setting bypassDocumentValidation
--------------------------------

.. literalinclude:: ../examples/find_and_modify_with_opts/bypass.c
   :language: c
   :caption: bypass.c

Outputs:

.. code-block:: c

  {
     "lastErrorObject" : {"updatedExisting" : true, "n" : 1},
                         "value" : {
                            "_id" : {"$oid" : "56562a99d13e6d86239c7b00"},
                            "age" : 34,
                            "firstname" : "Zlatan",
                            "goals" : 342,
                            "lastname" : "Ibrahimovic",
                            "profession" : "Football player",
                            "position" : "striker"
                         },
                                   "ok" : 1
  }

