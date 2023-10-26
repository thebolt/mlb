# Custom cmake target for creating a target using nala for unit testing.

execute_process(
  COMMAND nala include_dir
  RESULT_VARIABLE NALA_INCLUDE_RESULT
  OUTPUT_VARIABLE NALA_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT NALA_INCLUDE_RESULT EQUAL "0")
  message(FATAL_ERROR "nala include_dir failed with ${NALA_INCLUDE_RESULT}")
endif()

execute_process(
  COMMAND nala c_sources
  RESULT_VARIABLE NALA_CSOURCES_RESULT
  OUTPUT_VARIABLE NALA_CSOURCES
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT NALA_CSOURCES_RESULT EQUAL "0")
  message(FATAL_ERROR "nala c_sources failed with ${NALA_CSOURCES_RESULT}")
endif()

include(CTest)

function(add_nala_tests)
  set(oneValueArgs TARGET)
  set(multiValueArgs SOURCES)

  set(allKeywords ${options} ${oneValueArgs} ${multiValueArgs})
  if("${ARGV0}" IN_LIST allKeywords)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN})
  else()
    message(FATAL_ERROR "add_nala_tests without the required arguments")
  endif()

  if(NOT TARGET "${ARGS_TARGET}" AND NOT ARGS_SOURCES)
    message(
      FATAL_ERROR "${ARGS_TARGET} does not define an existing CMake target")
  endif()
  if(NOT ARGS_WORKING_DIRECTORY)
    unset(workDir)
  else()
    set(workDir WORKING_DIRECTORY "${ARGS_WORKING_DIRECTORY}")
  endif()

  if(NOT ARGS_SOURCES)
    get_property(
      ARGS_SOURCES
      TARGET ${ARGS_TARGET}
      PROPERTY SOURCES)
  endif()

  set(nala_case_name_regex ".*\\( *([A-Za-z_0-9]+) *\\).*")
  set(nala_test_type_regex "TEST")

  unset(testList)

  foreach(source IN LISTS ARGS_SOURCES)
    if(NOT ARGS_SKIP_DEPENDENCY)
      set_property(
        DIRECTORY
        APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS ${source})
    endif()

    cmake_path(GET source FILENAME source_file)
    if(${source_file} STREQUAL "nala_mocks.c")
      continue()
    endif()

    file(READ "${source}" contents)
    string(REGEX MATCHALL "${nala_test_type_regex} *\\(([A-Za-z_0-9 ,]+)\\)"
                 found_tests "${contents}")

    foreach(hit ${found_tests})
      string(REGEX REPLACE ${nala_case_name_regex} "\\1" nala_test_name ${hit})

      set(ctest_test_name
          ${ARGS_TEST_PREFIX}${nala_test_name}${ARGS_TEST_SUFFIX})

      add_test(NAME ${ctest_test_name} ${workDir}
               COMMAND ${ARGS_TARGET} "${nala_test_name}$" ${ARGS_EXTRA_ARGS})

      list(APPEND testList ${ctest_test_name})
    endforeach()
  endforeach()

  if(ARGS_TEST_LIST)
    set(${ARGS_TEST_LIST}
        ${testList}
        PARENT_SCOPE)
  endif()
endfunction()

function(add_nala_target)
  set(oneValueArgs TARGET TARGET_LIB)
  set(multiValueArgs SOURCES)

  set(allKeywords ${options} ${oneValueArgs} ${multiValueArgs})
  if("${ARGV0}" IN_LIST allKeywords)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN})
  else()
    message(FATAL_ERROR "add_nala_target without the required arguments")
  endif()

  # Build the test executable.
  add_executable(${ARGS_TARGET} ${ARGS_SOURCES})

  target_sources(${ARGS_TARGET} PRIVATE ${NALA_CSOURCES})

  target_include_directories(${ARGS_TARGET} PRIVATE ${NALA_INCLUDE_DIR})
  target_include_directories(${ARGS_TARGET}
                             PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/nala)
  target_link_libraries(${ARGS_TARGET} ${ARGS_TARGET_LIB})

  target_compile_options(${ARGS_TARGET} PRIVATE -fsanitize=address)
  target_link_options(${ARGS_TARGET} PRIVATE -fsanitize=address)

  add_custom_command(
    OUTPUT nala/nala_mocks.c nala/nala_mocks.h
    COMMAND
      nala cat ${ARGS_SOURCES} | ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS}
      "-I$<JOIN:$<TARGET_PROPERTY:${ARGS_TARGET},INCLUDE_DIRECTORIES>,;-I>" -E -
      | nala generate_mocks -o ${CMAKE_CURRENT_BINARY_DIR}/nala
    DEPENDS ${ARGS_SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND_EXPAND_LISTS VERBATIM)
  target_sources(${ARGS_TARGET}
                 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/nala/nala_mocks.c)
  target_compile_definitions(${ARGS_TARGET} PRIVATE NALA_INCLUDE_NALA_MOCKS_H)
  target_link_options(${ARGS_TARGET} PRIVATE
                      @${CMAKE_CURRENT_BINARY_DIR}/nala/nala_mocks.ldflags)

  add_nala_tests(TARGET ${ARGS_TARGET})
  # add_test(NAME nala_test COMMAND ${ARGS_TARGET} WORKING_DIRECTORY
  # ${CMAKE_CURRENT_BINARY_DIR}) This adds the "make test" target.
  # add_custom_target(${TARGET_NAME} COMMAND ${ARGS_TARGET} WORKING_DIRECTORY
  # ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Running tests.")
endfunction()
