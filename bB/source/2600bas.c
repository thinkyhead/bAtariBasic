#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "statements.h"
#include "keywords.h"
#include <math.h>

#define BB_VERSION_INFO "bAtariBASIC v1.1 (c)2005-2017\n"

int main(int argc, char *argv[]) {
  char **statement;
  int i, j, k, unnamed = 0;
  char *c, single,
       code[500], displaycode[500];
  FILE *header = NULL;
  int multiplespace = 0;
  char *includes_file = "default.inc",
       *filename = "2600basic_variable_redefs.h",
       *path = NULL,
       def[50][100],
       defr[50][100],
       finalcode[500],
       *codeadd,
       mycode[500];
  int defi = 0;

  // get command line arguments
  while ((i = getopt(argc, argv, "i:r:")) != -1) {
  switch (i) {
    case 'i':
      path = (char*)malloc(500);
      path = optarg;
      break;
    case 'r':
      filename = (char*)malloc(100);
      //strcpy(filename, optarg);
      filename = optarg;
      break;
    case 'v':
        printf("%s", BB_VERSION_INFO);
        exit(0);
    case '?':
      fprintf(stderr, "Usage: %s -r <variable redefs file> -i <includes path>\n", argv[0]);
      exit(1);
    }
  }

  fprintf(stderr, BB_VERSION_INFO);

  printf("game\n"); // label for start of game
  header_open(header);
  init_includes(path);

  statement = (char**)malloc(sizeof(char*) * STATEMENT_MAX_PARTS);
  for (i = 0; i < STATEMENT_MAX_PARTS; ++i)
    statement[i] = (char*)malloc(sizeof(char) * STATEMENT_PART_SIZE);

  while (1) { // clear out statement cache

    for (i = 0; i < STATEMENT_MAX_PARTS; ++i) memset(statement[i], '\0', sizeof(char) * STATEMENT_PART_SIZE);

    c = fgets(code, 500, stdin); // get next line from input
    incline();

    strcpy(displaycode, code);

    // look for defines and remember them
    strcpy(mycode, code);
    for (i = 0; i < 500; ++i) if (code[i] == ' ') break;
    if (code[i + 1] == 'd' &&
        code[i + 2] == 'e' &&
        code[i + 3] == 'f' &&
        code[i + 4] == ' ') {  // found a define
      i += 5;
      for (j = 0; code[i] != ' '; i++)
        def[defi][j++] = code[i]; // get the define
      def[defi][j] = '\0';

      i += 3;

      for (j = 0; code[i] != '\0'; i++)
        defr[defi][j++] = code[i]; // get the definition
      defr[defi][j] = '\0';
      removeCR(defr[defi]);
      printf(";.%s.%s.\n", def[defi], defr[defi]);
      defi++;
    }
    else if (defi) {
      for (i = 0; i < defi; ++i) {
        codeadd = NULL;
        finalcode[0] = '\0';
        // if (codeadd != NULL)
        int defcount = 0;
        while (1) {
          if (defcount++ > 50) {
            fprintf(stderr, "(%d) Infinitely repeating definition or too many instances of a definition\n", bbgetline());
            exit(1);
          }
          codeadd = strstr(mycode, def[i]);
          if (codeadd == NULL) break;
          for (j = 0; j < 500; ++j) finalcode[j] = '\0';
          strncpy(finalcode, mycode, strlen(mycode) - strlen(codeadd));
          strcat(finalcode, defr[i]);
          strcat(finalcode, codeadd + strlen(def[i]));
          strcpy(mycode, finalcode);
        }
      }
    }
    if (strcmp(mycode, code)) strcpy(code, mycode);
    if (!c) break; // end of file

    // preprocessing removed in favor of a simplistic lex-based preprocessor

    i = 0; j = 0; k = 0;

    // look for spaces, reject multiples
    while (code[i]) {
      single = code[i++];
      if (single == ' ') {
        if (!multiplespace) {
          j++;
          k = 0;
        }
        multiplespace++;
      }
      else {
        multiplespace = 0;
        if (k < STATEMENT_PART_SIZE - 1) // [REVENG] avoid overrun when users use REM with long horizontal separators
          statement[j][k++] = single;
      }
    }

    if (j > STATEMENT_PART_SIZE - 10)
      fprintf(stderr, "(%d) Warning: long line\n", bbgetline());

    if (CMATCH(0, '\0'))
      sprintf(statement[0], "L0%d", unnamed++);

    if (!SMATCH(0, "end")) {
      printf(".%s ; %s\n", statement[0], displaycode);
      //printf(".%s ; %s\n", statement[0], code);
    }
    else
      doend();

    //for (i = 0; i < 10; ++i) fprintf(stderr, "%s ", statement[i]);

    keywords(statement);
  }
  bank = bbank();
  bs = bbs();
  //if ((bank == 1) && (bs == 8)) newbank("2\n");
  barf_sprite_data();

  printf(" if ECHOFIRST\n");
  printf("       echo \"    \",[(%s - *)]d , \"bytes of ROM space left", bs == 28 ? "DPC_graphics_end" : "scoretable");
  switch (bs) {
    case 8: printf(" in bank 2"); break;
    case 16: printf(" in bank 4"); break;
    case 28: printf(" in graphics bank"); break;
    case 32: printf(" in bank 8"); break;
    case 64: printf(" in bank 16"); break;
  }
  printf(
    "\")\n"
    " endif \n"
    "ECHOFIRST = 1\n"
    " \n"
    " \n"
    " \n"
  );
  header_write(header, filename);
  create_includes(includes_file);
  fprintf(stderr, "2600 Basic compilation complete.\n");
  //freemem(statement);
  return 0;
}
