#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

struct c_instruction_value {
  char value[4];
  bool validity;
};

const unsigned int MAX_A_VALUE = 32767;

void usage(const char *program);
void error(FILE *assembly_file, FILE *output_file, char *line, const char *type,
           const char *message, ...);

void parse_options(int argc, char **argv, bool *force, char **source_filename,
                   char **output_filename);
void open_compiled_file(FILE *assembly_file, const char *source_filename, char **output_filename,
                        const bool force, FILE **write_to);
void cleanup(FILE *assembly_file, FILE *output_file, char *line);

void compile_to_file(FILE *assembly_file, FILE *output_file);
void parse_line(FILE *assembly_file, FILE *output_file, char *line,
                const size_t current_line, enum instruction *instruction_parsed,
                char a_value[], struct c_instruction_value *dest,
                struct c_instruction_value *comp,
                struct c_instruction_value *jump);
void compile_instruction(FILE *assembly_file, FILE *output_file, char *line,
                         const enum instruction instruction_parsed,
                         const char a_value[],
                         const struct c_instruction_value dest,
                         const struct c_instruction_value comp,
                         const struct c_instruction_value jump);