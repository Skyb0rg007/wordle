cmake_minimum_required(VERSION 3.21.3 FATAL_ERROR)
project(wordle
  VERSION 0.1.0
  DESCRIPTION "A wordle and absurdle implementation"
  HOMEPAGE_URL "www.github.com/Skyb0rg007/wordle"
  LANGUAGES CXX)

include(CTest)

add_subdirectory(wordle)
add_subdirectory(driver)