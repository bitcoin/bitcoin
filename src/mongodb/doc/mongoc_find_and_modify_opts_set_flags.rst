:man_page: mongoc_find_and_modify_opts_set_flags

mongoc_find_and_modify_opts_set_flags()
=======================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_find_and_modify_opts_set_flags (
     mongoc_find_and_modify_opts_t *opts,
     const mongoc_find_and_modify_flags_t flags);

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.
* ``flags``: .

Description
-----------

Adds one or more flags to the builder.

=================================  =============================================================================
MONGOC_FIND_AND_MODIFY_NONE        Default. Doesn't add anything to the builder.                                
MONGOC_FIND_AND_MODIFY_REMOVE      Will instruct find_and_modify to remove the matching document.               
MONGOC_FIND_AND_MODIFY_UPSERT      Update the matching document or, if no document matches, insert the document.
MONGOC_FIND_AND_MODIFY_RETURN_NEW  Return the resulting document.                                               
=================================  =============================================================================

Returns
-------

Returns Returns ``true`` if it successfully added the option to the builder, otherwise ``false`` and logs an error.

Setting flags
-------------

.. literalinclude:: ../examples/find_and_modify_with_opts/flags.c
   :language: c
   :caption: flags.c

Outputs:

.. code-block:: c

  {
     "lastErrorObject" : {
        "updatedExisting" : false,
        "n" : 1,
        "upserted" : {"$oid" : "56562a99d13e6d86239c7b00"}
     },
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

