# additional target to perform cppcheck run, requires cppcheck
find_package(cppcheck)

if(!CPPCHECK_FOUND)
  message("cppcheck not found. Please install it to run 'make cppcheck'")
endif()

add_custom_target(
        cppcheck
        COMMAND ${CPPCHECK_EXECUTABLE} --xml --xml-version=2 --enable=warning,performance,information,style -D__linux__ --error-exitcode=1  ${PROJECT_SOURCE_DIR} --suppress="*:*.pb.cc" -i${PROJECT_SOURCE_DIR}/src/external -i${PROJECT_SOURCE_DIR}/build -i${PROJECT_SOURCE_DIR}/cmake -i${PROJECT_SOURCE_DIR}/include/mammut/external -i${PROJECT_SOURCE_DIR}/test 2> cppcheck-report.xml || (cat cppcheck-report.xml; exit 2)
)


