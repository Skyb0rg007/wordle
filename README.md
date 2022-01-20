# A simple Wordle implementation

I hope to use this to create the perfect absurdle implementation

### Building and running

This project requires CMake >= 3.21.3 and a C++20 compiler. Tested with MSVC.

Configure the project

`$ cmake -S . -B _build`

Build the project

`$ cmake --build _build --target driver`

Run the driver (file structure depends on generator, compiler, and platform)

`$ ./_build/driver/driver ./src/driver/share/words.txt`

`$ .\_build\driver\Debug\driver.exe .\src\driver\share\words.txt`

### Installing

This project is not set up for installation.