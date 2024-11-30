#include "helpers.h"
#include <stdarg.h>
#include <stdlib.h>

// Clean up memory/opened files
void cleanup(FILE *assembly_file, FILE *output_file, char *line,
             char *output_filename, struct hashmap *comp_hashmap,
             struct hashmap *jump_hashmap, char *a_value, struct hashmap *symbol_hashmap) {
  if (assembly_file != NULL)
    fclose(assembly_file);

  if (output_file != NULL)
    fclose(output_file);

  if (line != NULL)
    free(line);

  if (a_value != NULL)
    free(a_value);

  if (output_filename != NULL)
    free(output_filename);

  if (comp_hashmap != NULL)
    hashmap_free(comp_hashmap);

  if (jump_hashmap != NULL)
    hashmap_free(jump_hashmap);

  if (symbol_hashmap != NULL)
    hashmap_free(symbol_hashmap);
}

// You should run cleanup() to free memory and files
// Print error, and exit
void error(const char *type, const char *message, ...) {
  fprintf(stderr, "ERROR: %s", type);

  va_list arg;
  va_start(arg, message);
  vfprintf(stderr, message, arg);
  va_end(arg);

  fprintf(stderr, "\n");

  exit(EXIT_FAILURE);
}

// Print usage
void usage(const char *program) {
  printf("Usage: %s [OPTIONS] [-i file]\n", program);
  printf("\n");
  printf("Options:\n");
  printf("-f    --force     Allow overwriting file\n");
  printf("-i    --input     Source hack file\n");
  printf("-o    --output    Output hack file\n");
  printf("Creates a file named foo.hack that can be "
         "executed on the Hack computer.\n");
}