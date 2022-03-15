cmake_minimum_required(VERSION 3.10)

#Set the project name
project(SocketProject)
enable_testing()


#C++ 11
set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED True)

#Boost settings
SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)

#Remove to remove debug symbols.
set(CMAKE_BUILD_TYPE Debug)

#Find Boost
find_package(Boost REQUIRED COMPONENTS system thread timer chrono regex)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

SET(USED_LIBS ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_TIMER_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_REGEX_LIBRARY})

add_executable(Server server.cpp src/TCPServer.cpp)

target_link_libraries(Server ${USED_LIBS})

#install(TARGETS Splits DESTINATION bin)
