
add_library(Settings
    Configuration/ConfigurationStoreInterface.h
    Configuration/ConfigurationIoExecutor.cc
    Configuration/ConfigurationIoExecutor.h
    Configuration/JsonConfigurationStore.cc
    Configuration/JsonConfigurationStore.h
	ADSBVehicleManagerSettings.cc
	ADSBVehicleManagerSettings.h
	APMMavlinkStreamRateSettings.cc
	APMMavlinkStreamRateSettings.h
	AppSettings.cc
	AppSettings.h
	AutoConnectSettings.cc
	AutoConnectSettings.h
	BrandImageSettings.cc
	BrandImageSettings.h
	FirmwareUpgradeSettings.cc
	FirmwareUpgradeSettings.h
	GimbalControllerSettings.cc
	GimbalControllerSettings.h
	FlightMapSettings.cc
	FlightMapSettings.h
	FlyViewSettings.cc
	FlyViewSettings.h
	NTRIPSettings.cc
	NTRIPSettings.h
	OfflineMapsSettings.cc
	OfflineMapsSettings.h
	PlanViewSettings.cc
	PlanViewSettings.h
	RTKSettings.cc
	RTKSettings.h
	SettingsGroup.cc
	SettingsGroup.h
	SettingsManager.cc
	SettingsManager.h
	UnitsSettings.cc
	UnitsSettings.h
	VideoSettings.cc
	VideoSettings.h
	RemoteIDSettings.h
	RemoteIDSettings.cc
)

target_link_libraries(Settings
	PUBLIC
		qgc
		Qt5::Multimedia
		Qt5::Concurrent
)

target_include_directories(Settings PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

