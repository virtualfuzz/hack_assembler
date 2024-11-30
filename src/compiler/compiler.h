#include "hashmap.h"
#include "parser.h"
#include <stdbool.h>
#include <stdio.h>

void compile_to_file(FILE *input, FILE *output);
void compile_instruction(FILE *assembly_file, FILE *output_file, char *line,
                         const enum instruction instruction_parsed,
                         char a_or_dest_value[],
                         const struct c_instruction_value dest,
                         const struct c_instruction_value comp,
                         const struct c_instruction_value jump,
                         struct hashmap *symbol_hashmap,
                         struct hashmap *comp_hashmap,
                         struct hashmap *jump_hashmap,
                         size_t *instruction_memory_address);
void a_instruction_to_binary(FILE *stream, unsigned short n);