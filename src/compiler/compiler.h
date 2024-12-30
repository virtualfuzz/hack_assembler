#include "parser.h"
#include <stdbool.h>
#include <stdio.h>
#include "../helpers.h"

bool compile_to_file(FILE *input, FILE *output);
bool compile_instruction(
    FILE *output_file, const enum instruction instruction_parsed,
    char a_or_label_value[], struct c_instruction_value dest,
    struct c_instruction_value comp, struct c_instruction_value jump,
    struct hashmap *predefined_hashmap, struct hashmap *comp_hashmap,
    struct hashmap *jump_hashmap, unsigned short *instruction_memory_address,
    struct array_list *unhandled_symbols, struct hashmap *symbol_hashmap);
