
#  Targets
#  =======

add_custom_target(doxygen
  COMMAND doxygen doxygen.config
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_target(docs
  COMMAND make html
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_dependencies(docs doxygen)

set(immer_ssh_method
  ssh -p 5488
      -o StrictHostKeyChecking=no)

add_custom_target(upload-docs
  COMMAND
  rsync -av -e \"${immer_ssh_method}\"
        ${CMAKE_CURRENT_SOURCE_DIR}/_build/html/*
        raskolnikov@sinusoid.es:public/immer/)
