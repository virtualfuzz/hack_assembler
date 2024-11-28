#include <stdio.h>

void cleanup(FILE *assembly_file, FILE *output_file, char *line, char *output_filename);
void error(FILE *assembly_file, FILE *output_file, char *line, char *output_filename, const char *type,
           const char *message, ...);
void usage(const char *program);