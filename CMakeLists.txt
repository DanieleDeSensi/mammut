cmake_minimum_required(VERSION 3.5.1)
project(mammut VERSION 1.2.0)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_C_STANDARD 11)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules/")

###########
# Options #
###########
option (ENABLE_TESTS "Enables testing" OFF)
option (ENABLE_CPPCHECK "Enables cppcheck checks" OFF)
option (ENABLE_CODECOV "Enables code coverage reports" OFF)
option (ENABLE_CLANGFORMAT "Enables clang-format formatting" OFF)
option (ENABLE_DOCS "Enables documentation generation" OFF)
option (ENABLE_PYTHON "Enables Python binding generation generation" OFF)
option (ENABLE_REMOTE "Enables remote calls support" OFF)
option (ENABLE_PAPI "Enables PAPI support" OFF)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -finline-functions -O3") 
SET(CMAKE_CXX_FLAGS_DEBUG  "-Wall -finline-functions -O0 -g") 

# This must be the first thing done, since COVERAGE_COMPILER_FLAGS must be used by all the targets
###########
# codecov #
###########
if (ENABLE_CODECOV)
    set (CMAKE_BUILD_TYPE Debug)
    if (NOT ENABLE_TESTS)
        message (FATAL_ERROR "You need to define -DENABLE_TESTS=ON when you use -DENABLE_CODECOV=ON")
    endif()
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
endif (ENABLE_CODECOV)

########
# PAPI #
########
if (ENABLE_PAPI)
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_PAPI")
endif (ENABLE_PAPI)

###########
# Library #
###########
add_subdirectory(src)
add_subdirectory(demo)

############
# cppcheck #
############
if (ENABLE_CPPCHECK)
    include(cmake/cppcheck.cmake)
endif (ENABLE_CPPCHECK)

###########
# Testing #
###########
if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif (ENABLE_TESTS)

###########
# codecov #
###########
if (ENABLE_CODECOV)
    set(COVERAGE_GCOVR_EXCLUDES 'src/external/*' 'test/*' 'demo/*')
    SETUP_TARGET_FOR_COVERAGE_GCOVR_XML(
        NAME coverage
        EXECUTABLE make test
        DEPENDENCIES mammut
    )
endif (ENABLE_CODECOV)

################
# clang-format #
################
if (ENABLE_CLANGFORMAT)
  include(cmake/clang_format.cmake)
endif (ENABLE_CLANGFORMAT)

#######
# Doc #
#######
if (ENABLE_DOCS)
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        # set input and output files
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile)

        # request to configure the file
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        message("Doxygen build started")

        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
    else (DOXYGEN_FOUND)
      message(WARNING "Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif (ENABLE_DOCS)
