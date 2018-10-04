TEMPLATE = app
INCLUDEPATH += . ../cefclient .. ../.. 
VPATH += . ../cefclient

QMAKE_CXXFLAGS += -Wno-unused-parameter -Wl,-pie -municode

LIBS += -L../../libcef_dll/Release -lcef_dll -L.. -lcef

HEADERS += cefclient.h client_app.h client_app_js.h client_binding.h client_handler.h \
           client_renderer.h client_transfer.h message_event.h qcefwebview.h util.h

SOURCES += client_app.cpp client_app_delegates.cpp client_renderer.cpp client_transfer.cpp 
