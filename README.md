# cserver
Another Minecraft Classic server in C. Not well written, but it may be useful for someone. Use this software carefully! The server MAY have many security holes.

The goal of this project: create a stable, customizable and future-rich multiplatform Minecraft Classic server with a minimal dependencies count.

## Features
* Classic Protocol Extension
* Multiplatform (Windows/Linux)
* Plugins support
* WebSocket client support ([More info](https://www.classicube.net/api/docs/server))
* RCON server (Implemented in [base plugin](https://github.com/igor725/cs-base))
* Own world generator (Written by [scaled](https://github.com/scaledteam) for [LuaClassic](https://github.com/igor725/LuaClassic), later ported to C by me)
* Heartbeat API (ClassiCube heartbeat implemented in [base plugin](https://github.com/igor725/cs-base))
* Easy configurable

## Dependencies

### On Linux
1. zlib
2. pthread
3. libcrypto, libcurl (will be loaded on demand)

### On Windows
1. zlib (will be automatically cloned and compiled during the building process)
2. Several std libs (such as Kernel32, DbgHelp, WS2_32)
3. WinInet, Advapi32 (will be loaded on demand)

## Building

### On Linux
``./build [args ...]``

NOTE: This script uses gcc, but you can change it to another compiler by setting CC environment variable (``CC=clang ./build [args ...]``).

### On Windows
``.\build.bat [args ...]``

NOTE: This script must be runned in the Visual Studio Developer Command Prompt.

### Build script arguments
* ``cls`` - Clear console window before compilation;
* ``pb`` - Build a plugin (next argument must be a plugin name, without the "cs-" prefix);
* ``upd`` - Pull server (or plugin if ``pb`` passed AFTER ``upd``) repository before building;
* ``dbg`` - Build with debug symbols;
* ``wall`` - Enable all possible warnings;
* ``wx`` - Treat warnings as errors;
* ``w0`` - Disable all warnings;
* ``od`` - Disable compiler optimizations;
* ``san`` - Add LLVM sanitizers
* ``run`` - Start the server after compilation;
* ``noprompt`` - Suppress zlib download prompt message (Windows only);
* ``runsame`` - Start the server after compilation in the same console window (Windows only);
* ``install`` - Copy plugin binary to the ``plugins`` directory after compilation (Can be used only with ``pb``).

### Example
* ``./build`` - Build the server release binary
* ``./build dbg wall upd`` - Pull latest changes from this repository, then build the server with all warnings and debug symbols
* ``./build dbg wall pb base install`` - Build the base plugin with all warnings and debug symbols, then copy binary to the plugins directory
* ``./build dbg wall upd pb base install`` - Pull latest changes from cs-base repository, then build the base plugin and copy binary to the plugins directory

## Notes
* My main OS - Windows 10. It means the Linux part of the server not well tested.
* By default the server doesn't have any chat commands, build the [cs-base](https://github.com/igor725/cs-base) plugin to add them.
* Here is the [example plugin](https://github.com/igor725/cs-test) for this server software.
* Your directory should have the following structure in order to compile plugins:
```
	[root_folder]/cserver - Main server repository
	[root_folder]/cs-base - Base server functionality
	[root_folder]/cs-survival - Survival plugin
	[root_folder]/cs-test - Test plugin
```
