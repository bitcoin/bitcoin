Tools and Utilities
---

- A new `bitcoinconsensus_verify_script_with_spent_outputs` function is available in libconsensus which optionally accepts the spent outputs of the transaction being verified.
- A new `bitcoinconsensus_SCRIPT_FLAGS_VERIFY_TAPROOT` flag is available in libconsensus that will verify scripts with the Taproot spending rules.