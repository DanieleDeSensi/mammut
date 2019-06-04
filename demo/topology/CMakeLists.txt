add_executable(clockModulation clockModulation.cpp)
target_link_libraries(clockModulation LINK_PUBLIC mammut)

add_executable(demo-topology demo-topology.cpp)
target_link_libraries(demo-topology LINK_PUBLIC mammut)

add_executable(idleLevels idleLevels.cpp)
target_link_libraries(idleLevels LINK_PUBLIC mammut)

add_executable(idleLevelsPower idleLevelsPower.cpp)
target_link_libraries(idleLevelsPower LINK_PUBLIC mammut)

add_executable(offlining offlining.cpp)
target_link_libraries(offlining LINK_PUBLIC mammut)