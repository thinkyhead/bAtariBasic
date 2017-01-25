#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "statements.h"
#include "keywords.h"
#include <math.h>

int main(int argc, char *argv[]) {
  char **statement;
  int i,j,k;
  int count=0;
  int unnamed=0;
  char *c;
  char *cod;
  char single;
  char code[500];
  char displaycode[500];
  FILE *header;
  int multiplespace=0;
  const char *includes_file="default.inc";
  const char *filename="2600basic_variable_redefs.h";
  char *path=0;
    // get command line arguments
  while ((i=getopt(argc, argv, "i:r:")) != -1) {
  switch (i) {
    case 'i':
        path=(char *)malloc(500);
        path=optarg;
        break;
    case 'r':
        filename=(char *)malloc(100);
        //strcpy(filename, optarg);
        filename=optarg;
        break;
    case '?':
        fprintf(stderr,"usage: %s -r <variable redefs file> -i <includes path>\n",argv[0]);
        exit(1);
    }
  }
  condpart=0;
  last_bank=0; // last bank when bs is enabled (0 for 2k/4k)
  bank=1; // current bank: 1 or 2 for 8k
  bs=0; // bankswtiching type; 0=none
  superchip=0;
  optimization=0;
  smartbranching=0;
  line=0;
  numfixpoint44=0;
  numfixpoint88=0;
  ROMpf=0;
  ors=0;
  numjsrs=0;
  numfors=0;
  numthens=0;
  numelses=0;
  numredefvars=0;
  numconstants=0;
  branchtargetnumber=0;
  doingfunction=0;
  sprite_index=0;
  multisprite=0;
  lifekernel=0;
  playfield_number=0;
  playfield_index[0]=0;
  
  printf("game\n"); // label for start of game
  header_open(header);
  init_includes(path);

  statement=(char **)malloc(sizeof(char*)*50);
  for (i=0;i<50;++i) {
    statement[i]=(char*)malloc(sizeof(char)*50);
  }

  while (1) { // clear out statement cache
    for (i=0;i<50;++i) {
      for (j=0;j<50;++j) {
        statement[i][j]='\0';
      }
    }
    c=fgets(code, 500, stdin); // get next line from input
    incline();
    strcpy(displaycode,code);
    if (!c) break; //end of file

// preprocessing removed in favor of a simplistic lex-based preprocessor

    i=0;j=0;k=0;

// look for spaces, reject multiples
    while(code[i]!='\0') {
      single=code[i++];
      if (single == ' ') {
	if (!multiplespace)
	{
          j++;
	  k=0;
	}
	multiplespace++;
      }
      else
      {
	multiplespace=0;
	statement[j][k++]=single;
      }

    }
    if (j>40) fprintf(stderr,"(%d) Warning: long line\n",getline());
    if (statement[0][0]=='\0')
    {
      sprintf(statement[0],"L0%d",unnamed++);
    }
    if (strncmp(statement[0],"end\0",3))
      printf(".%s ; %s\n",statement[0],displaycode); //    printf(".%s ; %s\n",statement[0],code);

//for (i=0;i<10;++i)fprintf(stderr,"%s ",statement[i]);

    keywords(statement);
  }
  bank=bbank();
  bs=bbs();
//  if ((bank == 1) && (bs == 8)) newbank("2\n");
  barf_sprite_data();
  printf("       echo \"    \",[(scoretable - *)]d , \"bytes of ROM space left");
  if (bs == 8) printf(" in bank 2");
  if (bs == 16) printf(" in bank 4");
  if (bs == 32) printf(" in bank 8");
  printf("\")\n");
  printf(" \n");
  printf(" \n");
  printf(" \n");
  header_write(header, filename);
  create_includes(includes_file);
  fprintf(stderr,"2600 Basic compilation complete.\n");
//  freemem(statement);
  return 0;
}
