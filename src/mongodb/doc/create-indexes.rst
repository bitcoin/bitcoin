:man_page: mongoc_create_indexes

Creating Indexes
================

To create indexes on a MongoDB collection, execute the ``createIndexes`` command
with :symbol:`mongoc_database_write_command_with_opts`. See `the MongoDB
Manual entry for the createIndexes command
<https://docs.mongodb.com/manual/reference/command/createIndexes/>`_ for details.

Example
-------

.. literalinclude:: ../examples/example-create-indexes.c
   :language: c
   :caption: example-create-indexes.c
