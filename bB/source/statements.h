#include <stdlib.h>
#include <stdio.h>

#include "macros.h"

#define STATEMENT_MAX_PARTS 200
#define STATEMENT_PART_SIZE 200

extern uint8_t bank, bs;

BOOL islabel(char **);
BOOL islabelelse(char **);
void add_includes(char *);
void create_includes(char *);
void incline();
void init_includes();
void invalidate_Areg();
void shiftdata(char **, int);
void compressdata(char **, int, int);
void function(char **);
void endfunction();
void ongoto(char **);
void doreturn(char **);
void doconst(char **);
void dim(char **);
void dofor(char **);
void next(char **);
void gosub(char **);
void doif(char **);
void dolet(char **);
void rem(char **);
void set(char **);
void dogoto(char **);
void pfpixel(char **);
void pfhline(char **);
void pfvline(char **);
void pfscroll(char **);
void player(char **);
void drawscreen(void);
void prerror(char *);
void header_open(FILE *);
void header_write(FILE *, char *);

void doreboot();
int linenum();
void vblank();
void pfclear(char **statement);
void playfieldcolorandheight(char **statement);
void playfield(char **statement);
void lives(char **statement);
uint8_t bbank();
uint8_t bbs();
int getcondpart();
void add_inline(char *myinclude);
void newbank(int bankno);
int bbgetline();
void sdata(char **statement);
void data(char **statement);
void doasm();
void dopop();
void barf_sprite_data();

enum CollisionBit {
  COLL_P0,
  COLL_P1,
  COLL_M0,
  COLL_M1,
  COLL_PF,
  COLL_BALL
};

enum KernelOpts {        //_BV
  KERN_READPADDLE,       //  1
  KERN_PLAYER1COLORS,    //  2
  KERN_PLAYERCOLORS,     //  4
  KERN_NO_BLANK_LINES,   //  8
  KERN_PFCOLORS,         // 16
  KERN_PFHEIGHTS,        // 32
  KERN_BACKGROUNDCHANGE  // 64
};

enum Optimizations {   //_BV
  OPTI_SPEED,          //  1
  OPTI_SIZE,           //  2
  OPTI_NO_INLINE_DATA, //  4
  OPTI_INLINE_RAND     //  8
};
