:man_page: mongoc_find_and_modify_opts_set_sort

mongoc_find_and_modify_opts_set_sort()
======================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_find_and_modify_opts_set_sort (mongoc_find_and_modify_opts_t *opts,
                                        const bson_t *sort);

Parameters
----------

* ``opts``: A :symbol:`mongoc_find_and_modify_opts_t`.
* ``sort``: Determines which document the operation modifies if the query selects multiple documents. findAndModify modifies the first document in the sort order specified by this argument.

Description
-----------

Adds sort argument to the builder.

``sort`` does not have to remain valid after calling this function.

Returns
-------

Returns ``true`` if it successfully added the option to the builder, otherwise ``false``.

Setting sort
------------

.. literalinclude:: ../examples/find_and_modify_with_opts/sort.c
   :language: c
   :caption: sort.c

Outputs:

.. code-block:: c

  {
     "lastErrorObject" : {"updatedExisting" : true, "n" : 1},
                         "value" : {
                            "_id" : {"$oid" : "56562a99d13e6d86239c7b00"},
                            "age" : 35,
                            "firstname" : "Zlatan",
                            "goals" : 343,
                            "lastname" : "Ibrahimovic",
                            "profession" : "Football player",
                            "position" : "striker",
                            "author" : true
                         },
                                   "ok" : 1
  }

