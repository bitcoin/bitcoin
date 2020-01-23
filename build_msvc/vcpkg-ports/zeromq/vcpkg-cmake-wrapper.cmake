_find_package(${ARGS})

if(TARGET libzmq AND NOT TARGET libzmq-static)
    add_library(libzmq-static INTERFACE IMPORTED)
    set_target_properties(libzmq-static PROPERTIES INTERFACE_LINK_LIBRARIES libzmq)
elseif(TARGET libzmq-static AND NOT TARGET libzmq)
    add_library(libzmq INTERFACE IMPORTED)
    set_target_properties(libzmq PROPERTIES INTERFACE_LINK_LIBRARIES libzmq-static)
endif()
