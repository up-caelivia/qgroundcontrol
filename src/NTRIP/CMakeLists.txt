add_library(NTRIP
    NTRIPConfiguration.h
    NTRIPConnection.cc
    NTRIPConnection.h
    NTRIPManager.cc
    NTRIPManager.h
    NTRIPStatusController.cc
    NTRIPStatusController.h
)

target_link_libraries(NTRIP
    PUBLIC
        qgc
)

target_include_directories(NTRIP
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
