# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xcc316e3f71a040fb;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("mp");

annotation include(file): Text;
annotation includeTypes(file): Text;
# Extra include paths to add to generated files.

annotation wrap(interface, struct): Text;
# Wrap capnp interface generating ProxyClient / ProxyServer C++ classes that
# forward calls to a C++ interface with same methods and parameters. Text
# string should be the name of the C++ interface.
# If applied to struct rather than an interface, this will generate a ProxyType
# struct with get methods for introspection and copying fields between C++ and
# capnp structs.

annotation count(param, struct, interface): Int32;
# Indicate how many C++ method parameters there are corresponding to one capnp
# parameter (default is 1). If not 1, multiple C++ method arguments will be
# condensed into a single capnp parameter by the client and then expanded by
# the server by CustomReadField/CustomBuildField overloads which need to be
# provided separately. An example would be a capnp Text parameter initialized
# from C++ char* and size arguments. Can be 0 to fill an implicit capnp
# parameter from client or server side context. If annotation is applied to an
# interface or struct type it will apply to all parameters of that type.

annotation exception(param): Text;
# Indicate that a result parameter corresponds to a C++ exception. Text string
# should be the name of a C++ exception type that the generated server class
# will catch and the client class will rethrow.

annotation name(field, method): Text;
# Name of the C++ method or field corresponding to a capnp method or field.

annotation skip(field): Void;
# Synonym for count(0).

interface ThreadMap $count(0) {
    # Interface letting clients control which thread a method call should
    # execute on. Clients create and name threads and pass the thread handle as
    # a call parameter.
    makeThread @0 (name :Text) -> (result :Thread);
}

interface Thread {
    # Thread handle returned by makeThread corresponding to one server thread.

    getName @0 () -> (result: Text);
}

struct Context $count(0) {
   # Execution context passed as a parameter from the client class to the server class.

   thread @0 : Thread;
   # Handle of the server thread the current method call should execute on.

   callbackThread @1 : Thread;
   # Handle of the client thread that is calling the current method, and that
   # any callbacks made by the server thread should be made on.
}
