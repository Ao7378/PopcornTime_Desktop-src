@cd ..
@mkdir build 2>NUL
@cd build

@mkdir qmake 2>NUL
@cd qmake
@mkdir QtAV 2>NUL
@cd QtAV
set VisualStudioVersion=12.0
qmake -spec ../../../Import/qmake-spec/msvc -tp vc -r ../../../Import/QtAV "QMAKE_CXXFLAGS+=/MP" "QMAKE_CXXFLAGS+=/Zi" "DEFINES+=WINVER=0x0601" "DEFINES+=_WIN32_WINNT=0x0601" "DEFINES+=CAPI_IS_LAZY_RESOLVE=0" "QMAKE_LFLAGS+=/DEBUG /OPT:REF /MAP" "CONFIG+=no-examples" "CONFIG+=no-tests"

@if %errorlevel% neq 0 exit /b %errorlevel%

msbuild.exe /m /p:Configuration=Release /v:m || exit 0
@if %errorlevel% neq 0 exit /b %errorlevel%
@cd ..
@cd ..
@mkdir include 2>NUL
@cd include
@mkdir QtAV 2>NUL
@mkdir QtAVWidgets 2>NUL
@cp -fv ../../Import/QtAV/src/QtAV/* QtAV/
@cp -fv ../../Import/QtAV/widgets/QtAVWidgets/* QtAVWidgets/
@cd ..
@cp -fv qmake/QtAV/lib_win_x86/*.dll bin
@cp -fv ../Import/ffmpeg/bin/*.dll bin

@cp -fv qmake/QtAV/lib_win_x86/*.lib lib

exit 0
