#include "compiler/hashmap.h"
#include "compiler/array-list.h"
#include <stdio.h>

void cleanup(FILE *assembly_file, FILE *output_file, char *line,
             char *output_filename, struct hashmap *comp_hashmap,
             struct hashmap *jump_hashmap, char *a_or_label_value,
             struct hashmap *predefined_hashmap, struct array_list *array,
             struct hashmap *symbol_hashmap);
void error(const char *type, const char *message, ...);
void usage(const char *program);
