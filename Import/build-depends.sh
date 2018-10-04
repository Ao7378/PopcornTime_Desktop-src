cd ..

if [ ! -f build/lib/libz.a ]; then
  echo Building zlib...
  mkdir -p build/cmake/zlib && cd build/cmake/zlib
  /Applications/CMake.app/Contents/bin/cmake ../../../Import/zlib -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../.. -DBUILD_SHARED_LIBRARIES=OFF || exit 1
  make -j -l2 install || exit 1
  cd ../../../
  echo Done zlib.
fi

if [ ! -f build/lib/libquazip.a ]; then
  echo Building quazip...
  mkdir -p build/qmake/quazip && cd build/qmake/quazip
  sed -i '' -e "s&BUILD_PATH = _g/\$\${DRMODE}-\$\${QT_VERSION}-\$\${COMPILER_VERSION}-\$\${TARGET_OS}&BUILD_PATH=\.\./\.\./\.\./build/qmake/quazip&g" ../../../Import/quazip/quazip/qtcompilercheck.pri
  qmake ../../../Import/quazip/quazip "INCLUDEPATH+=..\..\include" "CONFIG+=static" "CONFIG+=staticlib" || exit 1
  make -j -l2 install || exit 1
  cp -fv bin/*.a ../../lib
  cd ../..
  mkdir -p include && cd include
  mkdir -p quazip
  cp -fv ../../Import/quazip/quazip/include/*.h quazip
  cd ../..
  echo Done quazip.
fi

echo Building QtAV...
pwd
mkdir -p build/qmake/QtAV && cd build/qmake/QtAV
pwd
qmake -spec ../../../Import/qmake-spec/clang -r ../../../Import/QtAV "CONFIG+=no-examples" "CONFIG+=no-tests" "CONFIG+=static_ffmpeg" "CONFIG+=static"  || exit 1
make -j -l2 || make -j -l2 || make -j -l2 || exit 2
cd ../../
mkdir -p include && cd include
mkdir -p QtAV
mkdir -p QtAVWidgets
cp -fv ../../Import/QtAV/src/QtAV/* QtAV/
cp -fv ../../Import/QtAV/widgets/QtAVWidgets/* QtAVWidgets/
cd ..
cp -fvR qmake/QtAV/lib_osx_x86_64_llvm/ lib
cd ..
echo Done QtAV.

exit 0

