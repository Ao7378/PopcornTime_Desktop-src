include(qcefclient.pri)
TARGET = cefclient_process
QT =
CONFIG = 

OBJECTS_DIR = obj
DESTDIR = release

# Input
DEFINES += WIN32 NOMINMAX NDEBUG _WINDOWS UNICODE _UNICODE USING_CEF_SHARED_MSVC

SOURCES += main_process.cpp \
	   mingw-unicode-gui.c 

