.. _design:

Design
======

This is a library of **immutable containers**.

These containers have all their methods marked ``const``.  Instead of
mutating them *in place*, they provide manipulation functions that
*return a new transformed value*, leaving the original value
unaltered.  In the context of data-structures, this property of
preserving old values is called **persistence**.

Most of these containers use data-structures in which these operations
can be done *efficiently*.  In particular, not all data is copied when
a new value is produced.  Instead, the new values may share,
internally, common data with other objects.  We sometimes refer to
this property as **structural sharing**.  This behaviour is
transparent to the user.

Assigment
---------

We are sorry, we lied. These containers provide *one mutating
operation*: **assignment** --- i.e. ``operator=``.

There is a good reason: without ``operator=`` everything becomes
complicated in C++.  For example, one may not contain non-assignable
types in many standard containers, assignment would also be disabled
from your custom types holding immutable containers, and so on and so
forth.

C++ is a multi-paradigm language with an imperative core.  Thus, it is
built on the foundation that *variables* can be mutated ---
i.e. assigned to.  We don't want to ride against this tide.  What we
want to prevent is in-place *object* manipulation.  Because of C++
semantics, *variable* assignment is defined in terms of *object
mutation*, so we have to provide *this very particular mutating
operation*, but nothing else.  Of course, you are free to mark your
variables ``const`` to completely forbid assignment.

.. warning::

   **Assignment is not thread safe**.  When a *mutable* variable is
   shared across multiple threads, protect access using some other
   mechanism.

   For obvious reasons, all other methods, which are ``const``, are
   thread-safe.  It is safe to share *immutable* state across multiple
   threads.

To ``const`` or not to ``const``
--------------------------------

Many C++ programmers, influenced by functional programming, are trying
to escape the evils of mutability by using ``const`` whenever
possible.  We also do it ourselves in many of our examples to
reinforce the property of immutability.

While in general this is a good practice backed up with very good
intentions, it has one caveat: *it disables moveability*.  It does so
even when ``std::move()`` is used.  This makes sense, since moving from
an object may mutate it, and ``const``, my friends, prevents *all*
mutations.  For example:

.. literalinclude:: ../example/vector/move.cpp
   :language: c++
   :start-after: move-bad/start
   :end-before:  move-bad/end

One may think that the variable ``v`` is moved into the
``push_back()`` call.  This is not the case, because the variable
``v`` is marked ``const``.  Of course, one may enable the move by
removing it, as in:

.. literalinclude:: ../example/vector/move.cpp
   :language: c++
   :start-after: move-good/start
   :end-before:  move-good/end

So, is it bad style then to use ``const`` as much as possible?  I
wouldn't say so and it is advisable when ``std::move()`` is not used.
An alternative style is to not use ``const`` but adopt an `AAA-style
<aaa>`_ (*Almost Always use Auto*).  This way, it is easy to look for
mutations by looking for lines that contain ``=`` but no ``auto``.
Remember that when using our immutable containers ``operator=`` is the
only way to mutate a variable.

.. _aaa: https://herbsutter.com/2013/08/12/gotw-94-solution-aaa-style-almost-always-auto/

.. admonition:: Why does ``const`` prevent move semantics?

   For those adventurous into the grainy details C++, here is why.
   ``std::move()`` does not move anything, it is just *a cast* from
   normal *l-value* references (``T&``) to *r-value* reference
   (``T&&``).  This is, you pass it a variable, and it returns a
   reference to its object disguised as an intermediate result.  In
   exchange, you promise not to do anything with this variable later
   [#f1]_. It is the role of the thing that *receives the moved-from
   value* (in the previous example, ``push_back``) to actually do
   anything interesting with it --- for example, steal its contents
   😈.

   So if you pass a ``T&`` to ``std::move()`` you get a ``T&&`` and,
   unsurprisingly, if you pass a ``const T&`` you get a ``const T&&``.
   But the receivers of the moved-from value (like constructors or our
   ``push_back()``) maybe be moved-into because they provide an
   overload that expects ``T&&`` --- without the ``const``!  Since a
   ``const T&&`` can not be converted into a ``T&&``, the compiler
   looks up for you another viable overload, and most often finds a
   copy constructor or something alike that expects a ``const T&`` or
   just ``T``, to which a ``const T&&`` can be converted.  The code
   compiles and works correctly, but it is less efficient than we
   expected.  Our call to ``std::move()`` was fruitless.

   .. [#f1] For the sake of completeness: it is actually allowed to do stuff
            with the variable *after another value is assigned to it*.

.. _move-semantics:

Leveraging move semantics
-------------------------

When using :ref:`reference counting<rc>` (which is the default)
mutating operations can often be faster when operating on *r-value
references* (temporaries and moved-from values).  Note that this
removes *persistence*, since one can not access the moved-from value
anymore!  However, this may be a good idea when doing a chain of
operations where the intermediate values are not important to us.

For example, let's say we want to write a function that inserts all
integers in the range :math:`[first, last)` into an immutable vector.
From the point of view of the caller of the function, this function is
a *transaction*.  Whatever intermediate vectors are generated inside
of it can be discarded since the caller can only see the initial
vector (the one passed in as argument) and the vector with *all* the
elements.  We may write such function like this:

.. literalinclude:: ../example/vector/iota-move.cpp
   :language: c++
   :start-after: myiota/start
   :end-before:  myiota/end

The intermediate values are *moved* into the next ``push_back()``
call.  They are going to be discarded anyways, this little
``std::move`` just makes the whole thing faster, letting ``push_back``
mutate part of the internal data structure in place when possible.

If you don't like this syntax, :doc:`transients<transients>` may be
used to obtain similar performance benefits.

.. admonition:: Assigment guarantees

   From the language point of view, the only requirement on moved from
   values is that they should still be destructible.  We provide the
   following two additional guarantees:

   - **It is valid to assign to a moved-from variable**.  The variable
     gets the assigned value and becomes usable again.  This is the
     behaviour of standard types.

   - **It is valid to assign a moved-from variable to itself**.  For
     most standard types this is *undefined behaviour*.  However, for our
     immutable containers types, expressions of the form ``v =
     std::move(v)`` are well-defined.

Recursive types
---------------

Most containers will fail to be instantiated with a type of unknown
size, this is, an *incomplete type*.  This prevents using them for
building recursive types.  The following code fails to compile:

.. code-block:: c++

  struct my_type
  {
      int data;
      immer::vector<my_type> children;
  };

However, we can easily workaround this by using an ``immer::box`` to wrap
the elements in the vector, as in:

.. code-block:: c++

  struct my_type
  {
      int data;
      immer::vector<immer::box<my_type>> children;
  };

.. admonition:: Standard containers and incomplete types

  While the first example might seem to compile when using some
  implementations of ``std::vector`` instead of ``immer::vector``, such
  use is actually forbidden by the standard:

    **17.6.4.8** *Other functions (...)* 2. the effects are undefined in
    the following cases: (...) In particular---if an incomplete type (3.9)
    is used as a template argument when instantiating a template
    component, unless specifically allowed for that component.

.. _batch-update:
Efficient batch manipulations
-----------------------------

Sometimes you may write a function that needs to do multiple changes
to a container.  Like most code you write with this library, this
function is *pure*: it takes one container value in, and produces a
new container value out, no side-effects.

Let's say we want to write a function that inserts all integers in the
range :math:`[first, last)` into an immutable vector:

.. literalinclude:: ../example/vector/iota-slow.cpp
   :language: c++
   :start-after: include:myiota/start
   :end-before:  include:myiota/end

This function works as expected, but it is slower than necessary.
On every loop iteration, a new value is produced, just to be
forgotten in the next iteration.

Instead, we can grab a mutable view on the value, a :ref:`transient`.
Then, we manipulate it *in-place*.  When we are done with it, we
extract back an immutable value from it.  The code now looks like
this:

.. _iota-transient:

.. literalinclude:: ../example/vector/iota-transient.cpp
   :language: c++
   :start-after: include:myiota/start
   :end-before:  include:myiota/end

Both conversions are :math:`O(1)`.  Note that calling ``transient()``
does not break the immutability of the variable it is called on.  The
new mutable object will adopt its contents, but when a mutation is
performed, it will copy the data necessary using *copy on write*.
Subsequent manipulations may hit parts that have already been copied,
and these changes are done in-place.  Because of this, it does not
make sense to use transients to do only one change.

.. tip::

   Note that :ref:`move semantics<move-semantics>` can be used instead to
   support a similar use-case.  However, transients optimise updates
   even when reference counting is disabled.

.. _std-compat:
Standard library compatibility
------------------------------

While the immutable containers provide an interface that follows a
functional style, this is incompatible with what the standard library
algorithms sometimes expect. :ref:`transients` try to provide an
interface as similar as possible to similar standard library
containers.  Thus, can also be used to interoperate with standard
library components.

For example the :ref:`myiota() function above<iota-transient>` may as
well be written using standard library tools:

.. literalinclude:: ../example/vector/iota-transient-std.cpp
   :language: c++
   :start-after: include:myiota/start
   :end-before:  include:myiota/end
