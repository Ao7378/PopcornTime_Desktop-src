@cd ..
@mkdir build 2>NUL
@cd build

@mkdir cmake 2>NUL
@cd cmake
@mkdir zlib 2>NUL
@cd zlib
cmake -G "Visual Studio 12 2013" ../../../Import/zlib -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../.. -DCMAKE_CXX_FLAGS=/MP
msbuild.exe INSTALL.vcxproj  /m /p:BuildInParallel=true /p:Configuration=Debug /v:m
@cd ..

@cd ..

@mkdir qmake 2>NUL
@cd qmake
@mkdir quazip 2>NUL
@cd quazip
set VisualStudioVersion=12.0
sed -i "s/BUILD_PATH = _g\/$${DRMODE}-$${QT_VERSION}-$${COMPILER_VERSION}-$${TARGET_OS}/BUILD_PATH=..\/..\/..\/build\/qmake\/quazip/g" ../../../Import/quazip/quazip/qtcompilercheck.pri
qmake -spec win32-msvc2013 -tp vc ../../../Import/quazip/quazip "INCLUDEPATH+=..\..\include" "QMAKE_CXXFLAGS+=/MP" "CONFIG+=static" "CONFIG+=staticlib"
msbuild.exe /m /p:BuildInParallel=true /p:Configuration=Debug /v:m
cp -fv bin/quazip.lib ../../lib/quazipd.lib
@cd ..
@cd ..
@mkdir include 2>NUL
@cd include
@mkdir quazip 2>NUL
cp -fv ../../Import/quazip/quazip/include/*.h quazip
@cd ..

@cd ..
@cd ..

cd ..\Import
@C:\Windows\System32\cmd.exe /c build-QtAV-msvc-2013-debug.cmd

exit 0
