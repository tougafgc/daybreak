# Melty Blood: Daybreak

Daybreak aims at taking the many individual utilites for Melty Blood: AACC and making them
available altogether inside the game's base executable. In addition, an interactive scripting
environment is provided by the newLISP and Squirrel languages for easy extensibility without the need
for recompilation.

[Play Melty Blood!](https://play.meltyblood.club/) | [newLISP](http://www.newlisp.org) | [Squirrel](http://squirrel-lang.org/)

## Installation

Download `daybreak_vX.X.X.zip` and unzip it inside of a pre-existing MBAA folder. `daybreak.exe` and `libwinpthread-1.dll` 
should be in the same folder as `MBAA.exe`, and everything else should be inside the `daybreak/` folder.

## Usage

Double-click `daybreak.exe`, or run it in the command line. 

## Extending Crescent STDLIB

"Crescent" is the name for the collection of Lisp/Squirrel files and functions that are used while an instance
of Melty Blood is running. Currently, the standard library, or the set of FFI functions that are defined in C++
and exported to Lisp/Squirrel, is quite small. `dll/crescent_stdlib.cpp` outlines how functions can be exported to 
Lisp, and how to wrap them in a way that Squirrel scripts will recognize them.

A more in-depth explanation on modifying Daybreak is given in HACKING.md. 

## Roadmap

- Write text inside of Melty (not in ImGui)
- In-game menu editing (vtables, etc)
- Controller mappings
- button test menu
- netplay with GGPO
