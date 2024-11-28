#include "helpers.h"
#include <stdarg.h>
#include <stdlib.h>

// Clean up memory/opened files
void cleanup(FILE *assembly_file, FILE *output_file, char *line, char *output_filename) {
  if (assembly_file)
    fclose(assembly_file);

  if (output_file)
    fclose(output_file);

  if (line)
    free(line);

  if (output_filename)
    free(output_filename);
}

// Cleanup memory, print error, and exit
void error(FILE *assembly_file, FILE *output_file, char *line, char *output_filename, const char *type,
           const char *message, ...) {
  fprintf(stderr, "ERROR: %s", type);

  va_list arg;
  va_start(arg, message);
  vfprintf(stderr, message, arg);
  va_end(arg);

  fprintf(stderr, "\n");

  cleanup(assembly_file, output_file, line, output_filename);
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