cmake_minimum_required(VERSION 3.2)

include_directories(.)
link_directories(.)

add_executable(tsukasa tsukasa.cpp stdafx.cpp)

if(MSVC)
    set(CMAKE_BUILD_TYPE "Release")
    set(CMAKE_C_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
    target_link_libraries(tsukasa PocoNetmt PocoFoundationmt Ws2_32 Iphlpapi)
elseif(WIN32)
    target_link_libraries(tsukasa PocoNet PocoFoundation Ws2_32 Iphlpapi)
else()
    target_link_libraries(tsukasa PocoNet PocoFoundation)
endif()
