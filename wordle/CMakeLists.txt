
add_library(wordle
  include/wordle.hpp
  src/wordle.cpp)
target_include_directories(wordle
  PUBLIC
      include)
target_compile_features(wordle
  PUBLIC
    cxx_std_20)
