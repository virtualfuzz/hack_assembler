# Hack Assembler

v1.0.1

Compiles hack assembly language to hack machine code.

Made for project 6 of the Nand2Tetris course and also to learn C.

## Usage

[Download the latest release](https://codeberg.org/virtualfuzz/Hack_Assembler/releases)
and run the hack_assembler file.

Usage: hack_assembler [OPTIONS] [-i file]

Options:\
-f --force Allow overwriting file\
-i --input Source hack file\
-o --output Output hack file\
Creates a file named foo.hack that can be executed on the Hack computer.

## Compiling

1. Run `make` or `make release` (does the same thing)
2. Execute `hack_assembler` from the builds directory

Please note that we compile with dynamic linking by
default which means it needs to be able to find the c-array-list library
and hashmap.c library.

The runpath contains the current path of the program so libarraylist.so and libhashmap.so
needs to in the same directory then hack_assembler.

## How does this work?

First off, I want to say that I simplified the explanation a bit on some
places, and that it would probably be easier to understand this explanation if
you took the [Nand2Tetris part 1 course](https://nand2tetris.org/), or at least
[got an explanation about the Hack assembly language](https://youtube.com/watch?v=bGxkS4f5i9s).

This uses a similar architecture then the one suggested in the
[Nand2Tetris course](https://www.nand2tetris.org/), it works in
two stages.

### First stage

The first stage already starts to compile the file, as most of the
time, most of the file can already be compiled without caring
about [forward declerations](https://en.wikipedia.org/wiki/Forward_declaration).

Forward declerations happen in the Hack language when you use a label while
it is not declared yet (but later in file).

Labels are simply written inside of the symbols_hashmap.

C instructions are always compiled entirely as they do not use labels or memory addresses.

A instructions on the other hand are compiled entirely if they are simply a number, otherise,
they may be a label or variable.\
We try to query them from the predefined_hashmap and the symbols_hashmap and use the number
found as the number at the A instruction.\
Now if they are still not found, they are probably later declared in the file or are a variable,
we add them to the unhandled_symbols to handle it at the second stage.

### Second stage

Now that the first stage is done, we can start working on the unhandled_symbols.

A second stage is required to be sure that we found every (forward) declaration of labels,
which means that anything that is not a label and is unknown must be a variable.

We loop though every unhandled_symbols and if they are found inside of symbols_hashmap, we
put the number found inside of symbols_hashmap. Otherise, we add a new variable with the same name
to the symbols_hashmap and we compile it.

Once all of that is done, the file is compiled entirely!

### Architecture

In here I will explain the most important parts of the assembler.

[src/](src/) contains every file that simply does the "API/Frontend" of the program and anything
that isn't really compiler related.\
[src/compiler/](src/compiler/) contains every file that implements the actual compiling.

[src/compiler/compiler.c](src/compiler/compiler.c) contains `compile_to_file` (implements stage 1/2)
and `compile_instruction` (compiles one instruction/one line for stage 1).\
[src/compiler/parser.c](src/compiler/parser.c) contains `parse_line` parses one line and updates the variables accordingly.

## License

This project is licensed under the [AGPL-3.0-or-later](LICENSE.md).

Hack Assembler that compiles hack assembly to hack machine code
Copyright (C) 2024 virtualfuzz

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.

### Third-Party projects

[LICENSE-3RD-PARTY.md](LICENSE-3RD-PARTY.md)

This project uses hashmap.c
hashmap.c and all its files inside of the hashmap.c/ and src/compiler/hashmap.h
are licensed under the MIT license.

This project uses c-array-list
c-array-list and all its files inside of c-array-list/ and src/compiler/array-list.h
are licensed under the GPL-3.0-or-later.
