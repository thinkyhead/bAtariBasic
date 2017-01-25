#include "statements.h"
#include "keywords.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#define a(ASM) "\t" ASM "\n"
#define ap(ASM, ...) asm_printf(a(ASM), ##__VA_ARGS__)

// Shared extern vars
uint8_t bank = 1, // current bank: 1 or 2 for 8k
        bs = 0; // bankswitching type; 0 = none

// Private
static uint16_t condpart = 0,
                sprite_index = 0,
                playfield_index[50] = { 0 },
                playfield_number = 0,
                numfors = 0,
                numfixpoint44 = 0,
                numfixpoint88 = 0,
                numredefvars = 0,
                numconstants = 0,
                numjsrs = 0,
                // numgosubs = 0,
                line = 0,
                branchtargetnumber = 0,
                pfdata[50][256],
                pfcolorindexsave,
                pfcolornumber;

static uint8_t  last_bank = 0, // last bank when bs is enabled (0 for 2k/4k)
                optimization = 0,
                kernel_options = 0;

static BOOL     ROMpf = false,
                superchip = false,
                multisprite = false,
                lifekernel = false,
                smartbranching = false,
                doingfunction = false,
                includesfile_already_done = false;

static char includespath[500],
            user_includes[1000],
            sprite_data[5000][50],
            forvar[50][50],
            forlabel[50][50],
            forstep[50][50],
            forend[50][50],
            fixpoint44[2][50][50],
            fixpoint88[2][50][50],
            redefined_variables[100][100],
            constants[50][100],
            Areg[50];

void loadindex(char *);
void jsr(char *);
BOOL findlabel(char **, int i);
void callfunction(char **);
void mul(char **, int);
void divd(char **, int);
void remove_trailing_commas(char *);
void removeCR(char *);
int number(unsigned char);
int getindex(char *mystatement, char *myindex);
int findpoint(char *item);

void bmi(char *);
void bpl(char *);
void bne(char *);
void beq(char *);
void bcc(char *);
void bcs(char *);
void bvc(char *);
void bvs(char *);

BOOL isimmed(char *value) {
  // search queue of constants
  // removeCR(value);

  // a constant should be treated as an immediate
  for (int i = 0; i < numconstants; ++i)
    if (!strcmp(value, constants[i])) return 1;

  return value[0] == '$' || value[0] == '%' || value[0] <= '9';
}

char* immedtok(char *value) { return isimmed(value) ? "#" : ""; }

char* indexpart(char *mystatement, int myindex) {
  static char outstr[50];
  if (!myindex)
    sprintf(outstr, "%s%s", immedtok(mystatement), mystatement);
  else
    sprintf(outstr, "%s,x", mystatement); // indexed with x!
  return outstr;
}

void asm_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

void doreboot() {
  ap("JMP ($FFFC)");
}

int linenum() {
  // returns current line number in source
  return line;
}

void vblank() {
  // code that will be run in the vblank area
  // subject to limitations!
  // must exit with a return [thisbank if bankswitching used]
  printf("vblank_bB_code\n");
}

void pfclear(char **statement) {
  char getindex0[STATEMENT_PART_SIZE];
  int index = 0;

  invalidate_Areg();

  if (CMATCH(2, '\0') || CMATCH(2, ':'))
    ap("LDA #0");
  else {
    index = getindex(statement[2], &getindex0[0]);
    if (index) loadindex(&getindex0[0]);
    ap("LDA %s", indexpart(statement[2], index));
  }

  removeCR(statement[1]);
  jsr(statement[1]);
}

void playfieldcolorandheight(char **statement) {
  char data[100], rewritedata[100];
  int i = 0, j = 0,
      pfpos = 0, pfoffset = 0, indexsave;
  // PF colors and/or heights
  // PFheights use offset of 21-31
  // PFcolors use offset of 84-124
  // if used together: playfieldblocksize-88, playfieldcolor-87
  if (!strncasecmp(statement[1], "pfheights:", 9)) { // doesn't look for the colon
    if ((kernel_options & (_BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS))) == _BV(KERN_PFHEIGHTS)) {
      sprintf(sprite_data[sprite_index++], " if (<*) > 223\n");
      sprintf(sprite_data[sprite_index++], "   repeat (277-<*)\n" a(".byte 0"));
      sprintf(sprite_data[sprite_index++], "   repend\n endif\n");
      sprintf(sprite_data[sprite_index], "pfcolorlabel%d\n", sprite_index);
      indexsave = sprite_index;
      sprite_index++;
      while (1) {
        if ((!fgets(data, sizeof(data), stdin) || (data[0] <= '9' && data[0] >= '0')) && data[0] != 'e') {
          prerror("Error: Missing \"end\" keyword at end of pfheight declaration\n");
          exit(1);
        }
        line++;
        if (MATCH(data, "end")) break;
        removeCR(data);
        if (!pfpos) {
          ap("LDA #%s", data);
          ap("STA playfieldpos");
        }
        else sprintf(sprite_data[sprite_index++], a(".byte %s"), data);
        pfpos++;
      }
      ap("LDA #>(pfcolorlabel%d-21)", indexsave);
      ap("STA pfcolortable+1");
      ap("LDA #<(pfcolorlabel%d-21)", indexsave);
      ap("STA pfcolortable");
    }
    else if ((kernel_options & (_BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS))) != (_BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS))) {
      prerror("PFheights kernel option not set");
      exit(1);
    }
    else { // pf color and heights enabled
      sprintf(sprite_data[sprite_index++], " if (<*) > 206\n");
      sprintf(sprite_data[sprite_index++], "   align 256\n endif\n");
      sprintf(sprite_data[sprite_index++], " if (<*) < 88\n");
      sprintf(sprite_data[sprite_index++], "   repeat (88-(<*))\n" a(".byte 0"));
      sprintf(sprite_data[sprite_index++], "   repend\n endif\n");
      sprintf(sprite_data[sprite_index++], "playfieldcolorandheight\n");

      pfcolorindexsave = sprite_index;
      while (1) {
        if ((!fgets(data, sizeof(data), stdin) || (data[0] <= '9' && data[0] >= '0')) && data[0] != 'e') {
          prerror("Error: Missing \"end\" keyword at end of pfheight declaration\n");
          exit(1);
        }
        line++;
        if (MATCH(data, "end")) break;
        removeCR(data);
        if (!pfpos) {
          ap("LDA #%s", data);
          ap("STA playfieldpos");
        }
        else
          sprintf(sprite_data[sprite_index++], a(".byte %s,0,0,0"), data);
        pfpos++;
      }
    }
  }
  else { // has to be pfcolors
    if ((kernel_options & (_BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS))) == _BV(KERN_PFCOLORS)) {
      if (!pfcolornumber) {
        sprintf(sprite_data[sprite_index++], " if (<*) > 206\n");
        sprintf(sprite_data[sprite_index++], "   align 256\n endif\n");
        sprintf(sprite_data[sprite_index++], " if (<*) < 88\n");
        sprintf(sprite_data[sprite_index++], "   repeat (88-(<*))\n" a(".byte 0"));
        sprintf(sprite_data[sprite_index++], "   repend\n endif\n");
        sprintf(sprite_data[sprite_index], "pfcolorlabel%d\n", sprite_index);
        sprite_index++;
      }
      //indexsave = sprite_index;
      pfoffset = 1;
      while (1) {
        if ((!fgets(data, sizeof(data), stdin) || (data[0] <= '9' && data[0] >= '0')) && data[0] != 'e') {
          prerror("Error: Missing \"end\" keyword at end of pfcolor declaration\n");
          exit(1);
        }
        line++;
        if (MATCH(data, "end")) break;
        removeCR(data);
        if (!pfpos) {
          ap("LDA #%s", data);
          ap("STA COLUPF");
          if (!pfcolornumber) pfcolorindexsave = sprite_index - 1;
          pfcolornumber = (pfcolornumber + 1) % 4;
          pfpos++;
        }
        else {
          if (pfcolornumber != 1) // add to existing table
          //if ((pfcolornumber % 3) != 1) // add to existing table (possible correction?)
          {
            j = 0; i = 0;
            while (j != (pfcolornumber + 3) % 4) {
              if (sprite_data[pfcolorindexsave + pfoffset][i++] == ',') j++;
              if (i > 99) {
                prerror("Warning: size of subsequent pfcolor tables should match\n");
                break;
              }
            }
            //fprintf(stderr, "%s\n", sprite_data[pfcolorindexsave + pfoffset]);
            strcpy(rewritedata, sprite_data[pfcolorindexsave + pfoffset]);
            rewritedata[i - 1] = '\0';
            if (i < 100) sprintf(sprite_data[pfcolorindexsave + pfoffset++], "%s,%s%s", rewritedata, data, rewritedata + i + 1);
          }
          else { // new table
            sprintf(sprite_data[sprite_index++], a(".byte %s,0,0,0"), data);
            // pad with zeros - later we can fill this with additional color data if defined
          }
        }
      }
      ap("LDA #>(pfcolorlabel%d-%d)", pfcolorindexsave, 85 - pfcolornumber - ((((pfcolornumber << 1) | (pfcolornumber << 2)) ^ 4) & 4));
      ap("STA pfcolortable+1");
      ap("LDA #<(pfcolorlabel%d-%d)", pfcolorindexsave, 85 - pfcolornumber - ((((pfcolornumber << 1) | (pfcolornumber << 2)) ^ 4) & 4));
      //ap("lda #<(pfcolorlabel%d-%d)", pfcolorindexsave, 85 - pfcolornumber);
      ap("STA pfcolortable");
    }
    else if ((kernel_options & (_BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS))) != (_BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS))) {
      prerror("PFcolors kernel option not set");
      exit(1);
    }
    else {  // pf color fixed table
      while (1) {
        if ((!fgets(data, sizeof(data), stdin) || (data[0] <= '9' && data[0] >= '0')) && data[0] != 'e') {
          prerror("Error: Missing \"end\" keyword at end of pfcolor declaration\n");
          exit(1);
        }
        line++;
        if (MATCH(data, "end")) break;
        removeCR(data);

        if (!pfpos) {
          ap("LDA #%s", data);
          ap("STA COLUPF");
        }
        else {
          j = 0; i = 0;
          while (!j) {
            if (sprite_data[pfcolorindexsave + pfoffset][i++] == ',') j++;
            if (i > 99) {
              prerror("Warning: size of subsequent pfcolor tables should match\n");
              break;
            }
          }
          //fprintf(stderr, "%s\n", sprite_data[pfcolorindexsave + pfoffset]);
          strcpy(rewritedata, sprite_data[pfcolorindexsave + pfoffset]);
          rewritedata[i - 1] = '\0';
          if (i < 100) sprintf(sprite_data[pfcolorindexsave + pfoffset++], "%s,%s%s", rewritedata, data, rewritedata + i + 1);
        }
        pfpos++;
      }
    }
  }
}

void playfield(char **statement) {
  int i, j, k, l = 0;
  // for specifying a ROM playfield
  char zero = '.', one = 'X',
       data[100], pframdata[50][100];
  if ((unsigned char)statement[2][0] > ' ') zero = statement[2][0];
  if ((unsigned char)statement[3][0] > ' ') one = statement[3][0];

  // read data until we get an end
  // stored in global var and output at end of code

  while (1) {
    if ((!fgets(data, sizeof(data), stdin) || (data[0] != zero && data[0] != one)) && data[0] != 'e') {
      prerror("Error: Missing \"end\" keyword at end of playfield declaration\n");
      exit(1);
    }
    line++;
    if (MATCH(data, "end")) break;
    if (ROMpf) { // if playfield is in ROM:
      pfdata[playfield_number][playfield_index[playfield_number]] = 0;
      for (i = 0; i < 32; ++i) {
        if (data[i] != zero && data[i] != one) break;
        pfdata[playfield_number][playfield_index[playfield_number]] <<= 1;
        if (data[i] == one) pfdata[playfield_number][playfield_index[playfield_number]] |= 1;
      }
      playfield_index[playfield_number]++;
    }
    else
      strcpy(pframdata[l++], data);
  }

  //}

  if (ROMpf) { // if playfield is in ROM:
    ap("LDA #<PF1_data%d", playfield_number);
    ap("STA PF1pointer");
    ap("LDA #>PF1_data%d", playfield_number);
    ap("STA PF1pointer+1");

    ap("LDA #<PF2_data%d", playfield_number);
    ap("STA PF2pointer");
    ap("LDA #>PF2_data%d", playfield_number);
    ap("STA PF2pointer+1");
    playfield_number++;
  }
  else { // RAM pf, as in std_kernel
    printf("  ifconst pfres\n");
    ap("LDX #4*pfres-1");
    printf("  else\n");
    ap("LDX #47");
    printf("  endif\n");
    ap("JMP pflabel%d", playfield_number);

    // no need to align to page boundaries

    printf("PF_data%d\n", playfield_number);
    for (j = 0; j < l; ++j) { // stored right side up
      printf("\t.byte %%");
      // the below should be changed to check for zero instead of defaulting to it
      for (k = 0; k < 8; ++k)    printf(pframdata[j][k] == one ? "1" : "0");
      printf(", %%");
      for (k = 15; k >= 8; k--)  printf(pframdata[j][k] == one ? "1" : "0");
      printf(", %%");
      for (k = 16; k < 24; ++k)  printf(pframdata[j][k] == one ? "1" : "0");
      printf(", %%");
      for (k = 31; k >= 24; k--) printf(pframdata[j][k] == one ? "1" : "0");
      printf("\n");
    }

    printf("pflabel%d\n", playfield_number);
    ap("LDA PF_data%d,x", playfield_number);
    if (superchip) {
      //printf("  ifconst pfres\n");
      //ap("STA playfield+48-pfres*4-128,x");
      //printf("  else\n");
      ap("STA playfield-128,x");
      //printf("  endif\n");
    }
    else {
      //printf("  ifconst pfres\n");
      //ap("STA playfield+48-pfres*4,x");
      //printf("  else\n");
      ap("STA playfield,x");
      //printf("  endif\n");
    }
    ap("DEX");
    ap("BPL pflabel%d", playfield_number);
    playfield_number++;

  }

}

uint8_t bbank() { return bank; }

uint8_t bbs() { return bs; }

void jsr(char *location) {
// determines whether to use the standard jsr (for 2k/4k or bankswitched stuff in current bank)
// or to switch banks before calling the routine
  if (!bs || bank == last_bank) {
    ap("JSR %s", location);
    return;
  }
  // we need to switch banks
  ap("STA temp7");
  // first create virtual return address
  ap("LDA #>(ret_point%d-1)", ++numjsrs);
  ap("PHA");
  ap("LDA #<(ret_point%d-1)", numjsrs);
  ap("PHA");
  // next we must push the place to jsr to
  ap("LDA #>(%s-1)", location);
  ap("PHA");
  ap("LDA #<(%s-1)", location);
  ap("PHA");
  // now store regs on stack
  ap("LDA temp7");
  ap("PHA");
  ap("TXA");
  ap("PHA");
  // select bank to switch to
  ap("LDX #%d", last_bank);
  ap("JMP BS_jsr");
  printf("ret_point%d\n", numjsrs);
}

void jsrbank1(char *location) {
  // bankswitched jsr to bank 1
  // determines whether to use the standard jsr (for 2k/4k or bankswitched stuff in current bank)
  // or to switch banks before calling the routine
  if (!bs || bank == 1) {
    ap("JSR %s", location);
    return;
  }
  // we need to switch banks
  ap("STA temp7");
  // first create virtual return address
  ap("LDA #>(ret_point%d-1)", ++numjsrs);
  ap("PHA");
  ap("LDA #<(ret_point%d-1)", numjsrs);
  ap("PHA");
  // next we must push the place to jsr to
  ap("LDA #>(%s-1)", location);
  ap("PHA");
  ap("LDA #<(%s-1)", location);
  ap("PHA");
  // now store regs on stack
  ap("LDA temp7");
  ap("PHA");
  ap("TXA");
  ap("PHA");
  // select bank to switch to
  ap("LDX #1");
  ap("JMP BS_jsr");
  printf("ret_point%d\n", numjsrs);
}

void switchjoy(char *input_source) {
  // place joystick/console switch reading code inline instead of as a subroutine
  // standard routines needed for pretty much all games
  // read switches, joysticks now compiler generated (more efficient)

  //invalidate_Areg()  // do we need this?

  char *val = NULL, *bit = NULL;

       if (MATCH(input_source, "switchreset"))  val = "01", bit = "SWCHB";
  else if (MATCH(input_source, "switchselect")) val = "02", bit = "SWCHB";
  else if (MATCH(input_source, "switchleftb"))  val = "40", bit = "SWCHB";
  else if (MATCH(input_source, "switchrightb")) val = "80", bit = "SWCHB";
  else if (MATCH(input_source, "switchbw"))     val = "08", bit = "SWCHB";
  else if (MATCH(input_source, "joy0up"))       val = "10", bit = "SWCHA";
  else if (MATCH(input_source, "joy0down"))     val = "20", bit = "SWCHA";
  else if (MATCH(input_source, "joy0left"))     val = "40", bit = "SWCHA";
  else if (MATCH(input_source, "joy0right"))    val = "80", bit = "SWCHA";
  else if (MATCH(input_source, "joy1up"))       val = "01", bit = "SWCHA";
  else if (MATCH(input_source, "joy1down"))     val = "02", bit = "SWCHA";
  else if (MATCH(input_source, "joy1left"))     val = "04", bit = "SWCHA";
  else if (MATCH(input_source, "joy1right"))    val = "08", bit = "SWCHA";
  else if (MATCH(input_source, "joy0fire"))     val = "80", bit = "INPT4";
  else if (MATCH(input_source, "joy1fire"))     val = "80", bit = "INPT5";

  if (val) {
    ap("LDA #$%s", val);
    ap("BIT %s", bit);
  }
  else prerror("invalid console switch/controller reference\n");
}

void newbank(int bankno) {
  FILE *bs_support;
  char line[500], fullpath[500];
  int len;

  if (bankno == 1) return;  // "bank 1" is ignored

  fullpath[0] = '\0';
  if (includespath[0]) {
    strcpy(fullpath, includespath);
    char c = includespath[strlen(includespath) - 1];
    if (c != '\\' && c != '/') strcat(fullpath, "/");
    strcat(fullpath, "includes/");
  }
  strcat(fullpath, "banksw.asm");

  // from here on, code will compile in the next bank
  // for 8k bankswitching, most of the libaries will go into bank 2
  // and majority of bB program in bank 1

  bank = bankno;
  if (bank > last_bank) prerror("bank not supported\n");

  printf(" echo \"    \",[(start_bank%d - *)]d , \"bytes of ROM space left in bank %d\")\n", bank - 1, bank - 1);

  // now display banksw.asm file

  if (!(bs_support = fopen("banksw.asm", "r"))) { // open file in current dir
    if (!(bs_support = fopen(fullpath, "r"))) { // open file
      fprintf(stderr, "Cannot open banksw.asm for reading\n");
      exit(1);
    }
  }

  while (fgets(line, 500, bs_support))
    if (MATCH(line, ";size=")) break;

  len = atoi(line + 6);
  if (bank == 2) sprintf(redefined_variables[numredefvars++], "bscode_length = %d", len);

  printf(" ORG $%dFF4-bscode_length\n", bank - 1);
  printf(" RORG $%XF4-bscode_length\n", (15 - bs / 2 + 2 * (bank - 1)) * 16 + 15);
  printf("start_bank%d", bank - 1);

  while (fgets(line, 500, bs_support))
    if (line[0] == ' ') printf("%s", line);

  fclose(bs_support);

  // startup vectors
  printf(" ORG $%dFFC\n", bank - 1);
  printf(" RORG $%XFC\n", (15 - bs / 2 + 2 * (bank - 1)) * 16 + 15);

  ap(".word start_bank%d", bank - 1);
  ap(".word start_bank%d", bank - 1);

  // now end
  printf(" ORG $%d000\n", bank);
  printf(" RORG $%X00\n", (15 - bs / 2 + 2 * bank) * 16);
  if (superchip) printf(" repeat 256\n" a(".byte $ff") " repend\n");

  if (bank == last_bank) printf("; bB.asm file is split here\n");

  // not working yet - need to :
  // do something I forgot
}

float immed_fixpoint(char *fixpointval) {
  int i = findpoint(fixpointval);
  char decimalpart[50];
  fixpointval[i] = '\0';
  sprintf(decimalpart, "0.%s", fixpointval + i + 1);
  return atof(decimalpart);
}

int findpoint(char *item) { // determine if fixed point var
  int i;
  for (i = 0; i < 50; ++i) {
    if (item[i] == '\0') return 50;
    if (item[i] == '.') return i;
  }
  return i;
}

void freemem(char **statement) {
  for (int i = 0; i < STATEMENT_MAX_PARTS; ++i) free(statement[i]);
  free(statement);
}

//
// The fractional part of a declared 8.8 fixpoint variable
//
char* fracpart(char *item) {
  static char outvar[50];
  for (int i = 0; i < numfixpoint88; ++i) {
    strcpy(outvar, fixpoint88[1][i]);
    if (!strcmp(fixpoint88[0][i], item)) return outvar;
  }
  // must be immediate value
  if (findpoint(item) < 50)
    sprintf(outvar, "#%d", (int)(immed_fixpoint(item) * 256.0));
  else
    sprintf(outvar, "#0");

  return outvar;
}

//
// Determine if a variable is fixed point, and if so, what kind
//
int isfixpoint(char *item) {
  removeCR(item);
  for (int i = 0; i < numfixpoint88; ++i)
    if (!strcmp(item, fixpoint88[0][i])) return 8;
  for (int i = 0; i < numfixpoint44; ++i)
    if (!strcmp(item, fixpoint44[0][i])) return 4;
  if (findpoint(item) < 50) return 12;
  return 0;
}

void set_romsize(char *size) {
  if (MATCH(size, "2k")) {
    strcpy(redefined_variables[numredefvars++], "ROM2k = 1");
  }
  else {
    int bshs = 0, bsnum = 0, bsmask = 0;
    char *sup = NULL;
    if (MATCH(size, "8k")) {
      bs = 8;
      last_bank = 2;
      bshs = 0x1FF8;
      bsnum = 8;
      bsmask = 1;
      sup = "8kSC";
    }
    else if (MATCH(size, "16k")) {
      bs = 16;
      last_bank = 4;
      bshs = 0x1FF6;
      bsnum = 16;
      bsmask = 3;
      sup = "16kSC";
    }
    else if (MATCH(size, "32k")) {
      bs = 32;
      last_bank = 8;
      bshs = 0x1FF4;
      bsnum = 32;
      bsmask = 7;
      //if (multisprite == 1) create_includes("multisprite_bankswitch.inc");
      //else
      sup = "32kSC";
    }

    if (bshs) {
      sprintf(redefined_variables[numredefvars++], "bankswitch_hotspot = $%04X", bshs);
      sprintf(redefined_variables[numredefvars], "bankswitch = %d", bsnum);
      sprintf(redefined_variables[numredefvars], "bs_mask = %d", bsmask);
      if (MATCH(size, sup)) {
        strcpy(redefined_variables[numredefvars++], "superchip = 1");
        create_includes("superchip.inc");
        superchip = true;
      }
      else
        create_includes("bankswitch.inc");
    }
    else if (strncmp(size, "4k", 2)) {
      fprintf(stderr, "Warning: unsupported ROM size: %s Using 4k.\n", size);
    }
  }
}

void add_includes(char *myinclude) { strcat(user_includes, myinclude); }

void add_inline(char *myinclude) { printf(" include %s\n", myinclude); }

void init_includes(char *path) {
  if (path) strcpy(includespath, path);
  else includespath[0] = '\0';
  user_includes[0] = '\0';
}

void barf_sprite_data() {
  int i, j, k;
  // go to the last bank before barfing the graphics
  if (!bank) bank = 1;
  for (i = bank; i < last_bank; ++i) newbank(i + 1);
  //{
  //  bank = 1;
  //  printf(" echo \"   \",[(start_bank1 - *)]d , \"bytes of ROM space left in bank 1\"\n");
  //}
  //for (i = bank; i < last_bank; ++i)
  //  printf("; bB.asm file is split here\n\n\n\n");

  for (i = 0; i < sprite_index; ++i)
    printf("%s", sprite_data[i]);

  // now we must regurgitate the PF data

  for (i = 0; i < playfield_number; ++i) {
    if (ROMpf) {
      printf(" if ((>(*+%d)) > (>*))\n ALIGN 256\n endif\n", playfield_index[i]);
      printf("PF1_data%d\n", i);
      for (j = playfield_index[i] - 1; j >= 0; j--) {
        printf("\t.byte %%");
        for (k = 15; k > 7; k--)
          printf((pfdata[i][j] & _BV(k)) ? "1" : "0");
        printf("\n");
      }

      printf(" if ((>(*+%d)) > (>*))\n ALIGN 256\n endif\n", playfield_index[i]);
      printf("PF2_data%d\n", i);
      for (j = playfield_index[i] - 1; j >= 0; j--) {
        printf("\t.byte %%");
        for (k = 0; k < 8; ++k) // reversed bit order!
          printf((pfdata[i][j] & _BV(k)) ? "1" : "0");
        printf("\n");
      }
    }
    else { // RAM pf
      // do nuttin'
    }
  }
}

void create_includes(char *includesfile) {
  FILE *includesread, *includeswrite;
  char dline[500], fullpath[500];
  int i, writeline;

  removeCR(includesfile);
  if (includesfile_already_done) return;
  includesfile_already_done = true;

  fullpath[0] = '\0';
  if (includespath[0]) {
    strcpy(fullpath, includespath);
    char c = includespath[strlen(includespath) - 1];
    if (c != '\\' && c != '/') strcat(fullpath, "/");
    strcat(fullpath, "includes/");
  }
  strcat(fullpath, includesfile);
  //for (i = 0; i < strlen(includesfile); ++i) if (includesfile[i] == '\n') includesfile[i] = '\0';
  if (!(includesread = fopen(includesfile, "r"))) { // try again in current dir
    if (!(includesread = fopen(fullpath, "r"))) { // open file
      fprintf(stderr, "Cannot open %s for reading\n", includesfile);
      exit(1);
    }
  }
  if (!(includeswrite = fopen("includes.bB", "w"))) { // open file
    fprintf(stderr, "Cannot open includes.bB for writing\n");
    exit(1);
  }

  while (fgets(dline, 500, includesread)) {
    writeline = 0;
    for (i = 0; i < 500; ++i) {
      if (dline[i] == ';' || dline[i] == '\n' || dline[i] == '\r') break;
      if (dline[i] > ' ') {
        writeline = 1;
        break;
      }
    }
    if (writeline) {
      if (!strncasecmp(dline, "bb.asm", 6) && user_includes[0])
        fprintf(includeswrite, "%s", user_includes);
      fprintf(includeswrite, "%s", dline);
    }
  }
  fclose(includesread);
  fclose(includeswrite);
}

void loadindex(char *myindex) {
  ap("LDX %s%s", immedtok(myindex), myindex); // get index
}

int getindex(char *mystatement, char *myindex) {
  int i, j, index = 0;
  for (i = 0; i < STATEMENT_MAX_PARTS; ++i) {
    if (!mystatement[i]) { i = STATEMENT_MAX_PARTS; break; }
    if (mystatement[i] == '[') { index = 1; break; }
  }
  if (i < STATEMENT_MAX_PARTS) {
    strcpy(myindex, mystatement + i + 1);
    myindex[strlen(myindex) - 1] = '\0';
    if (myindex[strlen(myindex) - 2] == ']') myindex[strlen(myindex) - 2] = '\0';
    if (myindex[strlen(myindex) - 1] == ']') myindex[strlen(myindex) - 1] = '\0';
    for (j = i; j < STATEMENT_PART_SIZE; ++j) mystatement[j] = '\0';
  }
  return index;
}

int checkmul(int value) {
  // check to see if value can be optimized to save cycles
  if (!(value & 1)) return 1;
  if (value < 11) return 1;  // always optimize these

  while (value != 1) {
    if (!(value % 9)) value /= 9;
    else if (!(value % 7)) value /= 7;
    else if (!(value % 5)) value /= 5;
    else if (!(value % 3)) value /= 3;
    else if (!(value & 1)) value >>= 1;
    else break;
    if (!(value & 1) && !(optimization & _BV(OPTI_SPEED))) break; // do not optimize multplications
  }
  return value == 1;
}

int checkdiv(int value) {
  // check to see if value is a power of two - if not, run standard div routine
  while (value != 1) {
    if (value & 1) break;
    else value >>= 1;
  }
  return value == 1;
}

void mul(char **statement, int bits) {
  // this will attempt to output optimized code depending on the multiplicand
  int multiplicand = atoi(statement[6]),
      tempstorage = 0, apply3_5_9;

  // we will optimize specifically for 2,3,5,7,9
  char *asl;
  if (bits == 16) {
    ap("LDX #0");
    ap("STX temp1");
    asl = a("ASL\n\tROL temp1");
  }
  else
    asl = a("ASL");

  while (multiplicand != 1) {
    apply3_5_9 = 0;
    if (!(multiplicand % 9)) {
      if (tempstorage) {
        strcpy(statement[4], "temp2");
        ap("STA temp2");
      }
      printf("%s", asl);
      printf("%s", asl);
      apply3_5_9 = 9;
    }
    else if (!(multiplicand % 7)) {
      if (tempstorage) {
        strcpy(statement[4], "temp2");
        ap("STA temp2");
      }
      multiplicand /= 7;
      printf("%s", asl);
      printf("%s", asl);
      printf("%s", asl);
      ap("SEC");
      ap("SBC %s%s", immedtok(statement[4]), statement[4]);
      if (bits == 16) {
        ap("TAX");
        ap("LDA temp1");
        ap("SBC #0");
        ap("STA temp1");
        ap("TXA");
      }
      tempstorage = 1;
    }
    else if (!(multiplicand % 5)) {
      if (tempstorage) {
        strcpy(statement[4], "temp2");
        ap("STA temp2");
      }
      printf("%s", asl);
      apply3_5_9 = 5;
    }
    else if (!(multiplicand % 3)) {
      if (tempstorage) {
        strcpy(statement[4], "temp2");
        ap("STA temp2");
      }
      apply3_5_9 = 3;
    }
    else if (!(multiplicand & 1)) {
      multiplicand >>= 1;
      printf("%s", asl);
    }
    else {
      ap("LDY #%d", multiplicand);
      ap("JSR mul%d", bits);
      fprintf(stderr, "Warning - there seems to be a problem.  Your code may not run properly.\n");
      fprintf(stderr, "If you are seeing this message, please report it - it could be a bug.\n");
      // WARNING: not fixed up for bankswitching
    }

    if (apply3_5_9) {
      printf("%s", asl);
      ap("CLC");
      ap("ADC %s%s", immedtok(statement[4]), statement[4]);
      if (bits == 16) {
        ap("TAX");
        ap("LDA temp1");
        ap("ADC #0");
        ap("STA temp1");
        ap("TXA");
      }
      tempstorage = 1;
      multiplicand /= apply3_5_9;
    }
  }
}

void divd(char **statement, int bits) {
  int divisor = atoi(statement[6]);
  if (bits == 16) {
    ap("LDX #0");
    ap("STX temp1");
  }
  while (divisor != 1) {
    if (!(divisor & 1)) { // div by power of two is the only easy thing
      divisor >>= 1;
      ap("LSR");
      if (bits == 16) ap("ROL temp1");  // I am not sure if this is actually correct
    }
    else {
      // Division by an odd number
      ap("LDY #%d", divisor);
      ap("JSR div%d", bits);
      divisor = 1; // [SRL] This may be needed to prevent infinite loop
      fprintf(stderr, "Warning - there seems to be a problem.  Your code may not run properly.\n");
      fprintf(stderr, "If you are seeing this message, please report it - it could be a bug.\n");
      // WARNING: Not fixed up for bankswitching
    }
  }
}

void endfunction() {
  if (!doingfunction) prerror("extraneous end keyword encountered\n");
  doingfunction = false;
}

void function(char **statement) {
  // declares a function - only needed if function is in bB.  For asm functions, see
  // the help.html file.
  // determine number of args, then run until we get an end.
  doingfunction = true;
  printf("%s\n", statement[2]);
  ap("STA temp1");
  ap("STY temp2");
}

void callfunction(char **statement) {
  // called by assignment to a variable
  // does not work as an argument in another function or an if-then... yet.
  char getindex0[STATEMENT_PART_SIZE];
  int index = 0, arguments = 0, argnum[10];
  for (int i = 6; !CMATCH(i, ')'); ++i) {
    if (!CMATCH(i, ',')) argnum[arguments++] = i;
    if (i > 45) prerror("missing \")\" at end of function call\n");
  }
  if (!arguments) fprintf(stderr, "(%d) Warning: function call with no arguments\n", line);
  while (arguments) {
    // get [index]
    index = getindex(statement[argnum[--arguments]], &getindex0[0]);
    if (index) loadindex(&getindex0[0]);

    char *opcode = arguments == 1 ? "LDY" : "LDA";
    ap("%s %s", opcode, indexpart(statement[argnum[arguments]], index));

    if (arguments > 1) ap("STA temp%d", arguments + 1);
    //arguments--;
  }
  //jsr(statement[4]);
  // need to fix above for proper function support
  ap("JSR %s", statement[4]);

  strcpy(Areg, "invalid");
}

void incline() { line++; }

int bbgetline() { return line; }

void invalidate_Areg() { strcpy(Areg, "invalid"); }

BOOL islabel(char **statement) {
  // this is for determining if the item after a "then" is a label or a statement
  // return of 0=label, 1=statement
  int i;
  // find the "then" or a "goto"
  for (i = 0; i < STATEMENT_MAX_PARTS - 2;) if (SMATCH(i++, "then")) break;
  return findlabel(statement, i);
}

BOOL islabelelse(char **statement) {
  // this is for determining if the item after an "else" is a label or a statement
  // return of 0=label, 1=statement
  int i;
  // find the "else"
  for (i = 0; i < STATEMENT_MAX_PARTS - 2;) if (SMATCH(i++, "else")) break;
  return findlabel(statement, i);
}

BOOL findlabel(char **statement, int i) {
  // 0 if label, 1 if not
  if (statement[i][0] >= '0' && statement[i][0] <= ':') return 0;
  if (CMATCH(i + 1, ':')) return !SMATCH(i + 2, "rem");

  if (SMATCH(i + 1, "else")) return 0;
  //if (SMATCH(i + 1, "bank")) return 0;
  // uncomment to allow then label bankx (needs work)

  // only possibilities left are: drawscreen, player0:, player1:, asm, next, return, maybe others added later?
  // Otherwise... I really hope we've got a label !!!!

  return (
       statement[i + 1][0]
    || SMATCH(i, "drawscreen")
    || SMATCH(i, "lives:")
    || SMATCH(i, "player0color:")
    || SMATCH(i, "player1color:")
    || SMATCH(i, "player0:")
    || SMATCH(i, "player1:")
    || SMATCH(i, "player2:")
    || SMATCH(i, "player3:")
    || SMATCH(i, "player4:")
    || SMATCH(i, "player5:")
    || SMATCH(i, "playfield:")
    || SMATCH(i, "pfcolors:")
    || SMATCH(i, "pfheights:")
    || SMATCH(i, "asm")
    || SMATCH(i, "rem")
    || SMATCH(i, "next")
    || SMATCH(i, "reboot")
    || SMATCH(i, "return")
  );
}

void sread(char **statement) {
  // read sequential data
  ap("LDX #%s", statement[6]);
  ap("LDA (0,x)");
  ap("INC 0,x");
  ap("BNE *+4");
  ap("INC 1,x");
  strcpy(Areg, "invalid");
}

void sdata(char **statement) {
  // sequential data, works like regular basics and doesn't have the 256 byte restriction
  char data[200];
  int i;
  removeCR(statement[4]);
  sprintf(redefined_variables[numredefvars++], "%s = %s", statement[2], statement[4]);
  ap("LDA #<%s_begin", statement[2]);
  ap("STA %s", statement[4]);
  ap("LDA #>%s_begin", statement[2]);
  ap("STA %s+1", statement[4]);

  ap("JMP .skip%s", statement[0]);
  // not affected by noinlinedata

  printf("%s_begin\n", statement[2]);
  while (1) {
    if ((!fgets(data, sizeof(data), stdin) || (data[0] <= '9' && data[0] >= '0')) && data[0] != 'e') {
      prerror("Error: Missing \"end\" keyword at end of data\n");
      exit(1);
    }
    line++;
    if (MATCH(data, "end")) break;
    remove_trailing_commas(data);
    for (i = 0; i < COUNT(data); ++i) {
      if ((int)data[i] > 32) break;
      if ((int)data[i] < 14) i = COUNT(data);
    }
    if (i < COUNT(data)) ap(".byte %s", data);
  }
  printf(".skip%s\n", statement[0]);

}

void data(char **statement) {
  char data[200];
  char **data_length;
  char **deallocdata_length;
  int i;
  data_length = (char**)malloc(sizeof(char*) * 50);
  for (i = 0; i < 50; ++i) data_length[i] = (char*)malloc(sizeof(char) * 50);
  deallocdata_length = data_length;
  removeCR(statement[2]);

  if (!(optimization & _BV(OPTI_NO_INLINE_DATA))) ap("JMP .skip%s", statement[0]);
  // if optimization level >=4 then data cannot be placed inline with code!

  printf("%s\n", statement[2]);
  while (1) {
    if ((!fgets(data, sizeof(data), stdin) || (data[0] <= '9' && data[0] >= '0')) && data[0] != 'e') {
      prerror("Error: Missing \"end\" keyword at end of data\n");
      exit(1);
    }
    line++;
    if (MATCH(data, "end")) break;
    remove_trailing_commas(data);
    for (i = 0; i < COUNT(data); ++i) {
      if ((int)data[i] > 32) break;
      if ((int)data[i] < 14) i = COUNT(data);
    }
    if (i < COUNT(data)) ap(".byte %s", data);
  }
  printf(".skip%s\n", statement[0]);
  strcpy(data_length[0], " ");
  strcpy(data_length[1], "const");
  sprintf(data_length[2], "%s_length", statement[2]);
  strcpy(data_length[3], "=");
  sprintf(data_length[4], ".skip%s-%s", statement[0], statement[2]);
  strcpy(data_length[5], "\n");
  data_length[6][0] = '\0';
  keywords(data_length);
  freemem(deallocdata_length);
}

void shiftdata(char **statement, int num) {
  for (int i = STATEMENT_MAX_PARTS - 1; i > num; i--)
    for (int j = 0; j < STATEMENT_PART_SIZE; ++j)
      statement[i][j] = statement[i - 1][j];
}

void compressdata(char **statement, int num1, int num2) {
  for (int i = num1; i < STATEMENT_MAX_PARTS - num2; i++)
    for (int j = 0; j < STATEMENT_PART_SIZE; ++j)
      statement[i][j] = statement[i + num2][j];
}

void ongoto(char **statement) {
  // warning!!! bankswitching not yet supported
  int k, i = 4;
  if (strcmp(statement[2], Areg)) ap("LDX %s", statement[2]);
  //ap("ASL");
  //ap("TAX");
  ap("LDA .%sjumptablehi,x", statement[0]);
  ap("PHA");
  //ap("INX");
  ap("LDA .%sjumptablelo,x", statement[0]);
  ap("PHA");
  ap("RTS");
  printf(".%sjumptablehi\n", statement[0]);
  while (!CMATCH(i, ':') && !CMATCH(i, '\0')) {
    for (k = 0; k < STATEMENT_PART_SIZE; ++k)
      if (statement[i][k] == '\n' || statement[i][k] == '\r') {
        statement[i][k] = '\0';
        break;
      }
    ap(".byte >(.%s-1)", statement[i++]);
  }
  printf(".%sjumptablelo\n", statement[0]);
  i = 4;
  while (!CMATCH(i, ':') && !CMATCH(i, '\0')) {
    for (k = 0; k < STATEMENT_PART_SIZE; ++k)
      if (statement[i][k] == '\n' || statement[i][k] == '\r') {
        statement[i][k] = '\0';
        break;
      }
    ap(".byte <(.%s-1)", statement[i++]);
  }
}

void dofor(char **statement) {
  if (strcmp(statement[4], Areg))
    ap("LDA %s%s", immedtok(statement[4]), statement[4]);

  ap("STA %s", statement[2]);

  //forlabel[numfors][0] = '\0';
  sprintf(forlabel[numfors], "%sfor%s", statement[0], statement[2]);
  printf(".%s\n", forlabel[numfors]);

  //forend[numfors][0] = '\0';
  strcpy(forend[numfors], statement[6]);

  //forvar[numfors][0] = '\0';
  strcpy(forvar[numfors], statement[2]);

  //forstep[numfors][0] = '\0';
  strcpy(forstep[numfors], IMATCH(7, "step") ? statement[8] : "1");

  numfors++;
}

void next(char **statement) {
  BOOL immed = false, immedend = false;
  int failsafe = 0;
  char failsafelabel[60];

  invalidate_Areg();

  if (!numfors) {
    fprintf(stderr, "(%d) next without for\n", line);
    exit(1);
  }
  numfors--;
  if (!strncmp(forstep[numfors], "1\0", 2)) {
    // step 1
    ap("LDA %s", forvar[numfors]);
    ap("CMP %s%s", immedtok(forend[numfors]), forend[numfors]);
    ap("INC %s", forvar[numfors]);
    bcc(forlabel[numfors]);
  }
  else if (!strncmp(forstep[numfors], "-1\0", 3) || !strncmp(forstep[numfors], "255\0", 4)) {
    // step -1
    ap("DEC %s", forvar[numfors]);
    if (strncmp(forend[numfors], "1\0", 2)) {
      ap("LDA %s", forvar[numfors]);
      ap("CMP %s%s", immedtok(forend[numfors]), forend[numfors]);
      bcs(forlabel[numfors]);
    }
    else
      bne(forlabel[numfors]);
  }
  else { // step other than 1 or -1
    // normally, the generated code adds to or subtracts from the for variable, and checks
    // to see if it's less than the ending value.
    // however, if the step would make the variable less than zero or more than 255
    // then this check will not work.  The compiler will attempt to detect this situation
    // if the step and end are known.  If the step and end are not known (that is,
    // either is a variable) then much more complex code must be generated.

    ap("LDA %s", forvar[numfors]);
    ap("CLC");
    ap("ADC %s%s", immedtok(forstep[numfors]), forstep[numfors]);

    immed = isimmed(forstep[numfors]);
    if (immed && isimmed(forend[numfors])) { // the step and end are immediate
      if (atoi(forstep[numfors]) & 128) { // step is negative
        if ((256 - (atoi(forstep[numfors]) & 255)) >= atoi(forend[numfors])) {
          // if we are in danger of going < 0...we will have carry clear after ADC
          failsafe = 1;
          sprintf(failsafelabel, "%s_failsafe", forlabel[numfors]);
          bcc(failsafelabel);
        }
      }
      else { // step is positive
        if (atoi(forstep[numfors]) + atoi(forend[numfors]) > 255) {
          // if we are in danger of going > 255...we will have carry set after ADC
          failsafe = 1;
          sprintf(failsafelabel, "%s_failsafe", forlabel[numfors]);
          bcs(failsafelabel);
        }
      }

    }
    ap("STA %s", forvar[numfors]);

    printf("\tCMP %s", immedtok(forend[numfors]));
    // add 1 to immediate compare for increasing loops
    immedend = isimmed(forend[numfors]);
    if (immedend && !(atoi(forstep[numfors]) & 128))
      strcat(forend[numfors], "+1");
    printf("%s\n", forend[numfors]);
    // if step number is 1 to 127 then add 1 and use bcc, otherwise bcs
    // if step is a variable, we'll need to check for every loop iteration
    //
    // Warning! no failsafe checks with variables as step or end - it's the
    // programmer's job to make sure the end value doesn't overflow
    if (!immed) {
      ap("LDX %s", forstep[numfors]);
      ap("BMI .%sbcs", statement[0]);
      bcc(forlabel[numfors]);
      ap("CLC");
      printf(".%sbcs\n", statement[0]);
      bcs(forlabel[numfors]);
    }
    else if (atoi(forstep[numfors]) & 128)
      bcs(forlabel[numfors]);
    else {
      bcc(forlabel[numfors]);
      if (!immedend) beq(forlabel[numfors]);
    }
  }
  if (failsafe) printf(".%s\n", failsafelabel);
}

void dim(char **statement) {
  // just take the statement and pass it right to a header file
  char *fixpointvar1;
  char fixpointvar2[50];
  // check for fixedpoint variables
  int i = findpoint(statement[4]);
  if (i < 50) {
    removeCR(statement[4]);
    strcpy(fixpointvar2, statement[4]);
    fixpointvar1 = fixpointvar2 + i + 1;
    fixpointvar2[i] = '\0';
    if (!strcmp(fixpointvar1, fixpointvar2)) { // we have 4.4
      strcpy(fixpoint44[1][numfixpoint44], fixpointvar1);
      strcpy(fixpoint44[0][numfixpoint44++], statement[2]);
    }
    else { // we have 8.8
      strcpy(fixpoint88[1][numfixpoint88], fixpointvar1);
      strcpy(fixpoint88[0][numfixpoint88++], statement[2]);
    }
    statement[4][i] = '\0'; // terminate string at '.'
  }
  i = 2;
  redefined_variables[numredefvars][0] = '\0';
  while (!CMATCH(i, '\0')) {
    strcat(redefined_variables[numredefvars], statement[i++]);
    strcat(redefined_variables[numredefvars], " ");
  }
  numredefvars++;
}

void doconst(char **statement) {
  // basically the same as dim, except we keep a queue of variable names that are constant
  int i = 2;
  redefined_variables[numredefvars][0] = '\0';
  while (!CMATCH(i, '\0')) {
    strcat(redefined_variables[numredefvars], statement[i++]);
    strcat(redefined_variables[numredefvars], " ");
  }
  numredefvars++;
  strcpy(constants[numconstants++], statement[2]); // record to queue
}

void doreturn(char **statement) {
  int i, j, index = 0;
  char getindex0[STATEMENT_PART_SIZE];
  int bankedreturn = 0;
  // 0=no special action
  // 1=return thisbank
  // 2=return otherbank

  if (SMATCH(2, "thisbank") || SMATCH(3, "thisbank")) bankedreturn = 1;
  else if (SMATCH(2, "otherbank") || SMATCH(3, "otherbank")) bankedreturn = 2;

  // several types of returns:
  // return by itself (or with a value) can return to any calling bank
  // this one has the most overhead in terms of cycles and ROM space
  // use sparingly if cycles or space are an issue

  // return [value] thisbank will only return within the same bank
  // this one is the fastest

  // return [value] otherbank will slow down returns to the same bank
  // but speed up returns to other banks - use if you are primarily returning
  // across banks

  if (!CMATCH(2, '\0') && !CMATCH(2, ' ') && !SMATCH(2, "thisbank") && !SMATCH(2, "otherbank")) {

    index |= getindex(statement[2], &getindex0[0]);

    if (index & 1) loadindex(&getindex0[0]);

    char *opcode = bankedreturn ? "LDA" : "LDY";
    ap("%s %s", opcode, indexpart(statement[2], index & 1));
  }

  if (bankedreturn == 1) {
    ap("RTS");
    return;
  }
  if (bankedreturn == 2) {
    ap("JMP BS_return");
    return;
  }

  if (bs) { // check if sub was called from the same bank
    ap("TSX");
    ap("LDA 2,x ; check return address");
    ap("EOR #(>*) ; vs. current PCH");
    ap("AND #$E0 ;  mask off all but top 3 bits");
    // if zero, then banks are the same
    if (!CMATCH(2, '\0') && !CMATCH(2, ' ')) {
      ap("BEQ *+6 ; if equal, do normal return");
      ap("TYA");
    }
    else
      ap("BEQ *+5 ; if equal, do normal return");
    ap("JMP BS_return");
  }

  if (!CMATCH(2, '\0') && !CMATCH(2, ' ')) ap("TYA");
  ap("RTS");
}

void pfread(char **statement) {
  char getindex0[STATEMENT_PART_SIZE], getindex1[STATEMENT_PART_SIZE];
  int i, j, index;
  invalidate_Areg();

  index = getindex(statement[3], &getindex0[0]) | getindex(statement[4], &getindex1[0]) << 1;

  if (index & 1) loadindex(&getindex0[0]);

  ap("LDA %s", indexpart(statement[4], index & 1));

  if (index & 2) loadindex(&getindex1[0]);

  ap("LDY %s", indexpart(statement[6], index & 2));

  jsr("pfread");
}

void pfpixel(char **statement) {
  char getindex0[STATEMENT_PART_SIZE], getindex1[STATEMENT_PART_SIZE];
  int i, j, index;
  if (multisprite == 1) { prerror("Error: pfpixel not supported in multisprite kernel\n"); exit(1); }
  invalidate_Areg();

  index = getindex(statement[2], &getindex0[0]) | getindex(statement[3], &getindex1[0]) << 1;
  if (index & 1) loadindex(&getindex0[0]);

  ap("LDA %s", indexpart(statement[2], index & 1));

  if (index & 2) loadindex(&getindex1[0]);

  ap("LDY %s", indexpart(statement[3], index & 2));

  ap("LDX #%d", SMATCH(4, "flip") ? 2 : SMATCH(4, "off") ? 1 : 0);
  jsr("pfpixel");
}

void pfhline(char **statement) {
  char getindex0[STATEMENT_PART_SIZE], getindex1[STATEMENT_PART_SIZE], getindex2[STATEMENT_PART_SIZE];
  int i, j, index = 0;
  if (multisprite == 1) {
    prerror("Error: pfhline not supported in multisprite kernel\n");
    exit(1);
  }

  invalidate_Areg();

  index |= getindex(statement[2], &getindex0[0]);
  index |= getindex(statement[3], &getindex1[0]) << 1;
  index |= getindex(statement[4], &getindex2[0]) << 2;

  if (index & 4) loadindex(&getindex2[0]);
  ap("LDA %s", indexpart(statement[4], index & 4));
  ap("STA temp3");

  if (index & 1) loadindex(&getindex0[0]);
  ap("LDA %s", indexpart(statement[2], index & 1));

  if (index & 2) loadindex(&getindex1[0]);
  ap("LDY %s", indexpart(statement[3], index & 2));

  ap("LDX #%d", SMATCH(5, "flip") ? 2 : SMATCH(5, "off") ? 1 : 0);

  jsr("pfhline");
}

void pfvline(char **statement) {
  char getindex0[STATEMENT_PART_SIZE], getindex1[STATEMENT_PART_SIZE], getindex2[STATEMENT_PART_SIZE];
  int i, j, index = 0;
  if (multisprite == 1) { prerror("Error: pfvline not supported in multisprite kernel\n"); exit(1); }

  invalidate_Areg();

  index |= getindex(statement[2], &getindex0[0]);
  index |= getindex(statement[3], &getindex1[0]) << 1;
  index |= getindex(statement[4], &getindex2[0]) << 2;

  if (index & 4) loadindex(&getindex2[0]);
  ap("LDA %s", indexpart(statement[4], index & 4));
  ap("STA temp3");

  if (index & 1) loadindex(&getindex0[0]);
  ap("LDA %s", indexpart(statement[2], index & 1));

  if (index & 2) loadindex(&getindex1[0]);
  ap("LDY %s", indexpart(statement[3], index & 2));

  ap("LDX #%d", SMATCH(5, "flip") ? 2 : SMATCH(5, "off") ? 1 : 0);

  jsr("pfvline");
}

void pfscroll(char **statement) {
  if (multisprite == 1) {prerror("Error: pfscroll support not yet implemented in multisprite kernel\n"); exit(1); }
  invalidate_Areg();

  printf("\tLDA #");

       // With this code, "pfscroll lexluthor" will scroll left
       if (!strncmp(statement[2], "left", 2))     printf("0\n"); // Allow "le"
  else if (!strncmp(statement[2], "right", 2))    printf("1\n"); // Allow "ri"
  else if (SMATCH(2, "upup"))                     printf("6\n");
  else if (!strncmp(statement[2], "downdown", 6)) printf("8\n"); // Allow "downdo"
  else if (SMATCH(2, "up"))                       printf("2\n");
  else if (!strncmp(statement[2], "down", 2))     printf("4\n"); // Allow "do"
  else {
    fprintf(stderr, "(%d) pfscroll direction unknown\n", line);
    exit(1);
  }
  jsr("pfscroll");
}

void doasm() {
  char data[100];
  while (1) {
    if ((!fgets(data, sizeof(data), stdin) || (data[0] <= '9' && data[0] >= '0')) && data[0] != 'e') {
      prerror("Error: Missing \"end\" keyword at end of inline asm\n");
      exit(1);
    }
    line++;
    if (MATCH(data, "end")) break;
    printf("%s\n", data);
  }
}

void player(char **statement) {
  int height = 0; //calculating sprite height
  int doingcolor = 0; //doing player colors?
  char label[50], data[100];
  // char bytes[1000];
  // char byte[100];
  char pl = statement[1][6];
  if (statement[1][7] == 'c') doingcolor = 1;
  // bytes[0] = '\0';
  int heightrecord;
  if (!doingcolor) sprintf(label, "player%s_%c\n", statement[0], pl);
  else sprintf(label, "playercolor%s_%c\n", statement[0], pl);

  ap("LDA #<%s", label);
  if (!doingcolor) ap("STA player%cpointerlo", pl);
  else ap("STA player%ccolor", pl);

  ap("LDA #>%s", label);
  if (!doingcolor) ap("STA player%cpointerhi", pl);
  else ap("STA player%ccolor+1", pl);

  //ap("JMP .%sjump%c", statement[0], pl);

  // insert DASM stuff to prevent page-wrapping of player data
  // stick this in a data file instead of displaying

  heightrecord = sprite_index++;
  sprite_index += 2;
  // record index for creation of the line below

  if (multisprite == 1) {
    sprintf(sprite_data[sprite_index++], " if (<*) < 100\n");
    sprintf(sprite_data[sprite_index++], "   repeat (100-<*)\n" a(".byte 0"));
    sprintf(sprite_data[sprite_index++], "   repend\n endif\n");
  }

  sprintf(sprite_data[sprite_index++], "%s\n", label);
  sprintf(sprite_data[sprite_index++], a(".byte 0"));

  while (1) {
    if ((!fgets(data, sizeof(data), stdin) || (data[0] <= '9' && data[0] >= '0')) && data[0] != 'e') {
      prerror("Error: Missing \"end\" keyword at end of player declaration\n");
      exit(1);
    }
    line++;
    if (MATCH(data, "end")) break;
    height++;
    sprintf(sprite_data[sprite_index++], "\t.byte %s", data);
    //strcat(bytes, byte);
  }

  // record height
  sprintf(sprite_data[heightrecord], " if (<*) > (<(*+%d))\n", height + 1);
  sprintf(sprite_data[heightrecord + 1], "  repeat ($100-<*)\n" a(".byte 0"));
  sprintf(sprite_data[heightrecord + 2], "  repend\n endif\n");

  //printf(".%sjump%c\n", statement[0], pl);
  if (multisprite == 1)
    ap("LDA #%d", height + 2);
  else if (!doingcolor)
    ap("LDA #%d", height);

  if (!doingcolor)
    ap("STA player%cheight", pl);
}

void lives(char **statement) {
  int height = 8, i = 0; //height is always 8
  char label[50], data[100];
  if (!lifekernel) {
    lifekernel = true;
    //strcpy(redefined_variables[numredefvars++], "lifekernel = 1");
  }

  sprintf(label, "lives__%s\n", statement[0]);
  removeCR(label);

  ap("LDA #<%s", label);
  ap("STA lifepointer");

  ap("LDA lifepointer+1");
  ap("AND #$E0");
  ap("ORA #(>%s)&($1F)", label);
  ap("STA lifepointer+1");

  sprintf(sprite_data[sprite_index++], " if (<*) > (<(*+8))\n");
  sprintf(sprite_data[sprite_index++], "   repeat ($100-<*)\n" a(".byte 0"));
  sprintf(sprite_data[sprite_index++], "   repend\n endif\n");

  sprintf(sprite_data[sprite_index++], "%s\n", label);

  for (i = 0; i < 9; ++i) {
    if ((!fgets(data, sizeof(data), stdin) || (data[0] <= '9' && data[0] >= '0')) && data[0] != 'e') {
      prerror("Error: Not enough data or missing \"end\" keyword at end of lives declaration\n");
      exit(1);
    }
    line++;
    if (MATCH(data, "end")) break;
    sprintf(sprite_data[sprite_index++], "\t.byte %s", data);
  }
}

void doif(char **statement) {
  int index = 0, not = 0, i, j, k, bit = 0, Aregmatch = 0;
  char getindex0[STATEMENT_PART_SIZE], getindex1[STATEMENT_PART_SIZE],
       Aregcopy[STATEMENT_PART_SIZE],
       **cstatement, // conditional statement
       **deallocstatement; // for deallocation

  strcpy(Aregcopy, "index-invalid");

  deallocstatement = cstatement = (char**)malloc(sizeof(char*) * 50);
  for (k = 0; k < 50; ++k) cstatement[k] = (char*)malloc(sizeof(char) * 50);
  for (k = 0; k < 50; ++k) for (j = 0; j < 50; ++j) cstatement[j][k] = '\0';
  if (CMATCH(2, '!')) {
    not = 1;
    for (i = 0; i < STATEMENT_PART_SIZE - 1; ++i)
      statement[2][i] = statement[2][i + 1];
  }
  if (SMATCH(2, "joy") || SMATCH(2, "switch")) {
    switchjoy(statement[2]);
    if (!islabel(statement)) {
      not ? bne(statement[4]) : beq(statement[4]);
      // printf("\tB%s ", not ? "NE" : "EQ");
      // printf(".%s\n", statement[4]);
      freemem(deallocstatement);
      return;
    }
    else { // then statement
      ap("B%s .skip%s", not ? "EQ" : "NE", statement[0]);

      // separate statement
      for (i = 3; i < STATEMENT_MAX_PARTS; ++i)
        for (k = 0; k < STATEMENT_PART_SIZE; ++k)
          cstatement[i - 3][k] = statement[i][k];

      printf(".condpart%d\n", condpart++);
      keywords(cstatement);
      printf(".skip%s\n", statement[0]);

      freemem(deallocstatement);
      return;
    }
  }

  if (SMATCH(2, "pfread")) {
    pfread(statement);
    if (!islabel(statement)) {
      not ? bne(statement[9]) : beq(statement[9]);
      freemem(deallocstatement);
      return;
    }
    else { // then statement
      ap("B%s .skip%s", not ? "EQ" : "NE", statement[0]);

      // separate statement
      for (i = 8; i < STATEMENT_MAX_PARTS; ++i)
        for (k = 0; k < STATEMENT_PART_SIZE; ++k)
          cstatement[i - 8][k] = statement[i][k];

      printf(".condpart%d\n", condpart++);
      keywords(cstatement);
      printf(".skip%s\n", statement[0]);
      freemem(deallocstatement);
      return;
    }
  }

  if (SMATCH(2, "collision(")) {

    unsigned char unknown_coll = 0;
    char *bitstr = NULL;
    char colbits = 0;
    unsigned char nxt = 0;

         if (MATCH(&statement[2][10], "missile0,"))  { nxt = 9;  colbits |= _BV(COLL_M0); }
    else if (MATCH(&statement[2][10], "missile1,"))  { nxt = 9;  colbits |= _BV(COLL_M1); }
    else if (MATCH(&statement[2][10], "player0,"))   { nxt = 8;  colbits |= _BV(COLL_P0); }
    else if (MATCH(&statement[2][10], "player1,"))   { nxt = 8;  colbits |= _BV(COLL_P1); }
    else if (MATCH(&statement[2][10], "playfield,")) { nxt = 10; colbits |= _BV(COLL_PF); }
    else if (MATCH(&statement[2][10], "ball,"))      { nxt = 5;  colbits |= _BV(COLL_BALL); }

    if (colbits) {
           if (MATCH(&statement[2][10 + nxt], "missile0)"))  { colbits |= _BV(COLL_M0); }
      else if (MATCH(&statement[2][10 + nxt], "missile1)"))  { colbits |= _BV(COLL_M1); }
      else if (MATCH(&statement[2][10 + nxt], "player0)"))   { colbits |= _BV(COLL_P0); }
      else if (MATCH(&statement[2][10 + nxt], "player1)"))   { colbits |= _BV(COLL_P1); }
      else if (MATCH(&statement[2][10 + nxt], "playfield)")) { colbits |= _BV(COLL_PF); }
      else if (MATCH(&statement[2][10 + nxt], "ball)"))      { colbits |= _BV(COLL_BALL); }
    }

    switch (colbits) {
      case _BV(COLL_M0)|_BV(COLL_M1):   bitstr = "PPMM";  bit = 6; break;
      case _BV(COLL_M0)|_BV(COLL_P0):   bitstr = "M0P";   bit = 6; break;
      case _BV(COLL_M0)|_BV(COLL_P1):   bitstr = "M0P";   bit = 7; break;
      case _BV(COLL_M0)|_BV(COLL_BALL): bitstr = "M0FB";  bit = 6; break;
      case _BV(COLL_M0)|_BV(COLL_PF):   bitstr = "M0FB";  bit = 7; break;

      case _BV(COLL_M1)|_BV(COLL_P0):   bitstr = "M1P";   bit = 7; break;
      case _BV(COLL_M1)|_BV(COLL_P1):   bitstr = "M1P";   bit = 6; break;
      case _BV(COLL_M1)|_BV(COLL_BALL): bitstr = "M1FB";  bit = 6; break;
      case _BV(COLL_M1)|_BV(COLL_PF):   bitstr = "M1FB";  bit = 7; break;

      case _BV(COLL_P0)|_BV(COLL_P1):   bitstr = "PPMM";  bit = 7; break;
      case _BV(COLL_P0)|_BV(COLL_BALL): bitstr = "P0FB";  bit = 6; break;
      case _BV(COLL_P0)|_BV(COLL_PF):   bitstr = "P0FB";  bit = 7; break;

      case _BV(COLL_P1)|_BV(COLL_PF):   bitstr = "P1FB";  bit = 7; break;
      case _BV(COLL_P1)|_BV(COLL_BALL): bitstr = "P1FB";  bit = 6; break;

      case _BV(COLL_BALL)|_BV(COLL_PF): bitstr = "BLPF";  bit = 7; break;

      default: unknown_coll = 1;
    }

    if (unknown_coll) {
      fprintf(stderr, "(%d) Error: Unknown collision type: %s\n", line, statement[2] + 9);
      exit(1);
    }
    else {
      ap("BIT CX%s", bitstr);
    }

    if (!islabel(statement)) {
      bit == 7 ? (not ? bpl(statement[4]) : bmi(statement[4]))
               : (not ? bvc(statement[4]) : bvs(statement[4]));
      //{printf("\tB%s ", bit == 7 ? "MI" : "VS");}
      //      else { printf("\tB%s ", bit == 7 ? "PL" : "VC"); }
      //      printf(".%s\n", statement[4]);
      freemem(deallocstatement);
      return;
    }
    else { // then statement
      ap("B%s .skip%s", bit == 7 ? (not ? "MI" : "PL") : (not ? "VS" : "VC"), statement[0]);

      // separate statement
      for (i = 3; i < STATEMENT_MAX_PARTS; ++i)
        for (k = 0; k < STATEMENT_PART_SIZE; ++k)
          cstatement[i - 3][k] = statement[i][k];

      printf(".condpart%d\n", condpart++);
      keywords(cstatement);
      printf(".skip%s\n", statement[0]);

      freemem(deallocstatement);
      return;
    }
  }

  // check for array, e.g. x{1} to get bit 1 of x
  for (i = 0; i < STATEMENT_PART_SIZE; ++i) {
    if (!statement[2][i]) { i = STATEMENT_PART_SIZE; break; }
    if (statement[2][i] == '}') break;
  }
  if (i < STATEMENT_PART_SIZE) { // found array
    // extract expression in parentheses - for now just whole numbers allowed
    bit = (int)statement[2][i - 1] - '0';
    if (bit > 9 || bit < 0) {
      fprintf(stderr, "(%d) Error: variables in bit access not supported\n", line);
      exit(1);
    }
    if (bit == 7 || bit == 6) { // if bit 6 or 7, we can use BIT and save 2 bytes
      printf("\tBIT ");
      for (i = 0; i < STATEMENT_PART_SIZE; ++i) {
        if (statement[2][i] == '{') break;
        printf("%c", statement[2][i]);
      }
      printf("\n");
      if (!islabel(statement)) {
        bit == 7 ? (not ? bpl(statement[4]) : bmi(statement[4]))
                 : (not ? bvc(statement[4]) : bvs(statement[4]));
        freemem(deallocstatement);
        return;
      }
      else { // then statement
        ap("B%s .skip%s", bit == 7 ? (not ? "MI" : "PL") : (not ? "VS" : "VC"), statement[0]);

        // separate statement
        for (i = 3; i < STATEMENT_MAX_PARTS; ++i)
          for (k = 0; k < STATEMENT_PART_SIZE; ++k)
            cstatement[i - 3][k] = statement[i][k];

        printf(".condpart%d\n", condpart++);
        keywords(cstatement);
        printf(".skip%s\n", statement[0]);

        freemem(deallocstatement);
        return;
      }
    }
    else {
      Aregmatch = 0;
      printf("\tLDA ");
      for (i = 0; i < STATEMENT_PART_SIZE; ++i) {
        if (statement[2][i] == '{') break;
        printf("%c", statement[2][i]);
      }
      printf("\n");
      // if bit 0, we can use LSR and save a byte
      bit ? ap("AND #%d", _BV(bit)) : ap("LSR");
      if (!islabel(statement)) {
        bit ? (not ? beq(statement[4]) : bne(statement[4]))
            : (not ? bcc(statement[4]) : bcs(statement[4]));
        freemem(deallocstatement);
        return;
      }
      else { // then statement
        ap("B%s .skip%s", bit ? (not ? "NE" : "EQ") : (not ? "CS" : "CC"), statement[0]);

        // separate statement
        for (i = 3; i < STATEMENT_MAX_PARTS; ++i)
          for (k = 0; k < STATEMENT_PART_SIZE; ++k)
            cstatement[i - 3][k] = statement[i][k];

        printf(".condpart%d\n", condpart++);
        keywords(cstatement);
        printf(".skip%s\n", statement[0]);

        freemem(deallocstatement);
        return;
      }
    }
  }

  // generic if-then (no special considerations)
  //check for [indexing]
  strcpy(Aregcopy, statement[2]);
  if (!strcmp(statement[2], Areg)) Aregmatch = 1; // do we already have the correct value in A?

  index |= getindex(statement[2], &getindex0[0]);
  if (!SMATCH(3, "then"))
    index |= getindex(statement[4], &getindex1[0]) << 1;

  if (!Aregmatch) { // do we already have the correct value in A?
    if (index & 1) loadindex(&getindex0[0]);
    ap("LDA %s", indexpart(statement[2], index & 1));
    strcpy(Areg, Aregcopy);
  }
  if (index & 2) loadindex(&getindex1[0]);
  // todo:check for cmp #0--useless except for <, > to clear carry
  if (!SMATCH(3, "then")) {
    ap("CMP %s", indexpart(statement[4], index & 2));
  }

  if (!islabel(statement)) { // then linenumber
         if (CMATCH(3, '=')) beq(statement[6]);
    else if (SMATCH(3, "<>")) bne(statement[6]);
    else if (CMATCH(3, '<')) bcc(statement[6]);
    else if (CMATCH(3, '>')) bcs(statement[6]);
    else if (SMATCH(3, "then"))
      not ? beq(statement[4]) : bne(statement[4]);
  }
  else { // then statement
    // first, take negative of condition and branch around statement
         if (CMATCH(3, '=')) printf("\tBNE ");
    else if (!strcmp(statement[3], "<>")) printf("\tBEQ ");
    else if (CMATCH(3, '<')) printf("\tBCS ");
    else if (CMATCH(3, '>')) printf("\tBCC ");
    j = 5;

    if (SMATCH(3, "then")) {
      j = 3;
      printf(not ? "\tBNE " : "\tBEQ ");
    }
    printf(".skip%s\n", statement[0]);

    // separate statement
    for (i = j; i < STATEMENT_MAX_PARTS; ++i)
      for (k = 0; k < STATEMENT_PART_SIZE; ++k)
        cstatement[i - j][k] = statement[i][k];

    printf(".condpart%d\n", condpart++);
    keywords(cstatement);
    printf(".skip%s\n", statement[0]);

    freemem(deallocstatement);
    return;

    // i = 4;
    // while (!CMATCH(i, '\0'))
    //  cstatement[i - 4] = statement[i++];
    // keywords(cstatement);
    // printf(".skip%s\n", statement[0]);
  }
  freemem(deallocstatement);
}

int getcondpart() { return condpart; }

int orderofoperations(char op1, char op2) {
  // specify order of operations for complex equations
  // i.e.: parens, divmul (*/), +-, logical (^&|)
  if (op1 == '(' || op2 == '(') return 0;
  if (op2 == ')') return 1;
  if ( (op1 == '^' || op1 == '|' || op1 == '&')
    && (op2 == '^' || op2 == '|' || op2 == '&')
  ) return 0;
  // if (op1 == '^') return 1;
  // if (op1 == '&') return 1;
  // if (op1 == '|') return 1;
  // if (op2 == '^') return 0;
  // if (op2 == '&') return 0;
  // if (op2 == '|') return 0;
  if (op1 == '*' || op1 == '/') return 1;
  if (op2 == '*' || op2 == '/') return 0;
  if (op1 == '+' || op1 == '-') return 1;
  if (op2 == '+' || op2 == '-') return 0;
  return 1;
}

int isoperator(char op) {
  switch (op) {
    case '+': case '-': case '/':
    case '*': case '&': case '|':
    case '^': case ')': case '(': return 1;
  }
  return 0;
}

void displayoperation(char *opcode, char *operand, int index) {
  if (MATCH(operand, "stackpull")) {
    if (opcode[0] == '-') {
      // operands swapped
      ap("TAY");
      ap("PLA");
      ap("TSX");
      ap("STY $00,x");
      ap("SEC");
      ap("SBC $100,x");
    }
    else if (opcode[0] == '/') {
      // operands swapped
      ap("TAY");
      ap("PLA");
    }
    else {
      ap("TSX");
      ap("INX");
      ap("TXS");
      ap("%s $100,x", opcode + 1);
    }
  }
  else
    ap("%s %s", opcode + 1, indexpart(operand, index));
}

void dolet(char **cstatement) {
  int i, j = 0, bit = 0, k, index = 0,
      score[6] = { 0, 0, 0, 0, 0, 0 },
      Aregmatch = 0,
      fixpoint1, fixpoint2, fixpoint3 = 0,
      sp = 0, // stack pointer for converting to rpn
      numrpn = 0;
  char getindex0[STATEMENT_PART_SIZE], getindex1[STATEMENT_PART_SIZE], getindex2[STATEMENT_PART_SIZE],
       **statement, *getbitvar,
       Aregcopy[STATEMENT_PART_SIZE],
       **deallocstatement,
       **rpn_statement, // expression in rpn
       rpn_stack[100][100], // prolly doesn't need to be this big
       **atomic_statement, // singular statements to recurse back to here
       tempstatement1[100], tempstatement2[100];

  strcpy(Aregcopy, "index-invalid");

  statement = (char**)malloc(sizeof(char*) * STATEMENT_MAX_PARTS);
  deallocstatement = statement;
  if (MATCH(cstatement[2], "=")) {
    for (i = STATEMENT_MAX_PARTS - 2; i > 0; --i)
      statement[i + 1] = cstatement[i];
  }
  else
    statement = cstatement;

  // check for unary minus (e.g. a = -a) and insert zero before it
  if (CMATCH(4, '-') && statement[5][0] >= '@') {
    shiftdata(statement, 4);
    statement[4][0] = '0';
  }

  fixpoint1 = isfixpoint(statement[2]);
  fixpoint2 = isfixpoint(statement[4]);
  removeCR(statement[4]);

  // check for complex statement
  if (!(CMATCH(4, '-') && CMATCH(6, ':')) &&
      !CMATCH(5, ':') && !CMATCH(7, ':') &&
      !(CMATCH(5, '(') && !CMATCH(4, '(')) &&
      (unsigned char)statement[5][0] > ' ' &&
      (unsigned char)statement[7][0] > ' '
  ) {
    printf("; complex statement detected\n");
    // complex statement here, hopefully.
    // convert equation to reverse-polish notation so we can express it in terms of
    // atomic equations and stack pushes/pulls
    rpn_statement = (char**)malloc(sizeof(char*) * 100);
    for (i = 0; i < 100; ++i) {
      rpn_statement[i] = (char*)malloc(sizeof(char) * 100);
      for (k = 0; k < 100; ++k) rpn_statement[i][k] = '\0';
    }

    atomic_statement = (char**)malloc(sizeof(char*) * 10);
    for (i = 0; i < 10; ++i) {
      atomic_statement[i] = (char*)malloc(sizeof(char) * 100);
      for (k = 0; k < 100; ++k) atomic_statement[i][k] = '\0';
    }

    // this converts expression to rpn
    for (k = 4; !CMATCH(k, '\0') && !CMATCH(k, ':'); k++) {
      // ignore CR/LF
      if (CMATCH(k, '\n') || CMATCH(k, '\r')) continue;

      strcpy(tempstatement1, statement[k]);
      if (!isoperator(tempstatement1[0])) {
        strcpy(rpn_statement[numrpn++], tempstatement1);
      }
      else {
        while (sp && orderofoperations(rpn_stack[sp - 1][0], tempstatement1[0])) {
          sp--;
          strcpy(tempstatement2, rpn_stack[sp]);
          strcpy(rpn_statement[numrpn++], tempstatement2);
        }
        if (sp && tempstatement1[0] == ')')
          sp--;
        else
          strcpy(rpn_stack[sp++], tempstatement1);
      }
    }

    // get stuff off of our rpn stack
    while (sp) {
      sp--;
      strcpy(tempstatement2, rpn_stack[sp]);
      strcpy(rpn_statement[numrpn++], tempstatement2);
    }

    //for (i = 0; i < 20; ++i) fprintf(stderr, "%s ", rpn_statement[i]); i = 0;

    // now parse rpn statement

    //strcpy(atomic_statement[2], "Areg");
    //strcpy(atomic_statement[3], "=");
    //strcpy(atomic_statement[4], rpn_statement[0]);
    //strcpy(atomic_statement[5], rpn_statement[2]);
    //strcpy(atomic_statement[6], rpn_statement[1]);
    //dolet(atomic_statement);

    sp = 0; // now use as pointer into rpn_statement
    while (sp < numrpn) {
      // clear atomic statement cache
      for (i = 0; i < 10; ++i) for (k = 0; k < 100; ++k) atomic_statement[i][k] = '\0';
      if (isoperator(rpn_statement[sp][0])) {
        // operator: need stack pull as 2nd arg
        // Areg=Areg (op) stackpull
        strcpy(atomic_statement[2], "Areg");
        strcpy(atomic_statement[3], "=");
        strcpy(atomic_statement[4], "Areg");
        strcpy(atomic_statement[5], rpn_statement[sp++]);
        strcpy(atomic_statement[6], "stackpull");
        dolet(atomic_statement);
      }
      else if (isoperator(rpn_statement[sp + 1][0])) {
        // val,operator: Areg=Areg (op) val
        strcpy(atomic_statement[2], "Areg");
        strcpy(atomic_statement[3], "=");
        strcpy(atomic_statement[4], "Areg");
        strcpy(atomic_statement[6], rpn_statement[sp++]); // val
        strcpy(atomic_statement[5], rpn_statement[sp++]); // op
        dolet(atomic_statement);
      }
      else if (isoperator(rpn_statement[sp + 2][0])) {
        // val,val,operator: stackpush, then Areg=val1 (op) val2
        if (sp) ap("PHA");
        strcpy(atomic_statement[2], "Areg");
        strcpy(atomic_statement[3], "=");
        strcpy(atomic_statement[4], rpn_statement[sp++]); // val
        strcpy(atomic_statement[6], rpn_statement[sp++]); // val
        strcpy(atomic_statement[5], rpn_statement[sp++]); // op
        dolet(atomic_statement);
      }
      else {
        if (rpn_statement[sp] == '\0' || rpn_statement[sp + 1] == '\0' || rpn_statement[sp + 2] == '\0') {
          // incomplete or unrecognized expression
          prerror("Cannot evaluate expression\n");
          exit(1);
        }
        // val,val,val: stackpush, then load of next value
        if (sp) ap("PHA");
        strcpy(atomic_statement[2], "Areg");
        strcpy(atomic_statement[3], "=");
        strcpy(atomic_statement[4], rpn_statement[sp++]);
        dolet(atomic_statement);
      }
    }
    // done, now assign A-reg to original value
    for (i = 0; i < 10; ++i) for (k = 0; k < 100; ++k) atomic_statement[i][k] = '\0';
    strcpy(atomic_statement[2], statement[2]);
    strcpy(atomic_statement[3], "=");
    strcpy(atomic_statement[4], "Areg");
    dolet(atomic_statement);
    return; // bye-bye!
  }

  //check for [indexing]
  strcpy(Aregcopy, statement[4]);
  if (!strcmp(statement[4], Areg)) Aregmatch = 1; // do we already have the correct value in A?

  index |= getindex(statement[2], &getindex0[0]);
  index |= getindex(statement[4], &getindex1[0]) << 1;
  if (!CMATCH(5, ':'))
    index |= getindex(statement[6], &getindex2[0]) << 2;

  // check for array, e.g. x(1) to access bit 1 of x
  for (i = 3; i < STATEMENT_PART_SIZE; ++i) {
    if (statement[2][i] == '\0') { i = STATEMENT_PART_SIZE; break; }
    if (statement[2][i] == '}') break;
  }
  if (i < STATEMENT_PART_SIZE) { // found bit
    strcpy(Areg, "invalid");
    // extract expression in parentheses - for now just whole numbers allowed
    bit = (int)statement[2][i - 1] - '0';
    if (bit > 9 || bit < 0) {
      fprintf(stderr, "(%d) Error: variables in bit access not supported\n", line);
      exit(1);
    }
    if (bit > 7) {
      fprintf(stderr, "(%d) Error: invalid bit access\n", line);
      exit(1);
    }

    if (CMATCH(4, '0')) {
      printf("\tLDA ");
      for (i = 0; i < STATEMENT_PART_SIZE; ++i) {
        if (statement[2][i] == '{') break;
        printf("%c", statement[2][i]);
      }
      printf("\n");
      ap("AND #%d", 255 ^ _BV(bit));
    }
    else if (CMATCH(4, '1')) {
      printf("\tLDA ");
      for (i = 0; i < STATEMENT_PART_SIZE; ++i) {
        if (statement[2][i] == '{') break;
        printf("%c", statement[2][i]);
      }
      printf("\n");
      ap("ORA #%d", _BV(bit));
    }
    else if ((getbitvar = strtok(statement[4], "{"))) {
      // assign one bit to another
      // I haven't a clue if this will actually work!
      ap("LDA %s", getbitvar + (getbitvar[0] == '!' ? 1 : 0));
      ap("AND #%d", _BV((int)statement[4][strlen(getbitvar) + 1] - '0'));
      ap("PHP");
      printf("\tLDA ");
      for (i = 0; i < STATEMENT_PART_SIZE; ++i) {
        if (statement[2][i] == '{') break;
        printf("%c", statement[2][i]);
      }
      printf("\n");
      ap("PLP");
      ap(".byte $%c0, $03", getbitvar[0] == '!' ? 'F' : 'D'); // bad, bad way to do BEQ addr + 5

      ap("AND #%d", 255 ^ _BV(bit));
      ap(".byte $0C"); // NOP absolute to skip next two bytes
      ap("ORA #%d", _BV(bit));
    }
    else {
      fprintf(stderr, "(%d) Error: can only assign 0, 1 or another bit to a bit\n", line);
      exit(1);
    }
    printf("\tSTA ");
    for (i = 0; i < STATEMENT_PART_SIZE; ++i) {
      if (statement[2][i] == '{') break;
      printf("%c", statement[2][i]);
    }
    printf("\n");
    free(deallocstatement);
    return;
  }

  if (CMATCH(4, '-')) { // assignment to negative
    strcpy(Areg, "invalid");
    if (!fixpoint1 && isfixpoint(statement[5]) != 12) {
      if (statement[5][0] > '9') { // perhaps include constants too?
        ap("LDA #0");
        ap("SEC");
        ap("SBC %s", statement[5]);
      }
      else ap("LDA #%d", 256 - atoi(statement[5]));
    }
    else {
      if (fixpoint1 == 4) {
        if (statement[5][0] > '9') { // perhaps include constants too?
          ap("LDA #0");
          ap("SEC");
          ap("SBC %s", statement[5]);
        }
        else
          ap("LDA #%d", (int)(16.0 - atof(statement[5]) * 16.0));
        ap("STA %s", statement[2]);
        free(deallocstatement);
        return;
      }
      if (fixpoint1 == 8) {
        sprintf(statement[4], "%f", 256.0 - atof(statement[5]));
        ap("LDX %s", fracpart(statement[4]));
        ap("STX %s", fracpart(statement[2]));
        ap("LDA #%s", statement[4]);
        ap("STA %s", statement[2]);
        free(deallocstatement);
        return;
      }
    }
  }
  else if (SMATCH(4, "rand")) {
    strcpy(Areg, "invalid");
    if (optimization & _BV(OPTI_INLINE_RAND)) {
      ap("LDA rand");
      ap("LSR");
      printf(" ifconst rand16\n");
      ap("ROL rand16");
      printf(" endif\n");
      ap("BCC *+4");
      ap("EOR #$B4");
      ap("STA rand");
      printf(" ifconst rand16\n");
      ap("EOR rand16");
      printf(" endif\n");
    }
    else jsr("randomize");
  }
  else if (SMATCH(2, "score") && !SMATCH(2, "scorec")) {
    // break up into three parts
    strcpy(Areg, "invalid");
    if (CMATCH(5, '+')) {
      ap("SED");
      ap("CLC");
      for (i = 5; i >= 0; i--) {
        if (statement[6][i] != '\0') score[j] = number(statement[6][i]);
        score[j] = number(statement[6][i]);
        if (score[j] < 10 && score[j] >= 0) j++;
      }
      ap("LDA score+2");
      if (statement[6][0] >= '@')
        ap("ADC %s", statement[6]);
      else
        ap("ADC #$%d%d", score[1], score[0]);
      ap("STA score+2");
      ap("LDA score+1");
      if (score[0] > 9)
        ap("ADC #0");
      else
        ap("ADC #$%d%d", score[3], score[2]);
      ap("STA score+1");
      ap("LDA score");
      if (score[0] > 9)
        ap("ADC #0");
      else
        ap("ADC #$%d%d", score[5], score[4]);
      ap("STA score");
      ap("CLD");
    }
    else if (CMATCH(5, '-')) {
      ap("SED");
      ap("SEC");
      for (i = 5; i >= 0; i--) {
        if (statement[6][i] != '\0') score[j] = number(statement[6][i]);
        score[j] = number(statement[6][i]);
        if (score[j] < 10 && score[j] >= 0) j++;
      }
      ap("LDA score+2");
      if (score[0] > 9) ap("SBC %s", statement[6]);
      else ap("SBC #$%d%d", score[1], score[0]);
      ap("STA score+2");
      ap("LDA score+1");
      if (score[0] > 9) ap("SBC #0");
      else ap("SBC #$%d%d", score[3], score[2]);
      ap("STA score+1");
      ap("LDA score");
      if (score[0] > 9) ap("SBC #0");
      else ap("SBC #$%d%d", score[5], score[4]);
      ap("STA score");
      ap("CLD");
    }
    else {
      for (i = 5; i >= 0; i--) {
        if (statement[4][i] != '\0') score[j] = number(statement[4][i]);
        score[j] = number(statement[4][i]);
        if (score[j] < 10 && score[j] >= 0) j++;
      }
      ap("LDA #$%d%d", score[1], score[0]);
      ap("STA score+2");
      ap("LDA #$%d%d", score[3], score[2]);
      ap("STA score+1");
      ap("LDA #$%d%d", score[5], score[4]);
      ap("STA score");
    }
    free(deallocstatement);
    return;
  }
  else if (
       CMATCH(6, '1')
    && (statement[6][1] > '9' || statement[6][1] < '0')
    && (CMATCH(5, '+') || CMATCH(5, '-'))
    && !strncmp(statement[2], statement[4], STATEMENT_PART_SIZE)
    && !SMATCH(2, "Areg")
    && (statement[6][1] == '\0' || statement[6][1] == ' ' || statement[6][1] == '\n')
  ) { // var=var +/- something
    strcpy(Areg, "invalid");
    if (fixpoint1 == 4 && fixpoint2 == 4) {
      if (CMATCH(5, '+')) {
        ap("LDA %s", statement[2]);
        ap("CLC");
        ap("ADC #16");
        ap("STA %s", statement[2]);
        free(deallocstatement);
        return;
      }
      if (CMATCH(5, '-')) {
        ap("LDA %s", statement[2]);
        ap("SEC");
        ap("SBC #16");
        ap("STA %s", statement[2]);
        free(deallocstatement);
        return;
      }
    }

    if (index & 1) loadindex(&getindex0[0]);
    printf(CMATCH(5, '+') ? "\tINC " : "\tDEC ");
    if (!(index & 1))
      printf("%s\n", statement[2]);
    else
      printf("%s,x\n", statement[4]); // indexed with x!
    free(deallocstatement);

    return;
  }
  else { // This is generic x=num or var

    if (!Aregmatch) { // do we already have the correct value in A?
      if (index & 2) loadindex(&getindex1[0]);

      // if 8.8=8.8+8.8: this LDA will be superfluous - fix this at some point

      //if (!fixpoint1 && !fixpoint2 && !CMATCH(5, '('))
      if (((!fixpoint1 && !fixpoint2) || (!fixpoint1 && fixpoint2 == 8)) && !CMATCH(5, '('))

      //printfrac(statement[4]);
      //else
      {
        if (!SMATCH(4, "Areg"))
          ap("LDA %s", indexpart(statement[4], index & 2));
      }
      strcpy(Areg, Aregcopy);
    }
  }
  if (!CMATCH(5, '\0') && !CMATCH(5, ':')) {  // An operator was found
    fixpoint3 = isfixpoint(statement[6]);
    strcpy(Areg, "invalid");
    if (index & 4) loadindex(&getindex2[0]);
    if (CMATCH(5, '+')) {
      //if (fixpoint1 == 4 && fixpoint2 == 4) {
      //}
      //else {
        if (fixpoint1 == 8 && (fixpoint2 & 8) && (fixpoint3 & 8)) { //8.8=8.8+8.8
          ap("LDA %s", fracpart(statement[4]));
          ap("CLC ");
          ap("ADC %s", fracpart(statement[6]));
          ap("STA %s", fracpart(statement[2]));
          ap("LDA %s%s", immedtok(statement[4]), statement[4]);
          ap("ADC %s%s", immedtok(statement[6]), statement[6]);
        }
        else if (fixpoint1 == 8 && (fixpoint2 & 8) && fixpoint3 == 4) {
          ap("LDY %s", statement[6]);
          ap("LDX %s", fracpart(statement[4]));
          ap("LDA %s%s", immedtok(statement[4]), statement[4]);
          jsrbank1("Add44to88");
          ap("STX %s", fracpart(statement[2]));
        }
        else if (fixpoint1 == 4 && fixpoint2 == 8 && (fixpoint3 & 4)) {
          if (fixpoint3 == 4)
            ap("LDY %s", statement[6]);
          else
            ap("LDY #%d", (int)(atof(statement[6]) * 16.0));
          ap("LDA %s", statement[4]);
          ap("LDX %s", fracpart(statement[4]));
          jsrbank1("Add88to44");
        }
        else if (fixpoint1 == 4 && fixpoint2 == 4 && fixpoint3 == 12) {
          ap("CLC");
          ap("LDA %s", statement[4]);
          ap("ADC #%d", (int)(atof(statement[6]) * 16.0));
        }
        else if (fixpoint1 == 4 && fixpoint2 == 12 && fixpoint3 == 4) {
          ap("CLC");
          ap("LDA %s", statement[6]);
          ap("ADC #%d", (int)(atof(statement[4]) * 16.0));
        }
        else { // this needs work - 44+8+44 and probably others are screwy
          if (fixpoint2 == 4) ap("LDA %s", statement[4]);
          if (fixpoint3 == 4 && fixpoint2 == 0) {
            ap("LDA %s%s", immedtok(statement[4]), statement[4]); // this LDA might be superfluous
          }
          displayoperation("+CLC\n\tADC", statement[6], index & 4);
        }
     //}
    }
    else if (CMATCH(5, '-')) {
      if (fixpoint1 == 8 && (fixpoint2 & 8) && (fixpoint3 & 8)) { //8.8=8.8-8.8
        ap("LDA %s", fracpart(statement[4]));
        ap("SEC");
        ap("SBC %s", fracpart(statement[6]));
        ap("STA %s", fracpart(statement[2]));
        ap("LDA %s%s", immedtok(statement[4]), statement[4]);
        ap("SBC %s%s", immedtok(statement[6]), statement[6]);
      }
      else if (fixpoint1 == 8 && (fixpoint2 & 8) && fixpoint3 == 4) {
        ap("LDY %s", statement[6]);
        ap("LDX %s", fracpart(statement[4]));
        ap("LDA %s%s", immedtok(statement[4]), statement[4]);
        jsrbank1("Sub44from88");
        ap("STX %s", fracpart(statement[2]));
      }
      else if (fixpoint1 == 4 && fixpoint2 == 8 && (fixpoint3 & 4)) {
        if (fixpoint3 == 4)
          ap("LDY %s", statement[6]);
        else
          ap("LDY #%d", (int)(atof(statement[6]) * 16.0));
        ap("LDA %s", statement[4]);
        ap("LDX %s", fracpart(statement[4]));
        jsrbank1("Sub88from44");
      }
      else if (fixpoint1 == 4 && fixpoint2 == 4 && fixpoint3 == 12) {
        ap("SEC");
        ap("LDA %s", statement[4]);
        ap("SBC #%d", (int)(atof(statement[6]) * 16.0));
      }
      else if (fixpoint1 == 4 && fixpoint2 == 12 && fixpoint3 == 4) {
        ap("SEC");
        ap("LDA #%d", (int)(atof(statement[4]) * 16.0));
        ap("SBC %s", statement[6]);
      }
      else {
        if (fixpoint2 == 4) ap("LDA %s", statement[4]);
        if (fixpoint3 == 4 && fixpoint2 == 0) ap("LDA #%s", statement[4]);
        displayoperation("-SEC\n\tSBC", statement[6], index & 4);
      }
    }
    else if (CMATCH(5, '&')) {
      if (fixpoint2 == 4) ap("LDA %s", statement[4]);
      displayoperation("&AND", statement[6], index & 4);
    }
    else if (CMATCH(5, '^')) {
      if (fixpoint2 == 4) ap("LDA %s", statement[4]);
      displayoperation("^EOR", statement[6], index & 4);
    }
    else if (CMATCH(5, '|')) {
      if (fixpoint2 == 4) ap("LDA %s", statement[4]);
      displayoperation("|ORA", statement[6], index & 4);
    }
    else if (CMATCH(5, '*')) {
      if (fixpoint2 == 4) ap("LDA %s", statement[4]);
      if (!isimmed(statement[6]) || !checkmul(atoi(statement[6]))) {
        displayoperation("*LDY", statement[6], index & 4);
        jsrbank1(statement[5][1] == '*' ? "mul16" : "mul8"); // general mul routine
      }
      else {
        // attempt to optimize - may need to call mul anyway
        mul(statement, statement[5][1] == '*' ? 16 : 8);
      }
    }
    else if (CMATCH(5, '/')) {
      if (fixpoint2 == 4) ap("LDA %s", statement[4]);
      if (!isimmed(statement[6]) || !checkdiv(atoi(statement[6]))) {
        displayoperation("/LDY", statement[6], index & 4);
        jsrbank1(statement[5][1] == '/' ? "div16" : "div8"); // general div routine
      }
      else {
        // attempt to optimize - may need to call divd anyway
        divd(statement, statement[5][1] == '/' ? 16 : 8);
      }
    }
    else if (CMATCH(5, ':')) {
      strcpy(Areg, Aregcopy); // A reg is not invalid
    }
    else if (CMATCH(5, '(')) {
      // we've called a function, hopefully
      strcpy(Areg, "invalid");
      if (SMATCH(4, "sread"))
        sread(statement);
      else
        callfunction(statement);
    }
    else if (!CMATCH(4, '-')) { // if not unary -
      fprintf(stderr, "(%d) Error: invalid operator: %s\n", line, statement[5]);
      exit(1);
    }
  }
  else { // simple assignment
    // check for fixed point stuff here
    // bugfix: forgot the LDA (?) did I do this correctly???
    if (fixpoint1 == 4 && fixpoint2 == 0) {
      ap("LDA %s%s", immedtok(statement[4]), statement[4]);
      ap("ASL");
      ap("ASL");
      ap("ASL");
      ap("ASL");
    }
    else if (fixpoint1 == 0 && fixpoint2 == 4) {
      ap("LDA %s%s", immedtok(statement[4]), statement[4]);
      ap("LSR");
      ap("LSR");
      ap("LSR");
      ap("LSR");
    }
    else if (fixpoint1 == 4 && fixpoint2 == 8) {
      ap("LDA %s%s", immedtok(statement[4]), statement[4]);
      ap("LDX %s", fracpart(statement[4]));
      // note: this shouldn't be changed to jsr(); (why???)
      ap("JSR Assign88to44%s", bs ? "bs" : "");
    }
    else if (fixpoint1 == 8 && fixpoint2 == 4) {
      // note: this shouldn't be changed to jsr();
      ap("LDA %s%s", immedtok(statement[4]), statement[4]);
      ap("JSR Assign44to88%s", bs ? "bs" : "");
      ap("STX %s", fracpart(statement[2]));
    }
    else if (fixpoint1 == 8 && (fixpoint2 & 8)) {
      ap("LDX %s", fracpart(statement[4]));
      ap("STX %s", fracpart(statement[2]));
      ap("LDA %s%s", immedtok(statement[4]), statement[4]);
    }
    else if (fixpoint1 == 4 && (fixpoint2 & 4)) {
      if (fixpoint2 == 4)
        ap("LDA %s", statement[4]);
      else
        ap("LDA #%d", (int)(atof(statement[4]) * 16.0));
    }
    // else if (fixpoint1 == 0 && fixpoint1 == 8) {
    //   ap("LDA %s", statement[4]);
    // }
  }
  if (index & 1) loadindex(&getindex0[0]);
  if (!SMATCH(2, "Areg"))
    ap("STA %s", indexpart(statement[2], index & 1));
  free(deallocstatement);
}

void dogoto(char **statement) {
  int anotherbank = 0;
  if (SMATCH(3, "bank"))
    anotherbank = (int)(statement[3][4]) - '0';
  else {
    ap("JMP .%s", statement[2]);
    return;
  }

  // if here, we're jmp'ing to another bank
  // we need to switch banks
  ap("STA temp7");
  // next we must push the place to jmp to
  ap("LDA #>(.%s-1)", statement[2]);
  ap("PHA");
  ap("LDA #<(.%s-1)", statement[2]);
  ap("PHA");
  // now store regs on stack
  ap("LDA temp7");
  ap("PHA");
  ap("TXA");
  ap("PHA");
  // select bank to switch to
  ap("LDX #%d", anotherbank);
  ap("JMP BS_jsr"); // also works for jmps
}

void gosub(char **statement) {
  int anotherbank = 0;
  invalidate_Areg();
  //if (numgosubs++ > 3) {
  //  fprintf(stderr, "Max. nested gosubs exceeded in line %s\n", statement[0]);
  //  exit(1);
  //}
  if (SMATCH(3, "bank"))
    anotherbank = (int)(statement[3][4]) - '0';
  else {
    ap("JSR .%s", statement[2]);
    return;
  }

  // if here, we're jsr'ing to another bank
  // we need to switch banks
  ap("STA temp7");
  // first create virtual return address
  ap("LDA #>(ret_point%d-1)", ++numjsrs);
  ap("PHA");
  ap("LDA #<(ret_point%d-1)", numjsrs);
  ap("PHA");

  // next we must push the place to jsr to
  ap("LDA #>(.%s-1)", statement[2]);
  ap("PHA");
  ap("LDA #<(.%s-1)", statement[2]);
  ap("PHA");

  // now store regs on stack
  ap("LDA temp7");
  ap("PHA");
  ap("TXA");
  ap("PHA");

  // select bank to switch to
  ap("LDX #%d", anotherbank);
  ap("JMP BS_jsr");
  printf("ret_point%d\n", numjsrs);
}

void set(char **statement) {
  char **pass2const;
  int i;
  int valid_kernel_combos[] = {
    // C preprocessor should turn these into numbers!
    _BV(KERN_PLAYER1COLORS),
    _BV(KERN_NO_BLANK_LINES),
    _BV(KERN_PFCOLORS),
    _BV(KERN_PFHEIGHTS),
    _BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS),
    _BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS) | _BV(KERN_BACKGROUNDCHANGE),
    _BV(KERN_PFCOLORS) | _BV(KERN_NO_BLANK_LINES),
    _BV(KERN_PFCOLORS) | _BV(KERN_NO_BLANK_LINES) | _BV(KERN_BACKGROUNDCHANGE),
    _BV(KERN_PLAYER1COLORS) | _BV(KERN_NO_BLANK_LINES),
    _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFCOLORS),
    _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFHEIGHTS),
    _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS),
    _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFCOLORS) | _BV(KERN_BACKGROUNDCHANGE),
    _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFHEIGHTS) | _BV(KERN_BACKGROUNDCHANGE),
    _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS) | _BV(KERN_BACKGROUNDCHANGE),
    _BV(KERN_PLAYER1COLORS) | _BV(KERN_NO_BLANK_LINES) | _BV(KERN_READPADDLE),
    _BV(KERN_PLAYER1COLORS) | _BV(KERN_NO_BLANK_LINES) | _BV(KERN_PFCOLORS),
    _BV(KERN_PLAYER1COLORS) | _BV(KERN_NO_BLANK_LINES) | _BV(KERN_PFCOLORS) | _BV(KERN_BACKGROUNDCHANGE),
    _BV(KERN_PLAYERCOLORS) | _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFCOLORS),
    _BV(KERN_PLAYERCOLORS) | _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFHEIGHTS),
    _BV(KERN_PLAYERCOLORS) | _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS),
    _BV(KERN_PLAYERCOLORS) | _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFCOLORS) | _BV(KERN_BACKGROUNDCHANGE),
    _BV(KERN_PLAYERCOLORS) | _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFHEIGHTS) | _BV(KERN_BACKGROUNDCHANGE),
    _BV(KERN_PLAYERCOLORS) | _BV(KERN_PLAYER1COLORS) | _BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS) | _BV(KERN_BACKGROUNDCHANGE),
    _BV(KERN_NO_BLANK_LINES) | _BV(KERN_READPADDLE),
    255
  };
  pass2const = (char**)malloc(sizeof(char*) * 5);
  if (!strncasecmp(statement[2], "tv", 2)) {
    if (!strncasecmp(statement[3], "ntsc", 4)) {
      // pick constant timer values for now, later maybe add more lines
      strcpy(redefined_variables[numredefvars++], "overscan_time = 37");
      strcpy(redefined_variables[numredefvars++], "vblank_time = 43");
    }
    else if (!strncasecmp(statement[3], "pal", 3)) {
      // 36 and 48 scanlines, respectively
      strcpy(redefined_variables[numredefvars++], "overscan_time = 82");
      strcpy(redefined_variables[numredefvars++], "vblank_time = 57");
    }
    else prerror("set TV: invalid TV type\n");
  }
  else if (SMATCH(2, "smartbranching"))
    smartbranching = SMATCH(3, "on");
  else if (SMATCH(2, "romsize"))
    set_romsize(statement[3]);
  else if (!strncmp(statement[2], "optimization", 5)) { // Allow "optim"
    if (SMATCH(3, "speed"))
      optimization = _BV(OPTI_SPEED);
    else if (SMATCH(3, "size"))
      optimization = _BV(OPTI_SIZE);
    else if (SMATCH(3, "none"))
      optimization = 0;
    if (!strncmp(statement[3], "noinlinedata", 4))      // Allow "noin"
      optimization |= _BV(OPTI_NO_INLINE_DATA);
    if (!strncmp(statement[3], "inlinerand", 4))        // Allow "inli"
      optimization |= _BV(OPTI_INLINE_RAND);
  }
  else if (SMATCH(2, "kernal")) {
    prerror("The proper spelling is \"kernel\" - with an e.\n");
  }
  else if (!strncmp(statement[2], "kernel_options", 10)) { // Allow "kernel_opt"
    i = 3; kernel_options = 0;
    //fprintf(stderr, "matched 'kernel_options'\n");
    while (((unsigned char)statement[i][0] > (unsigned char)64) && ((unsigned char)statement[i][0] < (unsigned char)123)) {
      if (SMATCH(i, "readpaddle")) {
        strcpy(redefined_variables[numredefvars++], "readpaddle = 1");
        kernel_options |= _BV(KERN_READPADDLE);
      }
      else if (SMATCH(i, "player1colors")) {
        strcpy(redefined_variables[numredefvars++], "player1colors = 1");
        kernel_options |= _BV(KERN_PLAYER1COLORS);
        //fprintf(stderr, "matched 'player1colors'\n");
      }
      else if (SMATCH(i, "playercolors")) {
        strcpy(redefined_variables[numredefvars++], "playercolors = 1");
        strcpy(redefined_variables[numredefvars++], "player1colors = 1");
        kernel_options |= _BV(KERN_PLAYER1COLORS) | _BV(KERN_PLAYERCOLORS);
      }
      else if (!strncmp(statement[i], "no_blank_lines", 13)) {  // Allow "no_blank_line"
        strcpy(redefined_variables[numredefvars++], "no_blank_lines = 1");
        kernel_options |= _BV(KERN_NO_BLANK_LINES);
        //fprintf(stderr, "matched 'no_blank_lines'\n");
      }
      else if (!strncasecmp(statement[i], "pfcolors", 8))       // Allow any case (e.g., pFcOlOrS)
        kernel_options |= _BV(KERN_PFCOLORS);
      else if (!strncasecmp(statement[i], "pfheights", 9))      // Allow any case
        kernel_options |= _BV(KERN_PFHEIGHTS);
      else if (!strncasecmp(statement[i], "backgroundchange", 10)) { // Allow any case, and "background"
        strcpy(redefined_variables[numredefvars++], "backgroundchange = 1");
        kernel_options |= _BV(KERN_BACKGROUNDCHANGE);
      }
      else {
        prerror("set kernel_options: Options unknown or invalid\n");
        exit(1);
      }
      i++;
    }
    char *col_height_flag = NULL;
    switch (kernel_options & (_BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS))) {
      case _BV(KERN_PFCOLORS): col_height_flag = "PFcolors"; break;
      case _BV(KERN_PFHEIGHTS): col_height_flag = "PFheights"; break;
      case _BV(KERN_PFCOLORS) | _BV(KERN_PFHEIGHTS): col_height_flag = "PFcolorandheight"; break;
    }
    if (col_height_flag) sprintf(redefined_variables[numredefvars++], "%s = 1", col_height_flag);
    //fprintf(stderr, "kernel_options = %d\n", kernel_options);
    // check for valid combinations
    if (kernel_options == _BV(KERN_READPADDLE)) {
      prerror("set kernel_options: readpaddle must be used with no_blank_lines\n");
      exit(1);
    }
    i = 0;
    while (1) {
      if (valid_kernel_combos[i] == 255) {
        prerror("set kernel_options: Invalid combination of options\n");
        exit(1);
      }
      if (kernel_options == valid_kernel_combos[i++]) break;
    }
  }
  else if (SMATCH(2, "kernel")) {
    if (SMATCH(3, "multisprite")) {
      multisprite = true;
      strcpy(redefined_variables[numredefvars++], "multisprite = 1");
      create_includes("multisprite.inc");
      ROMpf = true;
    }
    else if (!strncmp(statement[3], "multisprite_no_include", 11)) { // Allow "multisprite"
      multisprite = true;
      strcpy(redefined_variables[numredefvars++], "multisprite = 1");
      ROMpf = true;
    }
    else prerror("set kernel: kernel name unknown or unspecified\n");
  }
  else if (SMATCH(2, "debug")) {
    if (SMATCH(3, "cyclescore"))
      strcpy(redefined_variables[numredefvars++], "debugscore = 1");
    else if (SMATCH(3, "cycles"))
      strcpy(redefined_variables[numredefvars++], "debugcycles = 1");
    else prerror("set debug: debugging mode unknown\n");
  }
  else if (SMATCH(2, "legacy"))
    sprintf(redefined_variables[numredefvars++], "legacy = %d", (int)(100 * atof(statement[3])));
  else prerror("set: unknown parameter\n");

}

void rem(char **statement) {
  if (SMATCH(2, "smartbranching"))
    smartbranching = SMATCH(3, "on");
}

void dopop() {
  ap("PLA");
  ap("PLA");
}

void branch(char *linenumber, const char *bra1, const char *bra2) {
  removeCR(linenumber);
  if (smartbranching) {
    printf(" if ((* - .%s) < 127) && ((* - .%s) > -128)\n", linenumber, linenumber);
    ap("B%s .%s", bra1, linenumber);
    // branches might be allowed as below - check carefully to make sure!
    // printf(" if ((* - .%s) < 127) && ((* - .%s) > -129)\n\tBMI .%s\n", linenumber, linenumber, linenumber);
    printf(" else\n");
    ap("B%s .%dskip%s", bra2, branchtargetnumber, linenumber);
    ap("JMP .%s", linenumber);
    printf(".%dskip%s\n", branchtargetnumber++, linenumber);
    printf(" endif\n");
  }
  else ap("B%s .%s", bra1, linenumber);
}
void bmi(char *linenumber) { branch(linenumber, "MI", "PL"); }
void bpl(char *linenumber) { branch(linenumber, "PL", "MI"); }
void bne(char *linenumber) { branch(linenumber, "NE", "EQ"); }
void beq(char *linenumber) { branch(linenumber, "EQ", "NE"); }
void bcc(char *linenumber) { branch(linenumber, "CC", "CS"); }
void bcs(char *linenumber) { branch(linenumber, "CS", "CC"); }
void bvc(char *linenumber) { branch(linenumber, "VC", "VS"); }
void bvs(char *linenumber) { branch(linenumber, "VS", "VC"); }

void drawscreen() {
  invalidate_Areg();
  jsr("drawscreen");
}

void prerror(char *myerror) {
  fprintf(stderr, "(%d): %s\n", line, myerror);
}

int number(unsigned char value) {
  return (int)value - '0';
}

void removeCR(char *linenumber) { // remove trailing CR from string
  char c;
  int len = strlen(linenumber);
  while (len && (c = linenumber[len - 1]) && (c == '\n' || c == '\r'))
    linenumber[--len] = '\0';
}

void remove_trailing_commas(char *linenumber) { // remove trailing commas from string
  for (int i = strlen(linenumber) - 1; i > 0; i--) {
    const char c = linenumber[i];
    if (c != ',' && c != ' ' && c != '\n' && c != '\r') break;
    if (c == ',') {
      linenumber[i] = ' ';
      break;
    }
  }
}

void header_open(FILE *header) { }

void header_write(FILE *header, char *filename) {
  if ((header = fopen(filename, "w")) == NULL) { // open file
    fprintf(stderr, "Cannot open %s for writing\n", filename);
    exit(1);
  }

  strcpy(redefined_variables[numredefvars], "; This file contains variable mapping and other information for the current project.\n");

  for (int i = numredefvars; i >= 0; i--)
    fprintf(header, "%s\n", redefined_variables[i]);

  fclose(header);
}
