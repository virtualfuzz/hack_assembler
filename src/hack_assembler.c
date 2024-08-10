#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage(char *program);
void update_labels(FILE *fp);
void error(char *type, char* message, ...);

int main(int argc, char **argv) {
  // Check if there is only one argument
  if (argc != 2) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  // Try to open file
  FILE *assembly;
  assembly = fopen(argv[1], "r");
  if (assembly == NULL) {
    error("", "\"%s\" doesn't exist\n", argv[1]);
  }

  // First pass: Only update the symbol table
  update_labels(assembly);

  // Close file
  fclose(assembly);

  // Exit successfully
  exit(EXIT_SUCCESS);
}

void error(char *type, char *message, ...) {
  fprintf(stderr, "ERROR: %s", type);

  va_list arg;
  va_start(arg, message);
  vfprintf(stderr, message, arg);
  va_end(arg);

  exit(EXIT_FAILURE);
}

// Print usage
void usage(char *program) {
  printf("USAGE: %s foo.asm\n", program);
  printf("Creates a file named foo.hack that can be "
         "executed on the Hack computer.\n");
}

// Take a file as input and add labels to the symbol table if missing
void update_labels(FILE *fp) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  size_t current_line = 1;

  // Loop though each line
  while ((read = getline(&line, &len, fp)) != -1) {

    // Loop though each character in line
    size_t line_len = strlen(line);
    int new_label = 0;
    int slash = 0;
    char label_name[line_len];
    strcpy(label_name, "");

    for (size_t i = 0; i < line_len; i++) {
      if (new_label == 1 && line[i] != ')') {
        strncat(label_name, &line[i], 1);
      }

      switch (line[i]) {
      case '(':
        if (new_label != 1 && strlen(label_name) == 0) {
          new_label = 1;
        } else {
          fclose(fp);
          free(line);
          error("SYNTAX ", "Unexpected ( at line %zu\n", current_line);
        }
        break;
      case ')':
        if (new_label != 0) {
          new_label = 0;
        } else {
          fclose(fp);
          free(line);
          error("SYNTAX ", "Unexpected ) at line %zu\n", current_line);
        }
        break;
      case '/':
        slash++;
        break;
      }

      if (slash == 2) {
        break;
      }
    }

    if (strlen(label_name)) {
      printf("%s\n", label_name);
    }
    
    current_line++;
  }

  free(line);
}