#include "keywords.h"
#include "statements.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void keywords(char **cstatement) {
  static int numthens = 0, ors = 0, numelses = 0;
  int i, j, k, colons = 0, currentcolon = 0,
      foundelse = 0;
  char errorcode[100],
       **statement,
       **pass2elstatement,
       **elstatement,
       **orstatement,
       **swapstatement,
       **deallocstatement,
       **deallocorstatement,
       **deallocelstatement;
  BOOL do_or;
  statement = (char**)malloc(sizeof(char*) * STATEMENT_MAX_PARTS);
  orstatement = (char**)malloc(sizeof(char*) * STATEMENT_MAX_PARTS);
  for (i = 0; i < STATEMENT_MAX_PARTS; ++i)
    orstatement[i] = (char*)malloc(sizeof(char) * STATEMENT_PART_SIZE);
  elstatement = (char**)malloc(sizeof(char*) * STATEMENT_MAX_PARTS);
  for (i = 0; i < STATEMENT_MAX_PARTS; ++i)
    elstatement[i] = (char*)malloc(sizeof(char) * STATEMENT_PART_SIZE);

  //for (i = 0; i < 10; ++i) fprintf(stderr, "%s ", cstatement[i]);
  //fprintf(stderr, "\n");
  //deallocstatement = statement;     // for deallocation purposes
  //deallocorstatement = orstatement; // for deallocation purposes
  //deallocelstatement = elstatement; // for deallocation purposes

  // check if there are boolean && or || in an if-then.
  // change && to "then if"
  // change || to two if thens
  // also change operands around to allow <= and >, since
  // currently all we can do is < and >=
  // if we encounter an else, break into two lines, and the first line jumps ahead.
  do_or = false;
  for (k = 0; k <= STATEMENT_MAX_PARTS - 10; ++k) { // reversed loop since last build, need to check for rems first!
    if (MATCH(cstatement[k + 1], "rem")) break; // if statement has a rem, do not process it
    if (MATCH(cstatement[k + 1], "if")) {
      for (i = k + 2; i < STATEMENT_MAX_PARTS; ++i) {
        if (MATCH(cstatement[i], "if")) break;
        if (MATCH(cstatement[i], "else")) foundelse = i;
      }
      if (!strncmp(cstatement[k + 3], ">\0", 2)) {
        // swap operands and switch compare
        strcpy(cstatement[k + 3], cstatement[k + 2]); // stick 1st operand here temporarily
        strcpy(cstatement[k + 2], cstatement[k + 4]);
        strcpy(cstatement[k + 4], cstatement[k + 3]);// get it back
        strcpy(cstatement[k + 3], "<"); // replace compare
      }
      else if (MATCH(cstatement[k + 3], "<=")) {
        // swap operands and switch compare
        strcpy(cstatement[k + 3], cstatement[k + 2]);
        strcpy(cstatement[k + 2], cstatement[k + 4]);
        strcpy(cstatement[k + 4], cstatement[k + 3]);
        strcpy(cstatement[k + 3], ">=");
      }
      if (MATCH(cstatement[k + 3], "&&")) {
        shiftdata(cstatement, k + 3);
        sprintf(cstatement[k + 3], "then%d", ors++);
        strcpy(cstatement[k + 4], "if");
      }
      else if (MATCH(cstatement[k + 5], "&&")) {
        shiftdata(cstatement, k + 5);
        sprintf(cstatement[k + 5], "then%d", ors++);
        strcpy(cstatement[k + 6], "if");
      }
      else if (MATCH(cstatement[k + 3], "||")) {
        if (!strncmp(cstatement[k + 5], ">\0", 2)) {
          // swap operands and switch compare
          strcpy(cstatement[k + 5], cstatement[k + 4]); // stick 1st operand here temporarily
          strcpy(cstatement[k + 4], cstatement[k + 6]);
          strcpy(cstatement[k + 6], cstatement[k + 5]);// get it back
          strcpy(cstatement[k + 5], "<"); // replace compare
        }
        else if (MATCH(cstatement[k + 5], "<=")) {
          // swap operands and switch compare
          strcpy(cstatement[k + 5], cstatement[k + 4]);
          strcpy(cstatement[k + 4], cstatement[k + 6]);
          strcpy(cstatement[k + 6], cstatement[k + 5]);
          strcpy(cstatement[k + 5], ">=");
        }

        for (i = 2; i < STATEMENT_MAX_PARTS - 2 - k; ++i) strcpy(orstatement[i], cstatement[k + i + 2]);
        if (MATCH(cstatement[k + 5], "then"))
          compressdata(cstatement, k + 3, k + 2);
        else if (MATCH(cstatement[k + 7], "then"))
          compressdata(cstatement, k + 3, k + 4);
        strcpy(cstatement[k + 3], "then");
        sprintf(orstatement[0], "%dOR", ors++);
        strcpy(orstatement[1], "if");
        do_or = true;
        // todo: need to skip over the next statement!
      }
      else if (MATCH(cstatement[k + 5], "||")) {
        if (!strncmp(cstatement[k + 7], ">\0", 2)) {
          // swap operands and switch compare
          strcpy(cstatement[k + 7], cstatement[k + 6]); // stick 1st operand here temporarily
          strcpy(cstatement[k + 6], cstatement[k + 8]);
          strcpy(cstatement[k + 8], cstatement[k + 7]);// get it back
          strcpy(cstatement[k + 7], "<"); // replace compare
        }
        else if (MATCH(cstatement[k + 7], "<=")) {
          // swap operands and switch compare
          strcpy(cstatement[k + 7], cstatement[k + 6]);
          strcpy(cstatement[k + 6], cstatement[k + 8]);
          strcpy(cstatement[k + 8], cstatement[k + 7]);
          strcpy(cstatement[k + 7], ">=");
        }
        for (i = 2; i < STATEMENT_MAX_PARTS - 4 - k; ++i) strcpy(orstatement[i], cstatement[k + i + 4]);
        if (MATCH(cstatement[k + 7], "then"))
          compressdata(cstatement, k + 5, k + 2);
        else if (MATCH(cstatement[k + 9], "then"))
          compressdata(cstatement, k + 5, k + 4);
        strcpy(cstatement[k + 5], "then");
        sprintf(orstatement[0], "%dOR", ors++);
        strcpy(orstatement[1], "if");
        do_or = true;
      }
    }
    if (do_or) break;
  }

  if (foundelse) {
    pass2elstatement = do_or ? orstatement : cstatement;
    for (i = 1; i < STATEMENT_MAX_PARTS; ++i) {
      if (MATCH(pass2elstatement[i], "else")) {
        foundelse = i;
        break;
      }
    }

    for (i = foundelse; i < STATEMENT_MAX_PARTS; ++i) strcpy(elstatement[i - foundelse], pass2elstatement[i]);
    if (islabelelse(pass2elstatement)) {
      strcpy(pass2elstatement[foundelse++], ":");
      strcpy(pass2elstatement[foundelse++], "goto");
      sprintf(pass2elstatement[foundelse++], "skipelse%d", numelses);
    }
    for (i = foundelse; i < STATEMENT_MAX_PARTS; ++i) pass2elstatement[i][0] = '\0';
    if (!islabelelse(elstatement)) {
      strcpy(elstatement[2], elstatement[1]);
      strcpy(elstatement[1], "goto");
    }
    if (do_or) {
      for (i = 1; i < STATEMENT_MAX_PARTS; ++i) if (MATCH(cstatement[i], "else")) break;
      for (k = i; k < STATEMENT_MAX_PARTS; ++k) cstatement[k][0] = '\0';
    }
  }

  if (do_or) {
    swapstatement = orstatement; // swap statements because of recursion
    orstatement = cstatement;
    cstatement = swapstatement;
    // this hacks off the conditional statement from the copy of the statement we just created
    // and replaces it with a goto.  This can be improved (i.e., there is no need to copy...)
    if (islabel(orstatement)) {
      // make sure islabel function works right!
      // find end of statement
      i = 3;
      //while (orstatement[i++][0]){} // not sure if this will work...
      while (strncmp(orstatement[i++], "then", 4)){} // not sure if this will work...
      // add goto to it
      if (i > STATEMENT_MAX_PARTS - 10) { i = STATEMENT_MAX_PARTS - 10; fprintf(stderr, "%d: Cannot find end of line - statement may have been truncated\n", linenum()); }
      //strcpy(orstatement[i++], ":");
      strcpy(orstatement[i++], "goto");
      sprintf(orstatement[i++], "condpart%d", getcondpart() + 1); // goto unnamed line number for then statement
      for (; i < STATEMENT_MAX_PARTS; ++i) orstatement[i][0] = '\0'; // clear out rest of statement
    }
    //for (i = 0; i < STATEMENT_MAX_PARTS; ++i) printf("%s ", cstatement[i]);
    //printf("\n");
    //for (i = 0; i < STATEMENT_MAX_PARTS; ++i) printf("%s ", orstatement[i]);
    //printf("\n");

    keywords(orstatement); // recurse
  }

  if (foundelse) {
    swapstatement = elstatement; // swap statements because of recursion
    elstatement = cstatement;
    cstatement = swapstatement;
    keywords(elstatement); // recurse
  }
  for (i = 0; i < STATEMENT_MAX_PARTS; ++i) {
    statement[i] = cstatement[i];
    //printf("%s ", cstatement[i]);
  }
  //printf("\n");
  for (i = 0; i < STATEMENT_MAX_PARTS; ++i) {
    if (CMATCH(i, '\0')) break;
    if (CMATCH(i, ':')) colons++;
  }

  if (SMATCH(0, "then"))
    sprintf(statement[0], "%dthen", numthens++);

  invalidate_Areg();
  while (1) {
    //for (i = 0; i < STATEMENT_MAX_PARTS; ++i) {
    //  for (j = 0; j < 10; ++j) printf("%x ", statement[i][j]);
    //  printf("\n");
    //}
    if (CMATCH(1, '\0')) return;

         if (SMATCH(0, "end"))             endfunction();
    else if (SMATCH(1, "includesfile"))    create_includes(statement[2]);
    else if (SMATCH(1, "include"))         add_includes(statement[2]);
    else if (SMATCH(1, "inline"))          add_inline(statement[2]);
    else if (SMATCH(1, "function"))        function(statement);
    else if (CMATCH(1, ' '))               return;
    else if (SMATCH(1, "if")) {            doif(statement);break;}
    else if (SMATCH(1, "goto"))            dogoto(statement);
    else if (SMATCH(1, "bank"))            newbank(atoi(statement[2]));
    else if (SMATCH(1, "sdata"))           sdata(statement);
    else if (SMATCH(1, "data"))            data(statement);
    else if (SMATCH(1, "on")
          && SMATCH(3, "goto"))            ongoto(statement);
    else if (SMATCH(1, "const"))           doconst(statement);
    else if (SMATCH(1, "dim"))             dim(statement);
    else if (SMATCH(1, "for"))             dofor(statement);
    else if (SMATCH(1, "next"))            next(statement);
    else if (SMATCH(1, "gosub"))           gosub(statement);
    else if (SMATCH(1, "pfpixel"))         pfpixel(statement);
    else if (SMATCH(1, "pfhline"))         pfhline(statement);
    else if (SMATCH(1, "pfclear"))         pfclear(statement);
    else if (SMATCH(1, "pfvline"))         pfvline(statement);
    else if (SMATCH(1, "pfscroll"))        pfscroll(statement);
    else if (SMATCH(1, "drawscreen"))      drawscreen();
    else if (SMATCH(1, "asm"))             doasm();
    else if (SMATCH(1, "pop"))             dopop();
    else if (SMATCH(1, "rem")) {           rem(statement); return; }
    else if (SMATCH(1, "set"))             set(statement);
    else if (SMATCH(1, "return"))          doreturn(statement);
    else if (SMATCH(1, "reboot"))          doreboot();
    else if (SMATCH(1, "vblank"))          vblank();
    else if (IMATCH(1, "pfcolors:")
          || IMATCH(1, "pfheights:"))      playfieldcolorandheight(statement);
    else if (SMATCH(1, "playfield:"))      playfield(statement);
    else if (SMATCH(1, "lives:"))          lives(statement);
    else if (SMATCH(1, "player0:")
          || SMATCH(1, "player1:")
          || SMATCH(1, "player2:")
          || SMATCH(1, "player3:")
          || SMATCH(1, "player4:")
          || SMATCH(1, "player5:")
          || SMATCH(1, "player0color:")
          || SMATCH(1, "player1color:"))   player(statement);
    else if (SMATCH(2, "="))               dolet(statement);
    else if (SMATCH(1, "let"))             dolet(statement);
    else {
      sprintf(errorcode, "Error: Unknown keyword: %s\n", statement[1]);
      prerror(&errorcode[0]);
      exit(1);
    }
    // see if there is a colon
    if (!colons || currentcolon == colons) break;
    currentcolon++;
    i = 0; k = 0;
    while (i != currentcolon)
      if (cstatement[k++][0] == ':') i++;

    for (j = k; j < STATEMENT_MAX_PARTS; ++j) statement[j - k + 1] = cstatement[j];
    for (; (j - k + 1) < STATEMENT_MAX_PARTS; ++j) statement[j - k + 1][0] = '\0';
  }

  if (foundelse)
    printf(".skipelse%d\n", numelses++);

} // keywords
