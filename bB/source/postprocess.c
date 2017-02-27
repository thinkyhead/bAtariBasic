#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"

// This reads the includes file created by bB and builds the
// final assembly that will be sent to DASM.
//
// This task used to be done with a batch file/shell script when
// the files in the final asm were static.
// Now, since the files in the final asm can be different, and it doesn't
// make sense to require the user to create a new batch file/shell script.

int main(int argc, char *argv[]) {
  FILE *includesfile, *asmfile;
  //char filename[500];
  char ***readbBfile,
       path[500], prepend[500], line[500], asmline[500];
  int bB = 2, // part of bB file
      writebBfile[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      i, j;
  while ((i = getopt(argc, argv, "i:")) != -1) {
  switch (i) {
    case '?':
      path[0] = '\0';
      break;
    case 'i':
      strcpy(path, optarg);
    }
  }
  char c = path[strlen(path) - 1];
  if (c != '\\' && c != '/') strcat(path, "/");
  strcat(path, "includes/");

  if (!(includesfile = fopen("includes.bB", "r"))) { // open file
    fprintf(stderr,"Cannot open includes.bB for reading\n");
    exit(1);
  }
  readbBfile = (char***)malloc(sizeof(char**) * 17);

  //while (fgets(line, 500, includesfile))
  while (1) {
    if (fscanf(includesfile, "%s", line) == EOF) break;
    if (MATCH(line, "bB") && line[2] != '.') {
      i = (int)(line[2] - '0');
      for (j = 1; j < writebBfile[i]; ++j)
        printf("%s", readbBfile[i][j]);
      continue;
    }
    else {
      strcpy(prepend, path);
      strcat(prepend, line);
      if (!(asmfile = fopen(line, "r"))) { // try file w/o includes path
        if (!(asmfile = fopen(prepend, "r"))) { // open file
          fprintf(stderr, "Cannot open %s for reading\n", line);
          exit(1);
        }
      }
      else if (!MATCH(line, "bB"))
        fprintf(stderr, "User-defined %s found in current directory\n", line);
    }
    while (fgets(asmline, sizeof(asmline), asmfile)) {
      if (!strncmp(asmline, "; bB.asm file is split here", 20)) {
        writebBfile[bB]++;
        readbBfile[bB] = (char**)malloc(sizeof(char*) * 50000);
        readbBfile[bB][writebBfile[bB]] = (char*)malloc(strlen(line) + 2);
        sprintf(readbBfile[bB][writebBfile[bB]], ";%s\n", line);
      }
      if (!writebBfile[bB])
        printf("%s", asmline);
      else {
        readbBfile[bB][++writebBfile[bB]] = (char*)malloc(strlen(asmline) + 2);
        sprintf(readbBfile[bB][writebBfile[bB]], "%s", asmline);
      }
    }
    fclose(asmfile);
    if (writebBfile[bB]) bB++;
    // if (writebBfile) fclose(bBfile);
  }
}
