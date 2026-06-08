\page bitcoinkernel_examples Bitcoinkernel Examples

Though these examples are provided, users from other programming languages should use wrappers for their respective languages. These will often provide simplified and more ergonomic interfaces, with less manual memory management.

The kernel library supports three fundamental features:

1. "Context-free" validation
2. Full validation
3. Bitcoin Core data retrieval

Full validation applies the complete validation rules that Bitcoin Core uses to validate transactions and blocks. Currently, this includes block processing. These functions read and write from and to disk.

The library also exposes a bunch of utilities for reading data. This includes iterating over the indexed blocks and reading them from disk.

\section bitcoinkernel_context_free Context-free validation

This describes validation functions for which the user of the library is not required to explicitly instantiate a btck_Context. These functions can validate data structures only with a subset of the full Bitcoin Core rules. They do not read or write data from or to disk. Currently, this includes script validation.

The user does not need to setup callbacks or other more stateful infrastructure to use these functions.

The following example validates a spend of a taproot output:

\include context_free_example.c

Note that a Null pointer may be passed to spent_outputs if the taproot bit is not set in the flags bitfield and the amount may be zero if the segwit bit is not set.

\section bitcoinkernel_full_validation Full validation

Full validation requires the user to instantiate a btck_Context. It is recommended to create a kernel notification callbacks struct to handle any fatal errors arrising during processing. If a fatal error is encountered it is recommended to halt further processing and tear down.

The below example is a bare-bones setup for processing a single block on regtest. It starts out with creating a context. The context requires a context option, which is setup with callbacks for the notification and validation interface, and targets regtest. Setting the callbacks is optional, but recommended, and done here for demonstration purposes.

Then the context is used to create a chainstate manager together with the required options for the chainstate manager. Note that if the directories the chainstate manager and block manager options are configured with don't exist yet, will be created.

Finally, a block is created from its raw data and then processed by the chainstate manager. The new_block variable is used to denote whether a block has been processed before or not. If it is a duplicate new_block will be set to false.

\include full_validation_example.c

\section bitcoinkernel_data_retrieval Bitcoin Core data retrieval

The only difference when using the kernel library just for data retrieval compared to the "Full" validation case is that the chainstate manager does not issue validation interface callbacks, so no such callbacks need to be configured.

Once retrieved there are no restrictions on the lifetime of the data for the user. It can be retained for later processing, serialized, or used to more data.

\include data_retrieval_example.c
