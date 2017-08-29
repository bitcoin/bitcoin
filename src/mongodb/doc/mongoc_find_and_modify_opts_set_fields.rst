:man_page: mongoc_find_and_modify_opts_set_fields

mongoc_find_and_modify_opts_set_fields()
========================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_find_and_modify_opts_set_fields (mongoc_find_and_modify_opts_t *opts,
                                          const bson_t *fields);

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.
* ``fields``: A subset of fields to return. Choose which fields to include by appending ``{fieldname: 1}`` for each fieldname, or excluding it with ``{fieldname: 0}``.

Description
-----------

Adds fields argument to the builder.

``fields`` does not have to remain valid after calling this function.

Returns
-------

Returns ``true`` if it successfully added the option to the builder, otherwise ``false``.

Setting fields
--------------

.. literalinclude:: ../examples/find_and_modify_with_opts/fields.c
   :language: c
   :caption: fields.c

Outputs:

.. code-block:: c

  {
     "lastErrorObject"
        : {"updatedExisting" : true, "n" : 1},
          "value"
          : {"_id" : {"$oid" : "56562a99d13e6d86239c7b00"}, "goals" : 343},
            "ok" : 1
  }

