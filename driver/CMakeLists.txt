
add_executable(driver
    src/driver.cpp
    include/solver.hpp
    src/solver.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/resources.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/resources.cpp)
target_include_directories(driver
    PRIVATE
        include
        ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(driver
    PRIVATE
        cxx_std_20)
target_link_libraries(driver
    PRIVATE
        wordle)

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/resources.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/resources.cpp
    COMMAND
        ${CMAKE_COMMAND}
        -DINPUT_FILE=${CMAKE_CURRENT_LIST_DIR}/share/words.txt
        -DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/resources.cpp
        -DOUTPUT_HEADER=${CMAKE_CURRENT_BINARY_DIR}/resources.hpp
        -P ${CMAKE_CURRENT_LIST_DIR}/share/GenerateResources.cmake
    DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/share/words.txt
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_LIST_DIR}/share
    COMMENT
        "Compiling wordlist into C++ array")
