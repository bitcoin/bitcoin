:man_page: mongoc_find_and_modify_opts_t

mongoc_find_and_modify_opts_t
=============================

find_and_modify abstraction

Synopsis
--------

``mongoc_find_and_modify_opts_t`` is a builder interface to construct a `find_and_modify <https://docs.mongodb.org/manual/reference/command/findAndModify/>`_ command.

It was created to be able to accommodate new arguments to the MongoDB find_and_modify command.

As of MongoDB 3.2, the :symbol:`mongoc_write_concern_t` specified on the :symbol:`mongoc_collection_t` will be used, if any.

.. _mongoc_collection_find_and_modify_with_opts_example:

Example
-------

.. literalinclude:: ../examples/find_and_modify_with_opts/flags.c
   :language: c
   :caption: flags.c

.. literalinclude:: ../examples/find_and_modify_with_opts/bypass.c
   :language: c
   :caption: bypass.c

.. literalinclude:: ../examples/find_and_modify_with_opts/update.c
   :language: c
   :caption: update.c

.. literalinclude:: ../examples/find_and_modify_with_opts/fields.c
   :language: c
   :caption: fields.c

.. literalinclude:: ../examples/find_and_modify_with_opts/sort.c
   :language: c
   :caption: sort.c

.. literalinclude:: ../examples/find_and_modify_with_opts/opts.c
   :language: c
   :caption: opts.c

.. literalinclude:: ../examples/find_and_modify_with_opts/fam.c
   :language: c
   :caption: fam.c

Outputs:

.. code-block:: json

  {
      "lastErrorObject": {
          "updatedExisting": false,
          "n": 1,
          "upserted": {
              "$oid": "56562a99d13e6d86239c7b00"
          }
      },
      "value": {
          "_id": {
              "$oid": "56562a99d13e6d86239c7b00"
          },
          "age": 34,
          "firstname": "Zlatan",
          "goals": 342,
          "lastname": "Ibrahimovic",
          "profession": "Football player",
          "position": "striker"
      },
      "ok": 1
  }
  {
      "lastErrorObject": {
          "updatedExisting": true,
          "n": 1
      },
      "value": {
          "_id": {
              "$oid": "56562a99d13e6d86239c7b00"
          },
          "age": 34,
          "firstname": "Zlatan",
          "goals": 342,
          "lastname": "Ibrahimovic",
          "profession": "Football player",
          "position": "striker"
      },
      "ok": 1
  }
  {
      "lastErrorObject": {
          "updatedExisting": true,
          "n": 1
      },
      "value": {
          "_id": {
              "$oid": "56562a99d13e6d86239c7b00"
          },
          "age": 35,
          "firstname": "Zlatan",
          "goals": 342,
          "lastname": "Ibrahimovic",
          "profession": "Football player",
          "position": "striker"
      },
      "ok": 1
  }
  {
      "lastErrorObject": {
          "updatedExisting": true,
          "n": 1
      },
      "value": {
          "_id": {
              "$oid": "56562a99d13e6d86239c7b00"
          },
          "goals": 343
      },
      "ok": 1
  }
  {
      "lastErrorObject": {
          "updatedExisting": true,
          "n": 1
      },
      "value": {
          "_id": {
              "$oid": "56562a99d13e6d86239c7b00"
          },
          "age": 35,
          "firstname": "Zlatan",
          "goals": 343,
          "lastname": "Ibrahimovic",
          "profession": "Football player",
          "position": "striker",
          "author": true
      },
      "ok": 1
  }

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_find_and_modify_opts_append
    mongoc_find_and_modify_opts_destroy
    mongoc_find_and_modify_opts_get_bypass_document_validation
    mongoc_find_and_modify_opts_get_fields
    mongoc_find_and_modify_opts_get_flags
    mongoc_find_and_modify_opts_get_max_time_ms
    mongoc_find_and_modify_opts_get_sort
    mongoc_find_and_modify_opts_get_update
    mongoc_find_and_modify_opts_new
    mongoc_find_and_modify_opts_set_bypass_document_validation
    mongoc_find_and_modify_opts_set_fields
    mongoc_find_and_modify_opts_set_flags
    mongoc_find_and_modify_opts_set_max_time_ms
    mongoc_find_and_modify_opts_set_sort
    mongoc_find_and_modify_opts_set_update

