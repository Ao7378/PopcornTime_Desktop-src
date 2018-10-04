######################################################################
# CONFIGURATION VARIABLES
win32:EXE_NAME = PopcornTimeDesktop
mac:EXE_NAME = PopcornTime
VERSION = 5.7.3.0
QMAKE_TARGET_BUNDLE_PREFIX = se.popcorn-time.
QMAKE_INFO_PLIST = PPInfo.plist
mac:CONFIG += USE_WEBENGINE
msvc:contains(QT_VERSION, ^5\\.[5-9]\\..*):CONFIG += USE_WEBENGINE
!USE_WEBENGINE {
win32:CONFIG += USE_QCEF
mac:VERSION = 4.9.2
}
#DEFINES += NO_TORRENT_IO_WRAPPER
#DEFINES += USE_SOCKS5
######################################################################

CONFIG += debug_and_release
mac: CONFIG += QTAV_STATIC

QT += opengl

TEMPLATE = app
TARGET = $$EXE_NAME
QT += widgets   

msvc: DEFINES += APP_VERSION=\"$${VERSION}\"
!msvc:DEFINES += APP_VERSION=\"\\\"$${VERSION}\"\\\"

USE_QCEF {
    message("Q5CEF3.")
    QT += network
    DEFINES += USE_QCEF CEF_EVENTS_TO_QAPPLICATION
    SOURCES += client_app.cpp client_app_delegates.cpp client_renderer.cpp client_transfer.cpp 
    HEADERS += cefclient.h client_app.h client_app_js.h client_binding.h client_handler.h \
           client_renderer.h client_transfer.h message_event.h qcefwebview.h util.h
    SOURCES += cefclient.cpp cefclient_qt.cpp client_binding.cpp client_handler.cpp client_handler_qt.cpp message_event.cpp qcefwebview.cpp init_wrapper.cpp
    INCLUDEPATH += ../Import/qcef/q5cef ../Import/qcef/q5cef/cefclient ../Import/qcef/
    VPATH += ../Import/qcef/q5cef ../Import/qcef/q5cef/cefclient
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter 
    win32: QMAKE_CXXFLAGS += -Wl,-pie -municode
    win32:LIBS += -L../Import/qcef/libcef_dll/release -lcef_dll -L../Import/qcef/q5cef -lcef
    RESOURCES += qcef.qrc
} 
USE_WEBENGINE {
    message("QWebEngine.")
    QT += webengine webenginewidgets quick webchannel websockets
    DEFINES += USE_WEBENGINE
    VPATH += $$[QT_INSTALL_EXAMPLES]/webchannel/shared
    INCLUDEPATH += $$[QT_INSTALL_EXAMPLES]/webchannel/shared
    SOURCES += websockettransport.cpp websocketclientwrapper.cpp
    HEADERS += websockettransport.h websocketclientwrapper.h
    RESOURCES += qwebengine.qrc
}
!USE_WEBENGINE:!USE_QCEF {
    message("QWebKit.")
    QT += webkitwidgets
    SOURCES += GraphicsBrowser.cpp
    HEADERS += GraphicsBrowser.h
}

DEFINES += QUAZIP_STATIC

win32:!msvc: QMAKE_CXXFLAGS += -std=c++11
CONFIG += c++11
msvc: QMAKE_CXXFLAGS += /MP
#mac: QMAKE_CXXFLAGS += -std=c++11 -Wc++11-extensions
msvc: QMAKE_CXXFLAGS += /Zi
mac: QMAKE_LFLAGS += -g


INCLUDEPATH += . ../Import/root/include

#Resources
RESOURCES += resources.qrc
QMAKE_RESOURCE_FLAGS += -compress 2
win32:RC_ICONS = res/popcorntime.ico
mac:  ICON     = res/popcorntime.icns

INCLUDEPATH += ../Import/vpnintegration
VPATH += ../Import/vpnintegration
HEADERS       += vpnIntegration.h 
SOURCES       += vpnIntegration.cpp
OBJECTIVE_SOURCES += osxutil.mm

SOURCES += FramelessMainWindow.cpp SeekSlider.cpp SubComboBox.cpp SubtitleDecoder.cpp TrackComboBox.cpp ControlRevealer.cpp SubtitleFetcher.cpp Subtitler.cpp \
           ResizeDragEventHandler.cpp VideoPlayer.cpp hostApp.cpp VideoControl.cpp commontypes.cpp TorrentWrapper.cpp cfgdownloader.cpp dxtn.cpp \
           ChromeCast.cpp GlyphButton.cpp NjsProcess.cpp TorHelpers.cpp Zoom4k.cpp widgets.cpp DropHandler.cpp TorrentIODevice.cpp \
           main.cpp SystemHelper.cpp


OBJECTIVE_SOURCES += osxutil.mm

USE_WEBENGINE:SOURCES += WebEngineWebView.cpp
USE_WEBENGINE:HEADERS += WebEngineWebView.h

HEADERS += FramelessMainWindow.h SeekSlider.h SubComboBox.h SubtitleDecoder.h TrackComboBox.h ControlRevealer.h SubtitleFetcher.h Subtitler.h \
           ResizeDragEventHandler.h VideoPlayer.h hostApp.h VideoControl.h commontypes.h TorrentWrapper.h cfgdownloader.h dxtn.h \
           ChromeCast.h GlyphButton.h NjsProcess.h TorHelpers.h Zoom4k.h widgets.h DropHandler.h TorrentIODevice.h \
           SystemHelper.h
           
win32:  HEADERS += lib.h
!win32: HEADERS += lib.h

win32:             LIBS += -llib
mac:               LIBS += -lstreamerok -ltorrent  -framework Cocoa -framework IOKit -liconv  \
                         $$(BOOST_ROOT)/stage/lib/libboost_filesystem.a $$(BOOST_ROOT)/stage/lib/libboost_thread.a $$(BOOST_ROOT)/stage/lib/libboost_system.a
#                          -L/opt/local/boost/stage/lib -lboost_thread-xgcc42-mt-s-1_55 -lboost_date_time-xgcc42-mt-s-1_55 \
#                          -lboost_filesystem-xgcc42-mt-s-1_55 -lboost_system-xgcc42-mt-s-1_55

win32:Release:     DESTDIR = releaseW
win32:debug:       DESTDIR = debugW
mac:debug:         DESTDIR = debugM
mac:Release:       DESTDIR = releaseM
DEPEND_DIR = ../build

win32:Release:LIBS += -lQtAV1 -lQtAVWidgets1
win32:Debug:LIBS += -lQtAVd1 -lQtAVWidgetsd1
QTAV_STATIC {
mac:Release:LIBS += -lQtAV -lQtAVWidgets -L../Import/ffmpeg-macOS/lib -lavcodec -lavdevice -lavfilter -lavformat -lavresample -lavutil -lswresample -lswscale \
                    -llzma -lbz2 \
                    -framework AudioToolbox -framework CoreMedia -framework VideoToolbox -framework CoreVideo -framework OpenGL -framework IOSurface -framework AVFoundation \
                    -framework VideoDecodeAcceleration -framework QuartzCore -framework Security
mac:Debug:LIBS += -lQtAVd -lQtAVWidgetsd
} else {
mac:Release:LIBS += -framework QtAV -framework QtAVWidgets
mac:Debug:LIBS += -framework QtAVd -framework QtAVWidgetsd
}

win32:msvc:LIBS += -liconv
INCLUDEPATH += $$DEPEND_DIR/include
LIBS        += -L$$DEPEND_DIR/lib
macx:  LIBS += -F$$DEPEND_DIR/lib

LIBS += -L. -L../Import/root -L../build
Release:msvc:LIBS += -llib/zlibstatic
Debug:msvc:LIBS += -llib/zlibstaticd
macx:LIBS += -lz

QT_BIN_DIR = $$dirname(QMAKE_QMAKE)

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.obj
RCC_DIR = $$DESTDIR/.obj
UI_DIR = $$DESTDIR/.obj

mac: QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
mac: QMAKE_POST_LINK=strip $$DESTDIR/$${EXE_NAME}.app/Contents/MacOS/$${EXE_NAME};

mac:Release{
    system(rm -Rf $${DESTDIR}/$${EXE_NAME}.app/Contents/Frameworks)
}

macx {
    INFO_PLIST_PATH = $$shell_quote($${DESTDIR}/$${EXE_NAME}.app/Contents/Info.plist)
    QMAKE_POST_LINK += /usr/libexec/PlistBuddy -c \"Set :CFBundleShortVersionString $${VERSION}\" $${INFO_PLIST_PATH};
    QMAKE_POST_LINK += /usr/libexec/PlistBuddy -c \"Set :CFBundleVersion $${VERSION}\" $${INFO_PLIST_PATH};
}
