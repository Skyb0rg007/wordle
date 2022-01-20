
add_executable(driver
  src/driver.cpp)
target_compile_features(driver
  PRIVATE
    cxx_std_20)
target_link_libraries(driver
  PRIVATE
    wordle)