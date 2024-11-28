#include <stdio.h>
#include <stdbool.h>
#include "parser.h"

void compile_to_file(FILE *input, FILE *output);
void compile_instruction(FILE *assembly_file, FILE *output_file, char *line,
                         const enum instruction instruction_parsed,
                         const char a_value[],
                         const struct c_instruction_value dest,
                         const struct c_instruction_value comp,
                         const struct c_instruction_value jump);
void a_instruction_to_binary(FILE *stream, unsigned short n);