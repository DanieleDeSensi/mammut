########################
# Src and header files #
########################
include(GNUInstallDirs)
include_directories(${PROJECT_SOURCE_DIR}/include)
set (EXTERNAL_INSTALL_LOCATION ${PROJECT_BINARY_DIR}/external)

file(GLOB SOURCES "*.cpp" "*.c" "cpufreq/*.cpp" "energy/*.cpp" "topology/*.cpp" "task/*.cpp")
list(REMOVE_ITEM SOURCES "${PROJECT_SOURCE_DIR}/src/mammut_pybind.cpp")
 
set(LINK_DEPS "pthread ${PROJECT_SOURCE_DIR}/src/external/odroid-smartpower-linux/libsmartgauge.a ${PROJECT_SOURCE_DIR}/src/external/libusb-1.0.9/libusb/.libs/libusb-1.0.a ${PROJECT_BINARY_DIR}/external/${CMAKE_INSTALL_LIBDIR}/libraplcap-msr.a")

###########
# RaplCap #
###########
include(ExternalProject)
ExternalProject_Add(raplcap_repo
    GIT_REPOSITORY https://github.com/powercap/raplcap.git
    GIT_TAG d21c2dc
    CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_C_FLAGS="-DRAPLCAP_LOG_LEVEL=OFF"
    UPDATE_COMMAND ""
)
include_directories(${EXTERNAL_INSTALL_LOCATION}/include)

########
# PAPI #
########
if (ENABLE_PAPI)
   find_library(PAPI_LIBRARY_NAMES papi)
   SET(LINK_DEPS "${LINK_DEPS} ${PAPI_LIBRARY_NAMES}")
endif (ENABLE_PAPI)


##################
# Remote support #
##################
if(ENABLE_REMOTE)
    find_package(Protobuf REQUIRED)
    include_directories(${Protobuf_INCLUDE_DIRS})
    foreach(module cpufreq energy topology)
        protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${module}/${module}-remote.proto)
        list(APPEND SOURCES ${PROTO_SRCS} ${PROTO_HDRS})
    endforeach(module)
    if(${CMAKE_VERSION} VERSION_LESS "3.6") 
        set(Protobuf_LIBRARIES ${PROTOBUF_LIBRARIES})
    endif()
    list(APPEND LINK_DEPS ${Protobuf_LIBRARIES})
endif()



####################
# Python interface #
####################
if(ENABLE_PYTHON)
    cmake_minimum_required(VERSION 2.8.12)
    execute_process(COMMAND git clone https://github.com/pybind/pybind11.git external/pybind11
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_subdirectory(external/pybind11)
    find_package(PythonLibs REQUIRED)
    pybind11_add_module(pymammut ${SOURCES} mammut_pybind.cpp)
    target_link_libraries(pymammut PRIVATE ${LINK_DEPS})
    target_include_directories(pymammut PUBLIC ${PROJECT_SOURCE_DIR}/src/external/pybind11/include/)
    target_include_directories(pymammut PUBLIC ${PYTHON_INCLUDE_DIRS})
endif()

####################
# Library building #
####################
add_library(mammut SHARED ${SOURCES})
add_library(mammut_static STATIC ${SOURCES})

# Generate protocol fields names before building
add_custom_target(
    compile_ext
    COMMAND make -C ${PROJECT_SOURCE_DIR}/src/external
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# Includes
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/mammut
        DESTINATION include)

# Dynamic Library
add_dependencies(mammut compile_ext raplcap_repo)
target_link_libraries(mammut ${LINK_DEPS})
install(TARGETS mammut LIBRARY DESTINATION lib)

# Static Library
add_dependencies(mammut_static compile_ext raplcap_repo)
target_link_libraries(mammut_static PRIVATE ${LINK_DEPS})
install(TARGETS mammut_static ARCHIVE DESTINATION lib)

# Install dependencies (raplcap)
install(DIRECTORY ${EXTERNAL_INSTALL_LOCATION}/include DESTINATION .)

####################
# Uninstall target #
####################
if(NOT TARGET uninstall)
    configure_file(
        "${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)
endif()
