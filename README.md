# Introduction

MathLine is a **Math**ematica command **Line** front end to the Mathematica kernel. The front end has feature parity with the textual front end that ships with Mathematica. It uses WSTP, the new name for Wolfram's MathLink as of Mathematica 10. This project subsumes [MathLinkBridge](https://github.com/rljacobson/MathLinkBridge), selecting MathLink or WSTP at build time.

# Usage

`$./MathLine [--option arg ]*`

<div style="width:4cm">Option</div> | Description
----------------------------|---------------------------
`  --mainloop arg (=1)`     |Boolean. Whether or not to use the kernel's Main Loop which keeps track of session history with In[#] and Out[#] variables. Defaults to true.
  `--prompt arg `           |String. The prompt presented to the user for input. When inoutstrings is true, this is typically the empty string.
  `--inoutstrings arg (=1)` |Boolean. Whether or not to print the `In[#]:=` and `Out[#]=` strings. When mainloop is false this option does nothing. Defaults to true.
  `--linkname arg`          |String. The call string to set up the link. The default works on *nix systems on which math is in the path and runnable. Defaults to "math -mathlink".
 ` --linkmode arg`          |String. The MathLink link mode. The default launches a new kernel which is almost certainly what you want. It should be possible, however, to take over an already existing kernel, though this has not been tested. Defaults to "launch".
 ` --usegetline arg (=0)`   |Boolean. If set to false, we use readline-like input with command history and emacs-style editing capability. If set to true, we use a simplified getline input with limited editing capability. Defaults to false.
  `--maxhistory arg (=10)`  |Integer (nonnegative). The maximum number of lines to keep in the input history. Defaults to 10.
  `--help`                  |Produce help message.
  

# Features
* Optionally bypass the Main Loop.
* Configurable prompts.
* Readline-like text input (uses [http://github.com/antirez/linenoise](linenoise)), which means command history and emacs-style editing.
* Automatic collection of postscript code produced by the kernel for the output of graphics, say, using xpdf.
* Open source, BSD licensed.

# Dependencies
Boost (boost::program_options) is used to parse the command line options. This dependency can easily be removed by writing your own command line argument parser.

Those users with cmake on their system can optionally use cmake to build the project instead of trying to determine the magic build incantation themselves, but cmake is not a dependency strictly speaking. 

You will need a functional installation of Mathematica. To build MathLine, your Mathematica installation must include the WSTP or MathLink DeveloperKit. (By default, Mathematica installs both.)

# Building
Math sure that Mathematica is installed on the build system (including the WSTP/MathLink DeveloperKit). If you have CMake on your system, the project includes cmake files that do all of the hard stuff for you. On unix-like systems, you would do something like the following in the root directory of the project:

```
mkdir build
cd build
cmake -Wno-dev ../../MathLine
make
```
The `-Wno-dev` is optional. CMake will select WSTP or MathLink automatically. If all goes well you should have a MathLine binary sitting in the build directory.

If you don't have cmake and want to build it the hard way, first copy the files from `src/WSTP` into `src`. (Adapt these instructions in the obvious way for MathLink.) Then basically build it like any other WSTP C/C++ program: Make sure `wstp.h` (shipped with Mathematica) is available in your include search path and link the binary to Mathematica's WSTP library (`libWSTPi4.a` on my system). Of course the Boost headers should also be available to the compiler.

# Missing Features
For those looking to enhance this code, here are some possibilities. These are features that the textual interface doesn't have but should. Someone should contribute the code.

* Intelligently display output formatted with ToString. (Easy, but I can't figure it out, so hard?)
* The code makes reasonable choices for when to print newline characters. However, this should probably be configurable, say, by printing "preprint" and "postprint" strings around each printed string. (Easy.)
* Implement autocompletion of names and contexts using something similar to this code which is similar to what JMath uses:<br>
	`NamesAndContexts[x_String] := Sort[Join[ Select[Contexts[], StringMatchQ[#, x] &], Names[x]]]`
 (Medium.)
* Implement autocompletion of filenames at, e.g., "<<". (Medium.)
* Implement window size change. (Easy.)

The last few items in the list above are features of [JMath](http://robotics.caltech.edu/~radford/jmath/), another free and open source (GPL) textual front end to Mathematica written by Jim Radford.

# Authors and License

Author(s): Robert Jacobson 

License: BSD license. See the file LICENSE.txt for details.

This software includes linenoise ([http://github.com/antirez/linenoise](http://github.com/antirez/linenoise)), a readline-like library by Salvatore Sanfilippo and Pieter Noordhuis released under the BSD license. See `linenoise.c` or `linenoise.h` for more information.

This software includes FindMathematica ([https://github.com/sakra/FindMathematica](https://github.com/sakra/FindMathematica)), a CMake module by Sascha Kratky that tries to find a Mathematica installation and provides CMake functions for Mathematica's C/C++ interface. FindMathematica is licensed under an MIT license. For details, see the files under `CMake/Mathematica`.