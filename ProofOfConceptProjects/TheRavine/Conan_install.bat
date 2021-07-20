@ECHO OFF

REM SFML implementation used std::auto_ptr (which was removed in C++17)
REM so we have to build it as C++14 unline the rest of the project.
REM Luckily, it's ABI-compatible with C++17.

@ECHO ON

conan install . -if Build/TheRavine-x64-Debug/ ^
    --profile=default ^
    --build=outdated ^
    -s build_type=Debug ^
    -s compiler.cppstd=17 ^
    -s sfml:compiler.cppstd=14
    
conan install . -if Build/TheRavine-x64-Release/ ^
    --profile=default ^
    --build=outdated ^
    -s build_type=Release ^
    -s compiler.cppstd=17 ^
    -s sfml:compiler.cppstd=14