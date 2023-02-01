# Generic Arith Classes

## What is it?
`Generic Arith Classes` is a loosely defined interface that consists of `Scalar`, `Point` and `Initializer` class aliases where `Scalar` class represents a scalar value and the arithmetic, `Point` class represents a curve point and the associated operations, and `Initializer` class represents initializers of the underlying libraries used by `Scalar` and `Point` classes.

## Implementation class structure
An implementation of `Generic Arith Classes` should have one top-level structure. The top-level structure should expose `Scalar`, `Point` and `Initializer` class aliases, and let each alias point to the actual implementation. Nothing else is enforced by `Generic Arith Classes` interface.

For example, [struct Mcl](../arith//mcl/mcl.h) below is a top-level structure of the `Mcl` classes. Underneath it, there exist [MclScalar](../arith/mcl/mcl_scalar.h), [MclG1Point](../arith/mcl/mcl_g1point.h) and [MclInitializer](../arith/mcl/mcl_initializer.h) which are aliases of `Scalar`, `Point` and `Initializer` respectively.

```c++
struct Mcl
{
  using Point = MclG1Point;
  using Scalar = MclScalar;
  using Initializer = MclInitializer;
};
```

`Generic Arith Classes` doesn't define how the `Mcl` classes should actually be implemented.

## An implementation as an interface
[Mcl](../arith//mcl/mcl.h) classes are designed to be generic so that it can be used to implement other cryptographic logic. So, other `Generic Arith Classes` implementations sharing the same API as [Mcl](../arith//mcl/mcl.h) can replace [Mcl](../arith//mcl/mcl.h) in the user classes of [Mcl]().

For instance, [RangeProofLogic](../range_proof/range_proof_logic.h) that currently uses an implementation based on `libmcl` library can later switch to a new implementation based on `libsecp256k1` library without changing the user code aside from the type parameter as long as the new implementation has the same set of API as [Mcl](../arith/mcl/mcl.h).

This effectively makes the [Mcl](../arith//mcl/mcl.h) implementation an interface.

## Adding a new implementation
### Based on existing implementation
1. Copy existing top-level structure and the headers of aliased classes to a different directory.
2. Rename them.
3. Add implementation of all the functions defined in the headers.

### From scratch
Below is one way of developing a new implementation of `Generic Arith Classes`:

1. Define the top-level structure that exposes aliases to its associated `Scalar`, `Point`, and `Initializer` classes. Let each alias point to an empty class.

2. Define a user class that takes the top-level class as a type parameter.

3. Obtain `Scalar`, `Point` and `Initializer` classes from the type parameter in the user class. If `Scalars` and/or `Points` class(es) are needed, use `Elements` class which is a helper class to generate a vector class of `Scalar` or `Point` with commonly used functions.

   Below is an example taken from [range_proof_logic.h](../range_proof/range_proof_logic.h):

    ```c++
    template <typename T>
    RangeProofLogic<T>::RangeProofLogic()
    {
        ...
        using Initializer = typename T::Initializer;
        using Scalar = typename T::Scalar;
        using Point = typename T::Point;
        using Scalars = Elements<Scalar>;
        ...
    ```

4. Implement logic in the user class using the `Generic Arith Classes` implementation classes adding functions to them as needed.

## Example implementation and user class
### Implementation
- [Mcl](../arith/mcl/mcl.h) -- the top-level class of `Generic Arith Classes` implementation
- [MclScalar](../arith/mcl/mcl_scalar.h) -- `Scalar` implementation of `Mcl`
- [MclG1Point](../arith/mcl/mcl_g1point.h) -- `Point` implementation of `Mcl`
- [MclInitializer](../arith/mcl/mcl_initializer.h) -- `Initializer` implementation of `Mcl`

### User Class
- [RangeProofLogic](../range_proof/range_proof.h) -- User class of `Mcl` classes

## Design candidates in the development process
1. Interface based on abstract classes: it requires usage of pointers that makes the code harder to write and requires memory management. It also involves a cost of vtable look-up. *Not adopted*.

2. CRTP (Curiously Recurring Template Pattern) pattern: it was hard to use for this use case involving nested classes that implemented arithmetic functions that returned an instance of a nested class. *Not adopted*.

3. Template class with associated classes: there is no interface to enforce an API and the usage will be based on duck-typing. But it allows local stack-allocated instances to be used and bears no additional run-time cost. *Adopted*.