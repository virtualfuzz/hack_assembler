#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void usage(char *program);
void update_labels(FILE *fp, FILE *output);
void error(char *type, char *message, ...);
void parse_options(int argc, char **argv, bool *force, char **source_filename,
                   char **output_filename);
void open_compiled_file(const char *source_filename, char **output_filename,
                        const bool force, FILE **write_to);
enum instruction {
  NONE,
  FINISHED_VALUE,
  FINISHED_INSTRUCTION,
  SLASH_ONE,
  SLASH_TWO,
  LABEL,
  A_INSTRUCT
};

int main(int argc, char **argv) {
  bool force = false;
  char *source_filename = NULL;
  char *output_filename = NULL;

  parse_options(argc, argv, &force, &source_filename, &output_filename);

  // Try to open source file
  FILE *assembly;
  assembly = fopen(source_filename, "r");
  if (assembly == NULL) {
    error("FILE", "\"%s\" doesn't exist\n", source_filename);
  }

  // Update write_to variable and
  // set it to the pointer of the file where we are going to compile
  FILE *write_to;
  open_compiled_file(source_filename, &output_filename, force, &write_to);

  // First pass: Only update the symbol table
  update_labels(assembly, write_to);

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

void open_compiled_file(const char *source_filename, char **output_filename,
                        const bool force, FILE **write_to) {
  if (output_filename == NULL) {
    // Copy into a new string source filename
    // Reason: We used to output_filename = source_filename
    // but it will modify source too
    size_t length = strlen(source_filename);
    size_t allocation_size = length + 2;

    *output_filename = (char *)malloc(allocation_size);
    if (*output_filename == NULL) {
      error("MEMORY ",
            "Failed to allocate %zu bytes for hack_save_file string\n",
            allocation_size);
    }

    *output_filename = strcpy(*output_filename, source_filename);

    // Remove .asm from the end of the file if it exists
    if (4 < length) {

      // Get last 4 characters of hack_save_file (first arg, asm filename)
      char to_compare[5];
      strncpy(to_compare, *output_filename + length - 4, 4);

      if (strcmp(to_compare, ".asm") == 0) {
        // Remove the last 4 characters (.asm)
        *output_filename[length - 4] = '\0';
      }
    }

    // Add .hack to the filename
    strcat(*output_filename, ".hack");
  }

  // If file already exists and we allow overwriting
  if (access(*output_filename, F_OK) != -1 && force == false) {
    error("FILE ",
          "%s already exists, use the --force flag to overwrite file\n",
          *output_filename);
  }

  // Create compiled code file
  *write_to = fopen(*output_filename, "w");
  if (*write_to == NULL) {
    error("FILE ",
          "Failed to create file %s maybe a directory doesn't exist...",
          *output_filename);
  }
}

// Parse options from cmd line args and update force, source_filename and
// output_filename accordingly
void parse_options(int argc, char **argv, bool *force, char **source_filename,
                   char **output_filename) {
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
      *force = true;
      break;
    case 'i':
      *source_filename = optarg;
      break;
    case 'o':
      *output_filename = optarg;
      break;
    case '?':
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  // Source filename is required
  if (*source_filename == NULL) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }
}

// Take a file as input and add labels to the symbol table if missing
void update_labels(FILE *fp, FILE *output) {
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

      // Check for current character and either add it to value or set the
      // current instruction
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
        if (current_instruct != FINISHED_INSTRUCTION ||
            current_instruct == FINISHED_VALUE) {
          current_instruct = FINISHED_INSTRUCTION;
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
        case FINISHED_INSTRUCTION:
        case NONE:
          current_instruct = SLASH_ONE;
          break;
        default:
          error("SYNTAX ", "Unexpected character (%c) on line %zu\n", line[i],
                current_line);
        }
        break;
      default:
        // Handle the values provided in the instruction
        if (current_instruct == A_INSTRUCT && isdigit(line[i])) {
          strncat(value, &line[i], 1);
        } else if (isspace(line[i])) {
          // This part of the code allows for spacing like @      19 or MD =  0
          if (strcmp(value, "") != 0) {
            switch (current_instruct) {
            case A_INSTRUCT:
              current_instruct = FINISHED_INSTRUCTION;
              break;
            case LABEL:
              current_instruct = FINISHED_VALUE;
              break;
            case SLASH_ONE:
              error("SYNTAX ", "Unexpected character (%c) on line %zu\n",
                    line[i], current_line);
              break;
            default:
              break;
            }
          }
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