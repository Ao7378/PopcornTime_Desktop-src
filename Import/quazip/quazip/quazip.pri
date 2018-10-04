INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD
HEADERS += \
        $$PWD/include/crypt.h \
        $$PWD/include/ioapi.h \
        $$PWD/include/JlCompress.h \
        $$PWD/include/quaadler32.h \
        $$PWD/include/quachecksum32.h \
        $$PWD/include/quacrc32.h \
        $$PWD/include/quagzipfile.h \
        $$PWD/include/quaziodevice.h \
        $$PWD/include/quazipdir.h \
        $$PWD/include/quazipfile.h \
        $$PWD/include/quazipfileinfo.h \
        $$PWD/include/quazip_global.h \
        $$PWD/include/quazip.h \
        $$PWD/include/quazipnewinfo.h \
        $$PWD/include/unzip.h \
        $$PWD/include/zip.h

SOURCES += $$PWD/qioapi.cpp \
           $$PWD/JlCompress.cpp \
           $$PWD/quaadler32.cpp \
           $$PWD/quacrc32.cpp \
           $$PWD/quagzipfile.cpp \
           $$PWD/quaziodevice.cpp \
           $$PWD/quazip.cpp \
           $$PWD/quazipdir.cpp \
           $$PWD/quazipfile.cpp \
           $$PWD/quazipfileinfo.cpp \
           $$PWD/quazipnewinfo.cpp \
           $$PWD/unzip.c \
           $$PWD/zip.c
