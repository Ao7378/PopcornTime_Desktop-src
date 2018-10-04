include(qcefclient.pri)
TARGET = qcefclient
QT += widgets

#DEFINES += CEF_ENABLE_SANDBOX=1
#LIBS += -L../../Release -lcef_sandbox

HEADERS += mainwindow.h
FORMS += mainwindow.ui
SOURCES += main.cpp mainwindow.cpp

SOURCES += cefclient.cpp \
	   cefclient_qt.cpp \
           client_binding.cpp \
           client_handler.cpp \
           client_handler_qt.cpp \
           message_event.cpp \
           qcefwebview.cpp \
           init_wrapper.cpp

