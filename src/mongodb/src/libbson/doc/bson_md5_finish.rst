:man_page: bson_md5_finish

bson_md5_finish()
=================

Synopsis
--------

.. code-block:: c

  void
  bson_md5_finish (bson_md5_t *pms, uint8_t digest[16]);

Parameters
----------

* ``pms``: A :symbol:`bson_md5_t`.
* ``digest``: A location for the digest.

Description
-----------

Completes the MD5 algorithm and stores the digest in ``digest``.

