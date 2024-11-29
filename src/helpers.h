#include <stdio.h>
#include "compiler/hashmap.h"

void cleanup(FILE *assembly_file, FILE *output_file, char *line,
             char *output_filename, struct hashmap *comp_hashmap,
             struct hashmap *jump_hashmap);
void error(const char *type, const char *message, ...);
void usage(const char *program);