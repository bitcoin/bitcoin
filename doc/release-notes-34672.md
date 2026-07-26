IPC Interface
-------------

- `BlockTemplate.submitSolution` now returns `reason` and `debug` rejection
  details in addition to the boolean result. Clients must regenerate IPC
  bindings from the updated `mining.capnp` schema to use the new method. The
  previous `@7` method now returns an error directing clients to update. (#34672)

- `BlockTemplate.submitSolution` now reports duplicate blocks as failures with
  `reason="duplicate"`, matching `Mining.submitBlock`, instead of returning
  success for duplicate submissions. (#34672)
