# A simple Wordle implementation

I hope to use this to create the perfect absurdle implementation.

Note: This implementation uses a compact State representation.

This is important for state-searching since only 31 bytes are used for each state,
and only 5 bytes for each word. This greatly reduces memory usage, and is faster than
maintaining and merging a list of previous results.

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