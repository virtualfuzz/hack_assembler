#include <stdlib.h>
#include <stdio.h>
#include <string.h>
void usage(char **argv);

int main(int argc, char **argv) {
  // Check if there is only one argument
  if (argc != 2) {
    usage(argv);
    exit(EXIT_FAILURE);
  }

  // Try to open file
  FILE *assembly;
  assembly = fopen(argv[1], "r");
  if (assembly == NULL) {
    usage(argv);
    printf("ERROR: \"%s\" doesn't exist\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  // Close file
  fclose(assembly);


  // Exit successfully
  exit(EXIT_SUCCESS);
}

void usage(char** argv) {
  printf("USAGE: %s foo.asm\n", argv[0]);
  printf("Creates a file named foo.hack that can be "
         "executed on the Hack computer.\n");
}