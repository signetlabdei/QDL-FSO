set(quantum_examples)
set(quantum_libraries)
if(q2ns
   IN_LIST
   contrib_libs_to_build
)
  set(quantum_examples
      qdl-fso-quantum-example
  )
  set(quantum_libraries
      ${libq2ns}
  )
else()
  message(
    STATUS "qdl-fso quantum adapter requires the q2ns module, but it was not found"
  )
endif()

set(example_sources
    ${quantum_examples}
    qdl-fso-transmissivity-example
)

foreach(example ${example_sources})
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK ${libqdlfso} ${quantum_libraries}
  )
endforeach()