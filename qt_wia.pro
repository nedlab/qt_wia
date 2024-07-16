DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050F00
CODECFORSRC     = UTF-8
CODECFORTR      = UTF-8


HEADERS += src/wia_demo.h

SOURCES += src/wia_demo.cpp \
    src/main.cpp

FORMS += ui/wia_demo.ui


UI_DIR = .ui
MOC_DIR = .moc
OBJECTS_DIR = .obj

win32 {
 QMAKE_CXXFLAGS += -GR /MP
}

QT += axcontainer printsupport
