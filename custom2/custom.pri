message("Adding Custom Plugin")

#-- Version control
#   Major and minor versions are defined here (manually)

CUSTOM_QGC_VER_MAJOR = 0
CUSTOM_QGC_VER_MINOR = 0
CUSTOM_QGC_VER_FIRST_BUILD = 0

# Build number is automatic
# Uses the current branch. This way it works on any branch including build-server's PR branches
CUSTOM_QGC_VER_BUILD = $$system(git --git-dir ../.git rev-list $$GIT_BRANCH --first-parent --count)
win32 {
    CUSTOM_QGC_VER_BUILD = $$system("set /a $$CUSTOM_QGC_VER_BUILD - $$CUSTOM_QGC_VER_FIRST_BUILD")
} else {
    CUSTOM_QGC_VER_BUILD = $$system("echo $(($$CUSTOM_QGC_VER_BUILD - $$CUSTOM_QGC_VER_FIRST_BUILD))")
}
CUSTOM_QGC_VERSION = $${CUSTOM_QGC_VER_MAJOR}.$${CUSTOM_QGC_VER_MINOR}.$${CUSTOM_QGC_VER_BUILD}

DEFINES -= GIT_VERSION=\"\\\"$$GIT_VERSION\\\"\"
DEFINES += GIT_VERSION=\"\\\"$$CUSTOM_QGC_VERSION\\\"\"

message(Custom QGC Version: $${CUSTOM_QGC_VERSION})

# Branding

DEFINES += CUSTOMHEADER=\"\\\"QGCCorePlugin.h\\\"\"
DEFINES += CUSTOMCLASS=QGCCorePlugin

TARGET   = MismartGCS
DEFINES += QGC_APPLICATION_NAME='"\\\"MismartGCS\\\""'

DEFINES += QGC_ORG_NAME=\"\\\"gcs.ai\\\"\"
DEFINES += QGC_ORG_DOMAIN=\"\\\"ai.gcs\\\"\"

QGC_APP_NAME        = "MiSmart GCS"
QGC_BINARY_NAME     = "MiSmartGroundStation"
QGC_ORG_NAME        = "MiSmart"
QGC_ORG_DOMAIN      = "ai.mismart"
QGC_ANDROID_PACKAGE = "ai.mismart.gcs"
QGC_APP_DESCRIPTION = "Mismart Ground Station (Desc)"
QGC_APP_COPYRIGHT   = "Copyright (C) 2020 Mismart Inc. All rights reserved."

# Our own, custom resources
RESOURCES += \
    $$PWD/custom.qrc

QML_IMPORT_PATH += \
   $$PWD/res
