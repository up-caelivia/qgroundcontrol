message("Adding Custom Plugin")

#-- Version control
# Build number is automatic

exists($$PWD/create_ABLUO) {
    # Get the latest tag matching the pattern v*-ABLUO
    GIT_TAG = $$system(git describe --tags --abbrev=0 --match "v*-ABLUO")
} else {
    # Get the latest tag matching the pattern v*-UP
    GIT_TAG = $$system(git describe --tags --abbrev=0 --match "v*-UP")
}

# Get short commit hash
GIT_HASH = $$system(git rev-parse --short HEAD)

# Count number of commits since the latest tag
COMMITS_FROM_TAG = $$system(git rev-list $${GIT_TAG}..HEAD --count)

# Compose version string
equals(COMMITS_FROM_TAG, 0) {
    # We're on the tag itself — version is the tag name
    CUSTOM_QGC_VERSION = $${GIT_TAG}
} else {
    # Not on the tag — include commit count and hash
    CUSTOM_QGC_VERSION = $${GIT_TAG}-$${COMMITS_FROM_TAG}-$${GIT_HASH}
}

ANDROID_MIN_SDK_VERSION = 21

DEFINES -= APP_VERSION_STR=\"\\\"$$APP_VERSION_STR\\\"\"

# Platform-specific substitution for Windows
win32 {
    VERSION_CLEAN = $$replace(CUSTOM_QGC_VERSION, ^v, )
    VERSION_SPLIT_DASH = $$split(VERSION_CLEAN, -) 
    VERSION_NUMBERS = $$first(VERSION_SPLIT_DASH) 
    VERSION_PARTS = $$split(VERSION_NUMBERS, .) 

    FILEVER_MAJOR = $$member(VERSION_PARTS, 0)
    FILEVER_MINOR = $$member(VERSION_PARTS, 1)
    FILEVER_PATCH = $$member(VERSION_PARTS, 2)

    VERSION = $$sprintf("%1.%2.%3", $$FILEVER_MAJOR, $$FILEVER_MINOR, $$FILEVER_PATCH)

} 
DEFINES += APP_VERSION_STR=\"\\\"$$CUSTOM_QGC_VERSION\\\"\"

message(Custom QGC Version: $${CUSTOM_QGC_VERSION})
message(APP_VERSION_STR: $${APP_VERSION_STR})
message(VERSION: $${VERSION})

# Build a single flight stack by disabling APM support
# CONFIG  += QGC_DISABLE_APM_MAVLINK
# CONFIG  += QGC_DISABLE_APM_PLUGIN QGC_DISABLE_APM_PLUGIN_FACTORY

# We implement our own PX4 plugin factory
# CONFIG  += QGC_DISABLE_PX4_PLUGIN_FACTORY

# Branding

DEFINES += CUSTOMHEADER=\"\\\"CustomPlugin.h\\\"\"
DEFINES += CUSTOMCLASS=CustomPlugin
DEFINES += CUSTOMCORE_PLUGIN=CustomPlugin

exists($$PWD/create_ABLUO) {
    TARGET   = QGroundControlABLUO
    DEFINES += QGC_APPLICATION_NAME='"\\\"QGroundControlABLUO\\\""'
    QGC_APP_NAME        = "QGroundControlABLUO"
    QGC_BINARY_NAME     = "QGroundControlABLUO"
    QGC_APP_DESCRIPTION = "QGroundControl ABLUO"
   
    manifest_copy_target.target = $$PWD/android/AndroidManifest.xml
    manifest_copy_target.commands = \
        $$QMAKE_COPY $$PWD/android/AndroidManifest_ABLUO.xml $$PWD/android/AndroidManifest.xml
    PRE_TARGETDEPS += $$manifest_copy_target.target
    QMAKE_EXTRA_TARGETS += manifest_copy_target
   
    images_copy_target.target = $$PWD/android/res
    images_copy_target.commands = \
        $$QMAKE_COPY_DIR \"$$PWD/android/res_ABLUO\" \"$$PWD/android/res\"
    PRE_TARGETDEPS += $$images_copy_target.target
    QMAKE_EXTRA_TARGETS += images_copy_target

    DEFINES += ABLUO_APP

} else {
    TARGET   = QGroundControlUP
    DEFINES += QGC_APPLICATION_NAME='"\\\"QGroundControlUP\\\""'
    QGC_APP_NAME        = "QGroundControlUP"
    QGC_BINARY_NAME     = "QGroundControlUP"
    QGC_APP_DESCRIPTION = "QGroundControl UP"
    linux {
        manifest_copy_target.target = $$PWD/android/AndroidManifest.xml
        manifest_copy_target.commands = \
            $$QMAKE_COPY \"$$PWD/android/AndroidManifest_UP.xml\" \"$$PWD/android/AndroidManifest.xml\"
        PRE_TARGETDEPS += $$manifest_copy_target.target
        QMAKE_EXTRA_TARGETS += manifest_copy_target
   
        images_copy_target.target = $$PWD/android/res
        images_copy_target.commands = \
            $$QMAKE_COPY_DIR \"$$PWD/android/res_UP\" \"$$PWD/android/res\"
        PRE_TARGETDEPS += $$images_copy_target.target
        QMAKE_EXTRA_TARGETS += images_copy_target
    }
}

DEFINES += QGC_ORG_NAME=\"\\\"qgroundcontrol.org\\\"\"
DEFINES += QGC_ORG_DOMAIN=\"\\\"org.qgroundcontrol\\\"\"


QGC_ORG_NAME        = "UP Caeli Via"
QGC_ORG_DOMAIN      = "www.up-caelivia.it"
QGC_ANDROID_PACKAGE = "org.custom.qgroundcontrol"
QGC_APP_COPYRIGHT   = "Copyright (C) 2020 QGroundControl Development Team. All rights reserved."


# Our own, custom resources
RESOURCES += \
    $$PWD/custom.qrc

exists($$PWD/create_ABLUO)  {
    RESOURCES += $$PWD/abluo.qrc
} else {
    RESOURCES += $$PWD/up.qrc
}

QML_IMPORT_PATH += \
   $$PWD/res

# Our own, custom sources
SOURCES += \
    $$PWD/src/CustomPlugin.cc \
    $$PWD/src/CustomToolbox.cpp \
    $$PWD/src/ParseNTRIP.cpp \
    $$PWD/src/constants.cpp \
    $$PWD/src/CustomAnnouncer.cpp \
    $$PWD/src/VideoStreamControl.cc \
    $$PWD/src/GeoAwareness/KmlPolygonObject.cpp \
    $$PWD/src/GeoAwareness/GeoAwarenessFencePolygon.cpp \
    $$PWD/src/GeoAwareness/KmlPolygonLoader.cpp

HEADERS += \
    $$PWD/src/CustomPlugin.h \
    $$PWD/src/CustomToolbox.h \
    $$PWD/src/ParseNTRIP.h \
    $$PWD/src/constants.h \
    $$PWD/src/CustomAnnouncer.h \
    $$PWD/src/VideoStreamControl.h \
    $$PWD/src/GeoAwareness/KmlPolygonObject.h \
    $$PWD/src/GeoAwareness/GeoAwarenessFencePolygon.h \
    $$PWD/src/GeoAwareness/KmlPolygonLoader.h

INCLUDEPATH += $$PWD/src \
               $$PWD/src/GeoAwareness

# Enable Herelink AirUnit video config
DEFINES += QGC_HERELINK_AIRUNIT_VIDEO