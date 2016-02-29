cmake_minimum_required(VERSION 2.6)
PROJECT(SimpleBump)
SUBDIRS(source)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_FLAGS "-fpermissive")
