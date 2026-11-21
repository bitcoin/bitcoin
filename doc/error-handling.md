# Bitcoin Core Error Handling Strategy

## Purpose

Bitcoin Core needs a precise and uniform distinction between **results**,
**errors**, and **defects**.

This distinction is especially important in consensus-critical code. Validation
is an operation whose purpose is to determine whether something satisfies the
consensus rules. An invalid block is therefore a **successful result of
validation**, not an error. Confusing these categories can cause a node to make
a different consensus decision from other nodes. In 2013, this distinction was
violated when an exception thrown by the database was caught and interpreted as
a negative validation result, contributing to an accidental chain split.

This document defines the error-handling strategy toward which all Bitcoin Core
code should converge, to avoid similar incidents in the future.

## Strategy

The fundamental rule is:

> **Return values represent successful outcomes. Exceptions represent failures
> to perform the operation. Defects terminate the affected execution.**

### Results

A function returns a value when it successfully performs its specified operation.

The value represents the answer to the question the function was asked.

For example:

```cpp
BlockValidationState ValidateBlock(const Block& block);
```

can return a state indicating that the block is valid or invalid. Both are
results of successfully performing validation.

Likewise, absence, rejection, or other negative domain outcomes are result
values whenever they are part of the operation's specification.

### Errors

An **error** occurs when an operation cannot fulfill its specified contract due
to a condition that is outside the correctness of the program itself.

Examples include an I/O operation that cannot access required data or an
external resource that is unavailable.

Errors are propagated using **exceptions**.

Exceptions keep the normal execution path free of error-state propagation. The
cost of handling an error is paid when an error actually occurs rather than on
every successful operation.

This is particularly appropriate for a Bitcoin node: block validation occurs
relatively infrequently, but when a block arrives it should be completed as
quickly as possible so the node can return to its predominantly idle state.

New code must not introduce `std::expected` or another return-value mechanism
for propagating errors.

### Defects

A **defect** is a violation of a requirement imposed by the program itself.
Examples include violating an object invariant, violating a function
precondition, or reaching a state that the implementation guarantees to be
impossible.

Defects must not be represented as recoverable errors. They should be detected
with assertions, contracts, or equivalent fail-fast mechanisms.

The distinction is therefore based on why the operation did not produce its
result, not on whether the result is desirable or how frequently it occurs:

| Situation                                          | Mechanism                        |
| -------------------------------------------------- | -------------------------------- |
| Operation completed and produced a domain outcome  | Return value                     |
| Operation could not fulfill its specified contract | Exception                        |
| Program violated its own requirements              | Assertion / contract / fail-fast |

The term **defect** is preferred to *programming error* in this document because
*error* already has a specific meaning above. *Bug* is appropriate when
discussing a defect informally, but **defect** is the architectural term.

## Proper Exception handling

Exceptions should propagate until reaching a layer that can meaningfully respond
to the error.

Code must not catch an exception to convert it into a return value. A catch is
appropriate when the code can recover or translate the error at an abstraction
boundary.

Exception safety is part of the design of every operation. All operations must
provide an appropriate exception safety guarantee: **no-throw**, **strong**, or
**basic**. In particular, an exception must not leave any object in a state that
violates its invariants.

## Proper Defect handling

A defect means that the program can no longer rely on its state satisfying its
requirements. A defect must therefore not trigger a graceful shutdown that
persists or flushes that state to durable storage.

Doing so could turn an in-memory defect into persistent corruption: state that
was already known to be invalid could overwrite previously valid state in the
database, making recovery from the defect substantially harder or impossible.

The response to a defect must terminate execution without performing further
state transitions that depend on the affected state.

## Convergence

This strategy is the target architecture for the entire Bitcoin Core codebase.

New interfaces must follow it. Existing interfaces should move toward it when
they are redesigned or substantially refactored.

Historical use of `bool`, output parameters, error strings, error callbacks,
result types with an error state, or other error-propagation mechanisms does not
justify introducing additional alternatives.
