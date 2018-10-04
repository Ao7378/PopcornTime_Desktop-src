@rem set BOOST_ROOT=
set CXXFLAGS=-I%BOOST_ROOT%
set CFLAGS=-I%BOOST_ROOT%
@rem C:\DEV\boost_1_55_0
bjam.exe define=TORRENT_NO_ASSERTS=1 define=TORRENT_DISABLE_INVARIANT_CHECKS=1 -j7 debug toolset=msvc  %1 %2 %3 | tee build.log
bjam.exe define=TORRENT_NO_ASSERTS=1 define=TORRENT_DISABLE_INVARIANT_CHECKS=1 -j7 release %1 %2 %3 | tee build.log
@rem define=TORRENT_ASIO_DEBUGGING=1
@rem define=TORRENT_NO_DEPRECATE=1 
@rem define=TORRENT_DISK_STATS=1