# Introduction

MathLine is a **Math**ematica command **Line** front end to the Mathematica kernel. The front end has feature parity with the textual front end that ships with Mathematica but provides a stable getline or readline-like interface that won't break between Mathematica releases. 

# Features
* Optionally bypass the Main Loop.
* Configurable prompts.
* Readline-like text input (uses [linenoise](http://github.com/antirez/linenoise)), which means command history and emacs-style editing. A more primitive `std:getline` interface is also available via a command line option.
* Automatic collection of postscript code produced by the kernel for the output of graphics, say, using xpdf.
* Open source, BSD licensed.

# Usage

## Preparing your environment
MathLine needs access to a Mathematica kernel (Wolfram kernel). By default, MathLine assumes that a kernel can be started on a command line with: 

`$ math`

If that does nothing, the easiest thing to do is create a bash script named `math` in `/usr/bin` (if there isn't one already). **Do not create a symlink instead.** Put the following in the file `/usr/bin/math` and make it executable:

```
#!/bin/bash

/path/to/WolframKernel $@
```
The kernel might be called `MathKernel` or something else on different systems. On my system (macOS Sierra, Mathematica v11.01), the path to the kernel is:
 
`/Applications/Mathematica.app/Contents/MacOS/WolframKernel`

If you want MathLine to start a kernel different from the default ("`math -wstp`"), use the `--linkname` option:

`$ mathline --linkname "/path/to/kernel -wstp"`

### WSTP or MathLink?

WSTP stands for Wolfram Symbolic Transfer Protocol. In Mathematica v10 Wolfram changed the name of MathLink to WSTP. *WSTP is identical to MathLink except for the name.* Change all occurrences of "wstp" to "mathlink" in the above if you are trying to use a kernel older than v10. Newer versions of Mathematica continue to ship both MathLink and WSTP to maintain backwards compatibility. MathLine selects the appropriate one at compile time.   

## Using MathLine

MathLine will use reasonable defaults if invoked without arguments.   

`$ mathline [--option arg ]*`

If you get, 
```
Could not connect to Mathematica. Check that "math -wstp" works from a command line.
```
then read the section "Preparing your environment" above.

Option| Description
----------------------------|---------------------------
`  --mainloop arg (=1)`     |Boolean. Whether or not to use the kernel's Main Loop which keeps track of session history with `In[#]` and `Out[#]` variables. Defaults to true.
  `--prompt arg `           |String. The prompt presented to the user for input. When inoutstrings is true, this is typically the empty string.
  `--inoutstrings arg (=1)` |Boolean. Whether or not to print the `In[#]:=` and `Out[#]=` strings. When mainloop is false this option does nothing. Defaults to true.
  `--linkname arg`          |String. The call string to set up the link. The default works on *nix systems on which math is in the path and runnable. Defaults to `"math -wstp"`.
 ` --linkmode arg`          |String. The WSTP/MathLink link mode. The default launches a new kernel which is almost certainly what you want. It should be possible, however, to take over an already existing kernel, though this has not been tested. Defaults to "launch".
 ` --usegetline arg (=0)`   |Boolean. If set to false, we use readline-like input with command history and emacs-style editing capability. If set to true, we use a simplified getline input with limited editing capability. Try setting this to true if MathLine connects to a kernel but doesn't give a correct prompt. Defaults to false.
  `--maxhistory arg (=10)`  |Integer (nonnegative). The maximum number of lines to keep in the input history. Defaults to 10.
  `--help`                  |Produce help message.
  

## Dependencies
CMake is used to locate the WSTP/MathLink header and library and is the recommended way to build MathLine. Those users without cmake on their system will have to either use the included Python script to generate a make file or determine the magic build incantation themselves. 

You will need a functional installation of Mathematica. To build MathLine, your Mathematica installation must include the WSTP or MathLink DeveloperKit. (By default, Mathematica installs both.) Of course the Mathematica kernel (Wolfram kernel) is necessary at runtime for MathLine to be of any use. 

# Building
Math sure that Mathematica is installed on the build system (including the WSTP/MathLink DeveloperKit). 

## Building with CMake
If you have CMake on your system, the project includes CMake files that do all of the hard stuff for you. On most systems just execute the following in the root directory of the project:

```
mkdir build
cd build
cmake ../
make
```
CMake will select WSTP or MathLink automatically. If all goes well you should have a MathLine binary sitting in the build directory. To install it: 

```make install```

## Building with GenMakefile.py
This is not supported or recommended. If you don't have CMake, there is an included Python script that will generate an appropriate Makefile for you, automatically selecting WSTP or MathLink depending on your Mathematica version. This script assumes that Mathematica is installed on your computer with a command line interface that runs when you give the `math` command at the terminal. (See the section "Preparing your environment" above.) To use the script, do the following:

```
mkdir build
./GenMakefile.py -runMake
```
If the build is successful, the MathLine binary will be in the build directory. If you omit the `-runMake`, the script will output the Makefile without running `make`.

##Building manually
You can also build it the hard way. (Adapt these instructions in the obvious way for MathLink.) Build it like any other WSTP C/C++ program: Make sure `wstp.h` (shipped with Mathematica) is available in your include search path and link the binary to Mathematica's WSTP library (`libWSTPi4.a` on my system).

# Missing Features
For those looking to enhance this code, here are some possibilities. These are features that the textual interface doesn't have but should. Someone should contribute the code.

* Implement scripting, i.e., `mathline --script file.m` executes the code in file.m and exits. (Easy.)
* Intelligently display output formatted with ToString. (Easy, but I can't figure it out, so hard?)
* The code makes reasonable choices for when to print newline characters. However, this should probably be configurable, say, by printing "preprint" and "postprint" strings around each printed string. (Easy.)
* Implement autocompletion of names and contexts using something similar to this code which is similar to what JMath uses:<br>
	`NamesAndContexts[x_String] := Sort[Join[ Select[Contexts[], StringMatchQ[#, x] &], Names[x]]]`
 (Medium.)
* Implement autocompletion of filenames at, e.g., "<<". (Medium.)
* Implement window size change. (Easy.)

The last few items in the list above are features of [JMath](http://robotics.caltech.edu/~radford/jmath/), another free and open source (GPL) textual front end to Mathematica written by Jim Radford.

# Authors and License

Author(s): Robert Jacobson \<rljacobson gmail\>

License: BSD license. See the file LICENSE.txt for details. Note that this license applies ONLY to the MathLine source code. At compile time MathLine links against the MathLink/WSTP library which use and distribution is governed by a separate non free license. It is your responsibility to ensure your use of the linked MathLine binary is in compliance with the MathLink/WSTP license.   

This software includes linenoise ([http://github.com/antirez/linenoise](http://github.com/antirez/linenoise)), a readline-like library by Salvatore Sanfilippo and Pieter Noordhuis released under the BSD license. See LICENSE-linenoise.txt for more information.

This software includes FindMathematica ([https://github.com/sakra/FindMathematica](https://github.com/sakra/FindMathematica)), a CMake module by Sascha Kratky that tries to find a Mathematica installation and provides CMake functions for Mathematica's C/C++ interface. FindMathematica is licensed under the MIT license. See LICENSE-FindMathematica.txt for more information.

This software includes popl ([https://github.com/badaix/popl](https://github.com/badaix/popl)), a c++ command line arguments parser by Johannes Pohl licensed under the MIT license. See LICENSE-popl.txt for more information.
