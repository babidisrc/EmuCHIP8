# EmuCHIP8
Yet another CHIP-8 emulator, now made using Raylib.
This project is a simple and portable CHIP-8 emulator designed to run on Windows, Linux, and macOS.

## Supported Platforms
This emulator supports the main 3 desktop platforms:
* Windows
* Linux
* MacOS

# Windows Users (MinGW-W64)
* Double click the `build-MinGW-W64.bat` file
* CD into the folder in your terminal
* run `make`
* You are good to go

# Linux Users
* CD into the build folder
* run `./premake5 gmake2`
* CD back to the root
* run `make`
* you are good to go

# MacOS Users
* CD into the build folder
* run `./premake5.osx gmake2`
* CD back to the root
* run `make`
* you are good to go

# Output files
The built code will be in the bin dir

# Template License
Copyright (c) 2020-2024 Jeffery Myers

This software is provided "as-is", without any express or implied warranty. In no event 
will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial 
applications, and to alter it and redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim that you 
  wrote the original software. If you use this software in a product, an acknowledgment 
  in the product documentation would be appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not be misrepresented
  as being the original software.

  3. This notice may not be removed or altered from any source distribution.
