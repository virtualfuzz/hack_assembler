#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void usage(char *program);
void error(char *type, char *message, ...);

void parse_options(int argc, char **argv, bool *force, char **source_filename,
                   char **output_filename);
void open_compiled_file(const char *source_filename, char **output_filename,
                        const bool force, FILE **write_to);
void update_labels(FILE *fp, FILE *output);

enum instruction {
  NONE,
  FINISHED_VALUE,
  FINISHED_INSTRUCTION,
  SLASH_ONE,
  SLASH_TWO,
  LABEL,
  A_INSTRUCTION,
  C_INSTRUCTION
};

const unsigned int MAX_A_VALUE = 32767;