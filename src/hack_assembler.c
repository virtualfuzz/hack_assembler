#include "hack_assembler.h"
#include "compiler/compiler.h"
#include "helpers.h"

// This file implements the "frontend" of the compiler
// Parses the command line arguments and then starts the proper functions
int main(int argc, char **argv) {
  // Parse options
  bool force = false;
  char *source_filename = NULL;
  char *output_filename = NULL;

  parse_options(argc, argv, &force, &source_filename, &output_filename);

  // Try to open source file
  FILE *assembly_file = fopen(source_filename, "r");
  if (assembly_file == NULL)
    error("FILE", "\"%s\" doesn't exist\n", source_filename);

  // Open the compiled file
  FILE *output_file;
  open_compiled_file(assembly_file, source_filename, &output_filename, force,
                     &output_file);

  // Finally start compiling
  compile_to_file(assembly_file, output_file);

  cleanup(assembly_file, output_file, NULL, NULL, NULL, NULL, NULL, NULL);
  exit(EXIT_SUCCESS);
}

// Opens/Creates compiled file on the write_to variable
// Update output_filename with the actual filename of the file
void open_compiled_file(FILE *assembly_file, const char *source_filename,
                        char **output_filename, const bool force,
                        FILE **write_to) {
  if (*output_filename == NULL) {
    // Copy into a new string source filename
    // Reason: We used to output_filename = source_filename
    // but it will modify source too
    size_t length = strlen(source_filename);
    size_t allocation_size = length + 2;

    *output_filename = (char *)malloc(allocation_size);
    if (*output_filename == NULL) {
      cleanup(assembly_file, NULL, NULL, *output_filename, NULL, NULL, NULL,
              NULL);
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
      to_compare[4] = '\0';

      if (strcmp(to_compare, ".asm") == 0) {
        // Remove the last 4 characters (.asm)
        (*output_filename)[length - 4] = '\0';
      }
    }

    // Add .hack to the filename
    strcat(*output_filename, ".hack");
  }

  // If file already exists and we allow overwriting
  if (access(*output_filename, F_OK) != -1 && force == false) {
    cleanup(assembly_file, NULL, NULL, *output_filename, NULL, NULL, NULL,
            NULL);
    error("FILE ",
          "%s already exists, use the --force flag to overwrite file\n",
          *output_filename);
  }

  // Create compiled code file
  *write_to = fopen(*output_filename, "w");
  if (*write_to == NULL) {
    cleanup(assembly_file, *write_to, NULL, *output_filename, NULL, NULL, NULL,
            NULL);
    error("FILE ",
          "Failed to create file %s maybe a directory doesn't exist...",
          *output_filename);
  }
}

// Parse options from cmd line args
// Set force, source_filename and output_filename accordingly
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
