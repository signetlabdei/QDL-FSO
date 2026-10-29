set(quantum_validations)
set(quantum_libraries)
if(q2ns
   IN_LIST
   contrib_libs_to_build
)
  set(quantum_validations
      qdl-fso-q2ns-qchannel-validation
  )
  set(quantum_libraries
      ${libq2ns}
  )
else()
  message(
    STATUS "qdl-fso quantum adapter requires the q2ns module, but it was not found"
  )
endif()

set(validation_sources
    qdl-fso-scintillation-validation
    qdl-fso-geometric-pointing-error-validation
    qdl-fso-beam-wander-validation
    qdl-fso-cn2-validation
    qdl-fso-lowtran-validation
    ${quantum_validations}
)

# Calculate the relative folder path for build outputs
string(
  REPLACE "${PROJECT_SOURCE_DIR}/"
          ""
          FOLDER
          "${CMAKE_CURRENT_SOURCE_DIR}"
)

foreach(validation ${validation_sources})
  message(STATUS "Registering validation: ${validation}")
  build_exec(
    EXECNAME ${validation}
    SOURCE_FILES ${validation}.cc
    LIBRARIES_TO_LINK ${libqdlfso} ${quantum_libraries}
    EXECUTABLE_DIRECTORY_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FOLDER}
  )
endforeach()