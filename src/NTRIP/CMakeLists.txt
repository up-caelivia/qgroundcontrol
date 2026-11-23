add_library(NTRIP
    NTRIPConfiguration.h
    NTRIPConfigurationCodec.cc
    NTRIPConfigurationCodec.h
    NTRIPConfigurationMigration.cc
    NTRIPConfigurationMigration.h
    NTRIPConfigurationQmlAdapter.cc
    NTRIPConfigurationQmlAdapter.h
    NTRIPConfigurationValidation.cc
    NTRIPConfigurationValidation.h
    NTRIPConnection.cc
    NTRIPConnection.h
    NTRIPManager.cc
    NTRIPManager.h
)

if(BUILD_TESTING)
    target_sources(NTRIP PRIVATE NTRIPManagerTest.cc NTRIPManagerTest.h)
endif()

target_link_libraries(NTRIP
    PUBLIC
        qgc
)

target_include_directories(NTRIP
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
