add_executable(activationPower activationPower.cpp)
target_link_libraries(activationPower LINK_PUBLIC mammut)

add_executable(demo-energy-extended demo-energy-extended.cpp)
target_link_libraries(demo-energy-extended LINK_PUBLIC mammut)

add_executable(joules joules.cpp)
target_link_libraries(joules LINK_PUBLIC mammut)

add_executable(joulesCpu joulesCpu.cpp)
target_link_libraries(joulesCpu LINK_PUBLIC mammut)

add_executable(launcher launcher.cpp)
target_link_libraries(launcher LINK_PUBLIC mammut)

add_executable(voltageTable voltageTable.cpp)
target_link_libraries(voltageTable LINK_PUBLIC mammut)

add_executable(cap cap.cpp)
target_link_libraries(cap LINK_PUBLIC mammut)
