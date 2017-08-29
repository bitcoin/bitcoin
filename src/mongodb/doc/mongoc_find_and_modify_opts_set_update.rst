:man_page: mongoc_find_and_modify_opts_set_update

mongoc_find_and_modify_opts_set_update()
========================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_find_and_modify_opts_set_update (mongoc_find_and_modify_opts_t *opts,
                                          const bson_t *update);

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.
* ``update``: The ``update`` document is the same format as the ``update`` document passed to :symbol:`mongoc_collection_update`.

Description
-----------

Adds update argument to the builder.

``update`` does not have to remain valid after calling this function.

Returns
-------

Returns ``true`` if it successfully added the option to the builder, otherwise ``false``.

Setting update
--------------

.. literalinclude:: ../examples/find_and_modify_with_opts/update.c
   :language: c
   :caption: update.c

Outputs:

.. code-block:: c

  {
     "lastErrorObject" : {"updatedExisting" : true, "n" : 1},
                         "value" : {
                            "_id" : {"$oid" : "56562a99d13e6d86239c7b00"},
                            "age" : 35,
                            "firstname" : "Zlatan",
                            "goals" : 342,
                            "lastname" : "Ibrahimovic",
                            "profession" : "Football player",
                            "position" : "striker"
                         },
                                   "ok" : 1
  }

