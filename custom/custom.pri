message("Adding Custom Plugin")

#-- Version control
# Build number is automatic
# Get the latest tag matching the pattern v*-UP
GIT_TAG = $$system(git describe --tags --abbrev=0 --match "v*-UP")

# Get short commit hash
GIT_HASH = $$system(git rev-parse --short HEAD)

# Count number of commits since the latest tag
COMMITS_FROM_TAG = $$system(git rev-list $${GIT_TAG}..HEAD --count)

# Check if current commit is exactly the tag commit
isOnTag = $$system(git rev-parse $${GIT_TAG}) == $$system(git rev-parse HEAD)

# Compose version string
!isEmpty(isOnTag) {
    # We're on the tag itself — version is the tag name
    CUSTOM_QGC_VERSION = $${GIT_TAG}
} else {
    # Not on the tag — include commit count and hash
    CUSTOM_QGC_VERSION = $${GIT_TAG}-$${COMMITS_FROM_TAG}-$${GIT_HASH}
}

ANDROID_MIN_SDK_VERSION = 21

DEFINES -= APP_VERSION_STR=\"\\\"$$APP_VERSION_STR\\\"\"
DEFINES += APP_VERSION_STR=\"\\\"$$CUSTOM_QGC_VERSION\\\"\"

message(Custom QGC Version: $${CUSTOM_QGC_VERSION})

# Build a single flight stack by disabling APM support
# CONFIG  += QGC_DISABLE_APM_MAVLINK
# CONFIG  += QGC_DISABLE_APM_PLUGIN QGC_DISABLE_APM_PLUGIN_FACTORY

# We implement our own PX4 plugin factory
# CONFIG  += QGC_DISABLE_PX4_PLUGIN_FACTORY

# Branding

DEFINES += CUSTOMHEADER=\"\\\"CustomPlugin.h\\\"\"
DEFINES += CUSTOMCLASS=CustomPlugin
DEFINES += CUSTOMCORE_PLUGIN=CustomPlugin

TARGET   = QGroundControlUP
DEFINES += QGC_APPLICATION_NAME='"\\\"QGroundControlUP\\\""'

DEFINES += QGC_ORG_NAME=\"\\\"qgroundcontrol.org\\\"\"
DEFINES += QGC_ORG_DOMAIN=\"\\\"org.qgroundcontrol\\\"\"


QGC_APP_NAME        = "QGroundControlUP"
QGC_BINARY_NAME     = "QGroundControlUP"
QGC_ORG_NAME        = "Custom"
QGC_ORG_DOMAIN      = "org.custom"
QGC_ANDROID_PACKAGE = "org.custom.qgroundcontrol"
QGC_APP_DESCRIPTION = "Custom QGroundControl"
QGC_APP_COPYRIGHT   = "Copyright (C) 2020 QGroundControl Development Team. All rights reserved."

exclude($$PWD/../QGCPostLinkInstaller.pri)
include($$PWD/QGCPostLinkInstaller.pri)

# Our own, custom resources
RESOURCES += \
    $$PWD/custom.qrc

QML_IMPORT_PATH += \
   $$PWD/res

# Our own, custom sources
SOURCES += \
    $$PWD/src/CustomPlugin.cc \
    $$PWD/src/CustomToolbox.cpp \
    $$PWD/src/ParseNTRIP.cpp \
    $$PWD/src/constants.cpp \
    $$PWD/src/CustomAnnouncer.cpp

HEADERS += \
    $$PWD/src/CustomPlugin.h \
    $$PWD/src/CustomToolbox.h \
    $$PWD/src/ParseNTRIP.h \
    $$PWD/src/constants.h \
    $$PWD/src/CustomAnnouncer.h

INCLUDEPATH += \
    $$PWD/src \
