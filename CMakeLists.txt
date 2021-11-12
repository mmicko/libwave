cmake_minimum_required(VERSION 3.6.0)
project(libwave LANGUAGES CXX C)

include_directories(. ./liblzma/)

add_executable(vcd2fst vcd2fst.c ./fst/lz4.c ./fst/lz4.h ./fst/fastlz.c ./fst/fastlz.h ./fst/fstapi.c ./fst/fstapi.h ./jrb/jrb.h ./jrb/jrb.c)
target_link_libraries(vcd2fst z)

add_executable(fst2vcd fst2vcd.c ./fst/lz4.c ./fst/lz4.h ./fst/fastlz.c ./fst/fastlz.h ./fst/fstapi.c ./fst/fstapi.h)
target_link_libraries(fst2vcd z)

add_executable(fstminer fstminer.c ./fst/lz4.c ./fst/lz4.h ./fst/fastlz.c ./fst/fastlz.h ./fst/fstapi.c ./fst/fstapi.h)
target_link_libraries(fstminer z)

add_executable(vcd2lxt vcd2lxt.c lxt_write.c lxt_write.h v2l_analyzer.h v2l_debug.c v2l_debug.h)
target_link_libraries(vcd2lxt z bz2)

add_executable(lxt2vcd lxt2_read.c lxt2_read.h lxt2vcd.c scopenav.c)
target_link_libraries(lxt2vcd z)

add_executable(vcd2lxt2 vcd2lxt2.c lxt2_write.c lxt2_write.h v2l_analyzer_lxt2.h v2l_debug_lxt2.c v2l_debug_lxt2.h)
target_link_libraries(vcd2lxt2 z)

add_executable(vzt2vcd vzt_read.c vzt_read.h vzt2vcd.c scopenav.c ./liblzma/LzmaLib.c ./liblzma/LzmaLib.h)
target_link_libraries(vzt2vcd z bz2 pthread)

add_executable(vcd2vzt vcd2vzt.c vzt_write.c vzt_write.h v2l_analyzer_lxt2.h v2l_debug_lxt2.c v2l_debug_lxt2.h ./liblzma/LzmaLib.c ./liblzma/LzmaLib.h)
target_link_libraries(vcd2vzt z bz2)

add_executable(vztminer vztminer.c vzt_read.c vzt_read.h ./liblzma/LzmaLib.c ./liblzma/LzmaLib.h)
target_link_libraries(vztminer z bz2 pthread)

add_executable(lxt2miner lxt2miner.c lxt2_read.c lxt2_read.h)
target_link_libraries(lxt2miner z)

add_executable(evcd2vcd evcd2vcd.c ./jrb/jrb.h ./jrb/jrb.c)
