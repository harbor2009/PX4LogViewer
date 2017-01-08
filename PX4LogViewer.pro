QT += qml quick charts
!no_desktop: QT += widgets

CONFIG += c++11

SOURCES += src/main.cpp \
    src/logparser.cpp

RESOURCES += \
    res.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    src/logparser.h \
    src/sdlog2_format.h \
    src/sdlog2_messages.h

DISTFILES += \
    qml/colors.js
