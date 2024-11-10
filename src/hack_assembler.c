#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void usage(char *program);
void update_labels(FILE *fp);
void error(char *type, char *message, ...);
enum instruction { NONE, LABEL, SLASH_ONE, SLASH_TWO, A_INSTRUCT };

int main(int argc, char **argv) {
  // Options
  bool force = false;
  char *source_filename = NULL;
  char *output_filename = NULL;

  // Options that our compiler takes
  struct option long_options[] = {{"force", no_argument, 0, 'f'},
                                  {"input", required_argument, 0, 'i'},
                                  {"output", required_argument, 0, 'o'},
                                  {0, 0, 0, 0}};

  // Get options
  int opt;
  while ((opt = getopt_long(argc, argv, "fi:o:", long_options, NULL)) != -1) {
    switch (opt) {
    case 'f':
      force = 1;
      break;
    case 'i':
      source_filename = optarg;
      break;
    case 'o':
      output_filename = optarg;
      break;
    case '?':
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  // Source filename is required
  if (source_filename == NULL) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  // Try to open source file
  FILE *assembly;
  assembly = fopen(source_filename, "r");
  if (assembly == NULL) {
    error("FILE", "\"%s\" doesn't exist\n", source_filename);
  }

  // Name of the hack save file
  char *hack_save_file;

  // If we gave the name of the hack file set it as that
  // Else create the file name using the filename of source file
  if (output_filename) {
    hack_save_file = output_filename;
  } else {
    // Copy into a new string source filename
    // Reason: We used to set hack_save_file to source but it will modify source
    // too
    size_t length = strlen(source_filename);
    size_t allocation_size = length + 2;

    hack_save_file = (char *)malloc(allocation_size); // FIXME: Free me
    if (hack_save_file == NULL) {
      error("MEMORY ",
            "Failed to allocate %zu bytes for hack_save_file string\n",
            allocation_size);
    }

    hack_save_file = strcpy(hack_save_file, source_filename);

    // Remove .asm from the end of the file if it exists
    if (4 < length) {

      // Get last 4 characters of hack_save_file (first arg, asm filename)
      char to_compare[5];
      strncpy(to_compare, hack_save_file + length - 4, 4);

      if (strcmp(to_compare, ".asm") == 0) {
        // Remove the last 4 characters (.asm)
        hack_save_file[length - 4] = '\0';
      }
    }

    // Add .hack to the filename
    strcat(hack_save_file, ".hack");
  }

  // Check if file exists to prevent overwriting
  // TODO: but what if the file is auto created?
  if (access(hack_save_file, F_OK) != -1) {
    if (force == true) {
      printf("empty file or some shit like that\n");
      printf("we didnt finish this shit\n");
      exit(EXIT_FAILURE);
    } else {
      error("FILE ", "%s already exists, use the --force flag to overwrite file\n", hack_save_file);
    }
  }

  // Create compiled code file
  FILE *write_to;
  write_to = fopen(hack_save_file, "w");
  if (write_to == NULL) {
    error("FILE ",
          "Failed to create file %s maybe a directory doesn't exist...",
          hack_save_file);
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
  printf("USAGE: FIXME Usage: %s [-f] [-i input] [-o output]\n", program);
  printf("USAGE: %s foo.asm foo.hack\n", program);
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

    size_t line_len = strlen(line);
    enum instruction current_instruct = NONE;
    char value[line_len];
    strcpy(value, "");

    // Loop though each character in line
    for (size_t i = 0; i < line_len; i++) {
      // TODO: Rework on that
      // Copy character if we are currently in a label
      // set instruction type to like label or a instruct
      // do something depending on that
      if (current_instruct == LABEL && line[i] != ')') {
        strncat(value, &line[i], 1);
      }

      // TODO: Document that
      switch (line[i]) {
      case '@':
        if (current_instruct == NONE) {
          current_instruct = A_INSTRUCT;
          printf("a instruction\n");
        } else {
          error("SYNTAX ", "Unexpected @ at line %zu\n", current_line);
        }
        break;
      case '(':
        if (current_instruct == NONE && strlen(value) == 0) {
          current_instruct = LABEL;
        } else {
          fclose(fp);
          free(line);
          error("SYNTAX ", "Unexpected ( at line %zu\n", current_line);
        }
        break;
      case ')':
        if (current_instruct != NONE) {
          current_instruct = NONE;
        } else {
          fclose(fp);
          free(line);
          error("SYNTAX ", "Unexpected ) at line %zu\n", current_line);
        }
        break;
      case '/':
        switch (current_instruct) {
        case SLASH_ONE:
          current_instruct = SLASH_TWO;
          break;
        case NONE:
          current_instruct = SLASH_ONE;
          break;
        default:
          error("SYNTAX ", "WAHT THE FUKC\n", current_line);
        }
        break;
      default:
        // TODO: case thing
        // Ignore whitespace
        if (isspace(line[i])) {
        } else if (current_instruct == A_INSTRUCT && isdigit(line[i])) {
          strncat(value, &line[i], 1);
        } else {
          error("SYNTAX ", "Unexepected character (%c) on line %zu\n", line[i],
                current_line);
        }
      }

      if (current_instruct == SLASH_TWO) {
        break;
      }
    }

    // TODO: In here compile the instruction

    if (strlen(value)) {
      // FIXME: We also need to know on what address we are
      // How about we consider everything that isn't a // or space as a command
      printf("%s\n", value);
    }

    current_line++;
  }

  free(line);
}