set(name qdlfso)
enable_language(Fortran)

set(LOWTRAN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lowtran")

# Pin down where gfortran writes the generated .mod file so we know
# where to point include_directories() below.
set(CMAKE_Fortran_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lowtran-modules")

add_subdirectory(${LOWTRAN_SOURCE_DIR})

include_directories("${CMAKE_Fortran_MODULE_DIRECTORY}")

set(quantum_sources)
set(quantum_headers)
set(quantum_tests)
set(quantum_libraries)
if(q2ns
   IN_LIST
   contrib_libs_to_build
)
  set(quantum_sources
      model/qdl-fso-q2ns-qchannel.cc
      helper/qdl-fso-helper.cc
  )
  set(quantum_headers
      model/qdl-fso-q2ns-qchannel.h
      helper/qdl-fso-helper.h
  )
  set(quantum_tests
      #test/qdl-fso-quantum-test-suite.cc
  )
  set(quantum_libraries
      ${libq2ns}
  )
  add_compile_definitions(QDL_FSO_HAVE_QUANTUM=1)
else()
  message(
    STATUS "qdl-fso quantum adapter requires the q2ns module, but it was not found"
  )
endif()

set(source_files
    model/qdl-fso-numerical-integration.cc
    model/qdl-fso-optical-terminal.cc
    model/qdl-fso-atmospheric-model.cc
    model/qdl-fso-cn2-model.cc
    model/qdl-fso-rytov-model.cc
    model/qdl-fso-beam-wander-model.cc
    model/qdl-fso-geometric-pointing-error-model.cc
    model/qdl-fso-scintillation-model.cc
    model/qdl-fso-propagation-loss-model.cc
    ${quantum_sources}
    ${LOWTRAN_SOURCE_DIR}/lowtran_c_wrapper.f90
)

set(header_files
    model/qdl-fso-numerical-integration.h
    model/qdl-fso-optical-terminal.h
    model/qdl-fso-atmospheric-model.h
    model/qdl-fso-cn2-model.h
    model/qdl-fso-rytov-model.h
    model/qdl-fso-beam-wander-model.h
    model/qdl-fso-geometric-pointing-error-model.h
    model/qdl-fso-scintillation-model.h
    model/qdl-fso-propagation-loss-model.h
    ${quantum_headers}
)

set(test_sources
    ${quantum_tests}
    test/qdl-fso-test-suite.cc
)

build_lib(
  LIBNAME ${name}
  SOURCE_FILES ${source_files}
  HEADER_FILES ${header_files}
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libpropagation}
    ${libmobility}
    ${libbuildings}
    ${quantum_libraries}
    $<BUILD_INTERFACE:lowtran>
    gfortran
#  TEST_SOURCES ${test_sources}
)

set(libqdlfso ${name})

# Compile also the validations of the module when the test are enabled (small trick to make it work)
if(${ENABLE_TESTS})
  add_subdirectory(validation)
endif()

# If the examples are enabled, the compiler automatically reads the cmake in the examples folder, and compile accordingly