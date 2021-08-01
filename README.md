# cserver
Another Minecraft Classic server written in C. Very buggy and dirty, but it may be useful for someone.

## Dependencies

### On linux
1. pthread
2. libcrypto
3. curl
4. zlib

### On windows
1. zlib
2. ws2_32
3. wininet
4. advapi32

## Building

### On linux
``./build [args ...]``

NOTE: This script uses gcc, but you can change this to any gcc-like compiler by setting CC environment variable (``CC=clang ./build [args ...]``).

### On Windows
``.\build.bat [args ...]``

NOTE: This script uses Microsoft Visual Studio to compile the project.

### Build script arguments
* ``cls`` - Clear console window before compilation;
* ``pb`` - Build a plugin (next argument must be a plugin name, without the "cs-" prefix);
* ``pbu`` - Pull plugin repository and build it (This argument is used instead of ``pb``);
* ``dbg`` - Build with debug symbols;
* ``wall`` - Enable all possible warnings;
* ``wx`` - Treat warnings as errors;
* ``w0`` - Disable all warnings;
* ``od`` - Disable compiler optimizations;
* ``run`` - Start the server after compilation;
* ``runsame`` - Start the server after compilation in the same console window;
* ``install`` - Copy plugin binary to the ``plugins`` directory after compilation (Can be used only with ``pb``).

### Notes
* My main OS - Windows 10. It means the Linux part of the server not well tested.
* By default the server doesn't have any chat commands, build the cs-base plugin to add them.
* Your directory should have the following structure in order to compile plugins:
```
  [root_folder]/cserver - Main server repository
  [root_folder]/cs-base - Base server functionality
  [root_folder]/cs-survival - Survival plugin
  [root_folder]/cs-test - Test plugin
```
