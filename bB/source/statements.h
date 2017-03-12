#include <stdio.h>

#define _readpaddle 1
#define _player1colors 2
#define _playercolors 4
#define _no_blank_lines 8
#define _pfcolors 16
#define _pfheights 32
#define _background 64

//extern int bank, bs;

int islabel(char **);
int islabelelse(char **);
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
void let(char **);
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
int bbank();
int bbs();
int getcondpart();
void add_inline(char *myinclude);
void newbank(int bankno);
int bbgetline();
void sdata(char **statement);
void data(char **statement);
void doasm();
void dopop();
void barf_sprite_data();
