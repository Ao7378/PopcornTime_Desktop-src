#export CXXFLAGS=-I/opt/local/boost/boost
#export BOOST_BUILD_PATH=bin/macos/release/link-static
#export BOOST_ROOT=/opt/local/boost

b2 define=TORRENT_NO_ASSERTS=1 define=TORRENT_DISABLE_INVARIANT_CHECKS=1 -j7 debug   | tee buildd.log
b2 define=TORRENT_NO_ASSERTS=1 define=TORRENT_DISABLE_INVARIANT_CHECKS=1 -j7 release | tee buildr.log
#@rem define=TORRENT_NO_DEPRECATE=1
#@rem define=TORRENT_DISK_STATS=1export LDFLAGS=-L/opt/local/lib

