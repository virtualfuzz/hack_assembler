#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void parse_options(int argc, char **argv, bool *force, char **source_filename,
                   char **output_filename);
void open_compiled_file(FILE *assembly_file, const char *source_filename,
                        char **output_filename, const bool force,
                        FILE **write_to);
