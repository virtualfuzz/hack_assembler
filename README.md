# Hack Assembler

v1.0.0

Compiles hack assembly language to hack machine code.

Made for project 6 of the Nand2Tetris course and also to learn C.

## Usage

[Download the latest release](https://codeberg.org/Jayden295/Hack_Assembler/releases)
and run the hack_assembler file.

Usage: hack_assembler [OPTIONS] [-i file]

Options:
-f --force Allow overwriting file
-i --input Source hack file
-o --output Output hack file
Creates a file named foo.hack that can be executed on the Hack computer.

## Compiling

1. Run `make` or `make release` (does the same thing)
2. Execute hack_assembler from the builds directory

Please note that we compile with dynamic linking by
default which means it needs to be able to find the c-array-list library
and hashmap.c library.

The runpath contains the current path of the program so libarraylist.so and libhashmap.so
needs to in the same directory then hack_assembler.

## License

This project is licensed under the [AGPL-3.0-or-later](LICENSE.md).

Hack Assembler that compiles hack assembly to hack machine code
Copyright (C) 2024 Jayden295

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
