#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "statements.h"
#include "keywords.h"

void doreboot()
{
  printf("	JMP ($FFFC)\n");
}

int linenum()
{
  // returns current line number in source
  return line;
}

void vblank()
{
  // code that will be run in the vblank area
  // subject to limitations!
  // must exit with a return [thisbank if bankswitching used]
  printf("vblank_bB_code\n");
}

void pfclear(char **statement)
{
  char getindex0[50];
  int i,j,index=0;

  invalidate_Areg();

  if ((!statement[2][0]) || (statement[2][0] == ':')) printf("	LDA #0\n");
  else
  {
    index = getindex(statement[2],&getindex0[0]);
    if (index) loadindex(&getindex0[0]);
    printf("	LDA ");
    printindex(statement[2],index);
  }
  removeCR(statement[1]);
  jsr(statement[1]);

}

void playfieldcolorandheight(char **statement)
{
  char data[100];
  char rewritedata[100];
  int i=0,j=0;
  int pfpos=0,pfoffset=0,indexsave;
  // PF colors and/or heights
  // PFheights use offset of 21-31
  // PFcolors use offset of 84-124
  // if used together: playfieldblocksize-88, playfieldcolor-87
  if (!strncasecmp(statement[1],"pfheights:\0",9))
  {
    if ((kernel_options&48)==32)
    {
      sprintf(sprite_data[sprite_index++]," if (<*) > 223\n");
      sprintf(sprite_data[sprite_index++],"	repeat (277-<*)\n	.byte 0\n");
      sprintf(sprite_data[sprite_index++],"	repend\n	endif\n");
      sprintf(sprite_data[sprite_index],"pfcolorlabel%d\n",sprite_index);
      indexsave=sprite_index;
      sprite_index++;
      while (1) {
        if ( ((!fgets(data,100,stdin))
           || ( (data[0]<(unsigned char)0x3A) && (data[0]>(unsigned char)0x2F) )) 
              && (data[0] != 'e')) {

          prerror("Error: Missing \"end\" keyword at end of pfheight declaration\n");
          exit(1);
        }
        line++;
        if (!strncmp(data,"end\0",3)) break;
	removeCR(data);
        if (!pfpos) printf(" lda #%s\n sta playfieldpos\n",data);
        else sprintf(sprite_data[sprite_index++]," .byte %s\n",data);
	pfpos++;
      }
      printf(" lda #>(pfcolorlabel%d-21)\n",indexsave);
      printf(" sta pfcolortable+1\n");
      printf(" lda #<(pfcolorlabel%d-21)\n",indexsave);
      printf(" sta pfcolortable\n");
    }
    else if ((kernel_options&48)!=48) {prerror("PFheights kernel option not set");exit(1);}
    else //pf color and heights enabled 
    {
      sprintf(sprite_data[sprite_index++]," if (<*) > 206\n");
      sprintf(sprite_data[sprite_index++],"	align 256\n	endif\n");
      sprintf(sprite_data[sprite_index++]," if (<*) < 88\n");
      sprintf(sprite_data[sprite_index++],"	repeat (88-(<*))\n	.byte 0\n");
      sprintf(sprite_data[sprite_index++],"	repend\n	endif\n");
      sprintf(sprite_data[sprite_index++],"playfieldcolorandheight\n");
      pfcolorindexsave=sprite_index;
      while (1) {
        if ( ((!fgets(data,100,stdin))
           || ( (data[0]<(unsigned char)0x3A) && (data[0]>(unsigned char)0x2F) )) 
              && (data[0] != 'e')) {

          prerror("Error: Missing \"end\" keyword at end of pfheight declaration\n");
          exit(1);
        }
        line++;
        if (!strncmp(data,"end\0",3)) break;
	removeCR(data);
        if (!pfpos) printf(" lda #%s\n sta playfieldpos\n",data);
        else sprintf(sprite_data[sprite_index++]," .byte %s,0,0,0\n",data);
	pfpos++;
      }
    }
  }
  else // has to be pfcolors
  {
    if ((kernel_options&48)==16)
    {
      if (!pfcolornumber)
      {
        sprintf(sprite_data[sprite_index++]," if (<*) > 206\n");
        sprintf(sprite_data[sprite_index++],"	align 256\n	endif\n");
        sprintf(sprite_data[sprite_index++]," if (<*) < 88\n");
        sprintf(sprite_data[sprite_index++],"	repeat (88-(<*))\n	.byte 0\n");
        sprintf(sprite_data[sprite_index++],"	repend\n	endif\n");
	sprintf(sprite_data[sprite_index],"pfcolorlabel%d\n",sprite_index);
	sprite_index++;
      }
      //indexsave=sprite_index;
      pfoffset=1;
      while (1) {
        if ( ((!fgets(data,100,stdin))
           || ( (data[0]<(unsigned char)0x3A) && (data[0]>(unsigned char)0x2F) )) 
              && (data[0] != 'e')) {
          prerror("Error: Missing \"end\" keyword at end of pfcolor declaration\n");
          exit(1);
        }
        line++;
        if (!strncmp(data,"end\0",3)) break;
	removeCR(data);
        if (!pfpos)
	{
	  printf(" lda #%s\n sta COLUPF\n",data);
	  if (!pfcolornumber) pfcolorindexsave=sprite_index-1;
          pfcolornumber=(pfcolornumber+1)%4;
	  pfpos++;
	}
        else
	{
	  if (pfcolornumber!=1) // add to existing table
	  {
	    j=0;i=0;
	    while (j != (pfcolornumber+3)%4)
	    {
	      if (sprite_data[pfcolorindexsave+pfoffset][i++]==',') j++;
	      if (i>99) {prerror("Warning: size of subsequent pfcolor tables should match\n");break;}
	    }
//fprintf(stderr,"%s\n",sprite_data[pfcolorindexsave+pfoffset]);
	    strcpy(rewritedata,sprite_data[pfcolorindexsave+pfoffset]);
	    rewritedata[i-1]='\0';
	    if (i<100) sprintf(sprite_data[pfcolorindexsave+pfoffset++],"%s,%s%s",rewritedata,data,rewritedata+i+1);
	  }
	  else  // new table
	  {
	    sprintf(sprite_data[sprite_index++]," .byte %s,0,0,0\n",data);
	    // pad with zeros - later we can fill this with additional color data if defined
	  }
	}
      }
      printf(" lda #>(pfcolorlabel%d-%d)\n",pfcolorindexsave,85-pfcolornumber-((((pfcolornumber<<1)|(pfcolornumber<<2))^4)&4));
      printf(" sta pfcolortable+1\n");
      printf(" lda #<(pfcolorlabel%d-%d)\n",pfcolorindexsave,85-pfcolornumber-((((pfcolornumber<<1)|(pfcolornumber<<2))^4)&4));
//      printf(" lda #<(pfcolorlabel%d-%d)\n",pfcolorindexsave,85-pfcolornumber);
      printf(" sta pfcolortable\n");
    }
    else if ((kernel_options&48)!=48) {prerror("PFcolors kernel option not set");exit(1);}
    else 
    {  // pf color fixed table
      while (1) {
        if ( ((!fgets(data,100,stdin))
           || ( (data[0]<(unsigned char)0x3A) && (data[0]>(unsigned char)0x2F) )) 
              && (data[0] != 'e')) {
          prerror("Error: Missing \"end\" keyword at end of pfcolor declaration\n");
          exit(1);
        }
        line++;
        if (!strncmp(data,"end\0",3)) break;
	removeCR(data);

        if (!pfpos) printf(" lda #%s\n sta COLUPF\n",data);
        else
	{
 	  j=0;i=0;
	  while (!j)
	  {
	    if (sprite_data[pfcolorindexsave+pfoffset][i++]==',') j++;
	    if (i>99) {prerror("Warning: size of subsequent pfcolor tables should match\n");break;}
	  }
//fprintf(stderr,"%s\n",sprite_data[pfcolorindexsave+pfoffset]);
	  strcpy(rewritedata,sprite_data[pfcolorindexsave+pfoffset]);
	  rewritedata[i-1]='\0';
	  if (i<100) sprintf(sprite_data[pfcolorindexsave+pfoffset++],"%s,%s%s",rewritedata,data,rewritedata+i+1);
	}
	pfpos++;
      }

    }

  }

}

void playfield(char **statement)
{
 // for specifying a ROM playfield
  char zero='.';
  char one='X';
  int i,j,k,l=0;
  char data[100];
  char pframdata[50][100];
  if ((unsigned char)statement[2][0] > (unsigned char)0x20) zero=statement[2][0]; 
  if ((unsigned char)statement[3][0] > (unsigned char)0x20) one=statement[3][0]; 

// read data until we get an end
// stored in global var and output at end of code  

  while (1) {
    if ( ((!fgets(data,100,stdin))
       || ( (data[0]<(unsigned char)0x3A) && (data[0]>(unsigned char)0x2F) )) 
          && (data[0] != 'e')) {

      prerror("Error: Missing \"end\" keyword at end of playfield declaration\n");
      exit(1);
    }
    line++;
    if (!strncmp(data,"end\0",3)) break;
    if (ROMpf) // if playfield is in ROM:
    {
      pfdata[playfield_number][playfield_index[playfield_number]] = 0;
      for (i=0;i<32;++i)
      {
        if ((data[i] != zero) && (data[i] != one)) break;
        pfdata[playfield_number][playfield_index[playfield_number]] <<= 1;
        if (data[i] == one) pfdata[playfield_number][playfield_index[playfield_number]] |= 1;
      }
      playfield_index[playfield_number]++;
    }
    else
    {
      strcpy(pframdata[l++],data);
    }

  }



  //}

  if (ROMpf) // if playfield is in ROM:
  {
    printf("	LDA #<PF1_data%d\n",playfield_number);
    printf("	STA PF1pointer\n");
    printf("	LDA #>PF1_data%d\n",playfield_number);
    printf("	STA PF1pointer+1\n");

    printf("	LDA #<PF2_data%d\n",playfield_number);
    printf("	STA PF2pointer\n");
    printf("	LDA #>PF2_data%d\n",playfield_number);
    printf("	STA PF2pointer+1\n");
    playfield_number++;
  }
  else // RAM pf, as in std_kernel
  {
      printf("  ifconst pfres\n");
      printf("    ldx #4*pfres-1\n");
      printf("  else\n");
      printf("	  ldx #47\n");
      printf("  endif\n");
      printf("	jmp pflabel%d\n",playfield_number);

        // no need to align to page boundaries

      printf("PF_data%d\n",playfield_number);
      for (j=0;j<l;++j)  // stored right side up
      {
	printf("	.byte %%");
	// the below should be changed to check for zero instead of defaulting to it
	for(k=0;k<8;++k)
 	  if (pframdata[j][k]==one) printf("1"); else printf("0");
	printf(", %%");
	for(k=15;k>=8;k--)
	  if (pframdata[j][k]==one) printf("1"); else printf("0");
	printf(", %%");
	for(k=16;k<24;++k)
 	  if (pframdata[j][k]==one) printf("1"); else printf("0");
	printf(", %%");
	for(k=31;k>=24;k--)
	  if (pframdata[j][k]==one) printf("1"); else printf("0");
	printf("\n");
      }

      printf("pflabel%d\n",playfield_number);
      printf("	lda PF_data%d,x\n",playfield_number);
      if (superchip)
      {
//        printf("  ifconst pfres\n");
  //      printf("	sta playfield+48-pfres*4-128,x\n");
    //    printf("  else\n");
        printf("	sta playfield-128,x\n");
      //  printf("  endif\n");
      }
      else
      {
//        printf("  ifconst pfres\n");
  //      printf("	sta playfield+48-pfres*4,x\n");
    //    printf("  else\n");
        printf("	sta playfield,x\n");
      //  printf("  endif\n");
      }
      printf("	dex\n");
      printf("	bpl pflabel%d\n",playfield_number);
    playfield_number++;

  }

}

int bbank()
{
  return bank;
}

int bbs()
{
  return bs;
}

void jsr(char *location)
{
// determines whether to use the standard jsr (for 2k/4k or bankswitched stuff in current bank)
// or to switch banks before calling the routine
  if ((!bs) || (bank == last_bank))
  {  
    printf(" jsr %s\n",location);
    return;
  }
// we need to switch banks
  printf(" sta temp7\n");
// first create virtual return address
  printf(" lda #>(ret_point%d-1)\n",++numjsrs);
  printf(" pha\n");
  printf(" lda #<(ret_point%d-1)\n",numjsrs);
  printf(" pha\n");
// next we must push the place to jsr to
  printf(" lda #>(%s-1)\n",location);
  printf(" pha\n");
  printf(" lda #<(%s-1)\n",location);
  printf(" pha\n");
// now store regs on stack
  printf(" lda temp7\n");
  printf(" pha\n");
  printf(" txa\n");
  printf(" pha\n");
// select bank to switch to
  printf(" ldx #%d\n",last_bank);
  printf(" jmp BS_jsr\n");
  printf("ret_point%d\n",numjsrs); 

}


void jsrbank1(char *location)
{
// bankswitched jsr to bank 1
// determines whether to use the standard jsr (for 2k/4k or bankswitched stuff in current bank)
// or to switch banks before calling the routine
  if ((!bs) || (bank == 1))
  {  
    printf(" jsr %s\n",location);
    return;
  }
// we need to switch banks
  printf(" sta temp7\n");
// first create virtual return address
  printf(" lda #>(ret_point%d-1)\n",++numjsrs);
  printf(" pha\n");
  printf(" lda #<(ret_point%d-1)\n",numjsrs);
  printf(" pha\n");
// next we must push the place to jsr to
  printf(" lda #>(%s-1)\n",location);
  printf(" pha\n");
  printf(" lda #<(%s-1)\n",location);
  printf(" pha\n");
// now store regs on stack
  printf(" lda temp7\n");
  printf(" pha\n");
  printf(" txa\n");
  printf(" pha\n");
// select bank to switch to
  printf(" ldx #1\n");
  printf(" jmp BS_jsr\n");
  printf("ret_point%d\n",numjsrs); 

}

void switchjoy(char *input_source)
{
// place joystick/console switch reading code inline instead of as a subroutine
// standard routines needed for pretty much all games
// read switches, joysticks now compiler generated (more efficient)

//  invalidate_Areg()  // do we need this?

  if (!strncmp(input_source,"switchreset\0",11))
  {
     printf(" lda #1\n");
     printf(" bit SWCHB\n");
  }
  else if (!strncmp(input_source,"switchselect\0",12))
  {
     printf(" lda #2\n");
     printf(" bit SWCHB\n");
  }
  else if (!strncmp(input_source,"switchleftb\0",11))
  {
     printf(" lda #$40\n");
     printf(" bit SWCHB\n");
  }
  else if (!strncmp(input_source,"switchrightb\0",12))
  {
     printf(" lda #$80\n");
     printf(" bit SWCHB\n");
  }
  else if (!strncmp(input_source,"switchbw\0",8))
  {
     printf(" lda #8\n");
     printf(" bit SWCHB\n");
  }
  else if (!strncmp(input_source,"joy0up\0",6))
  {
     printf(" lda #$10\n");
     printf(" bit SWCHA\n");
  }
  else if (!strncmp(input_source,"joy0down\0",8))
  {
     printf(" lda #$20\n");
     printf(" bit SWCHA\n");
  }
  else if (!strncmp(input_source,"joy0left\0",8))
  {
     printf(" lda #$40\n");
     printf(" bit SWCHA\n");
  }
  else if (!strncmp(input_source,"joy0right\0",9))
  {
     printf(" lda #$80\n");
     printf(" bit SWCHA\n");
  }
  else if (!strncmp(input_source,"joy1up\0",6))
  {
     printf(" lda #1\n");
     printf(" bit SWCHA\n");
  }
  else if (!strncmp(input_source,"joy1down\0",8))
  {
     printf(" lda #2\n");
     printf(" bit SWCHA\n");
  }
  else if (!strncmp(input_source,"joy1left\0",8))
  {
     printf(" lda #4\n");
     printf(" bit SWCHA\n");
  }
  else if (!strncmp(input_source,"joy1right\0",9))
  {
     printf(" lda #8\n");
     printf(" bit SWCHA\n");
  }
  else if (!strncmp(input_source,"joy0fire\0",8))
  {
     printf(" lda #$80\n");
     printf(" bit INPT4\n");
  }
  else if (!strncmp(input_source,"joy1fire\0",8))
  {
     printf(" lda #$80\n");
     printf(" bit INPT5\n");
  }
  else prerror("invalid console switch/controller reference\n");
}

void newbank(int bankno)
{
  FILE *bs_support;
  char line[500];
  char fullpath[500];
  int i;
  int len;

  if (bankno == 1) return;  // "bank 1" is ignored

  fullpath[0]='\0';
  if (includespath)
  {
    strcpy(fullpath,includespath);
    if ((includespath[strlen(includespath)-1]=='\\') || (includespath[strlen(includespath)-1]=='/'))
      strcat(fullpath,"includes/");
    else
      strcat(fullpath,"/includes/");
  }
  strcat(fullpath,"banksw.asm");

// from here on, code will compile in the next bank
// for 8k bankswitching, most of the libaries will go into bank 2
// and majority of bB program in bank 1

  bank=bankno;
  if (bank > last_bank) prerror("bank not supported\n");

  
  printf(" echo \"    \",[(start_bank%d - *)]d , \"bytes of ROM space left in bank %d\")\n",bank-1,bank-1);


  // now display banksw.asm file

  if ((bs_support = fopen("banksw.asm", "r")) == NULL)  // open file in current dir
  {  
    if ((bs_support = fopen(fullpath, "r")) == NULL)  // open file
    {  
      fprintf(stderr,"Cannot open banksw.asm for reading\n");
      exit(1);
    }
  }  
  while (fgets(line,500,bs_support))
  {
    if (!strncmp(line,";size=",6)) break;
  }
  len=atoi(line+6);
  if (bank == 2) sprintf(redefined_variables[numredefvars++],"bscode_length = %d",len);

  printf(" ORG $%dFF4-bscode_length\n",bank-1);
  printf(" RORG $%XF4-bscode_length\n",(15-bs/2+2*(bank-1))*16+15);
  printf("start_bank%d",bank-1);

  while (fgets(line,500,bs_support))
  {
    if (line[0] == ' ') printf("%s",line);
  }

  fclose(bs_support);

  // startup vectors
  printf(" ORG $%dFFC\n",bank-1);
  printf(" RORG $%XFC\n",(15-bs/2+2*(bank-1))*16+15);
  printf(" .word start_bank%d\n",bank-1);
  printf(" .word start_bank%d\n",bank-1);

  // now end
  printf(" ORG $%d000\n",bank);
  printf(" RORG $%X00\n",(15-bs/2+2*bank)*16);
  if (superchip) printf(" repeat 256\n .byte $ff\n repend\n");

  if (bank == last_bank) printf("; bB.asm file is split here\n");

 // not working yet - need to :
  // do something I forgot

}

float immed_fixpoint(char *fixpointval)
{
  int i=findpoint(fixpointval);
  char decimalpart[50];
  fixpointval[i]='\0';
  sprintf(decimalpart,"0.%s",fixpointval+i+1);
  return atof(decimalpart);
}

int findpoint(char *item)
{
  int i;
  for (i=0;i<50;++i)
  {
    if (item[i] == '\0') return 50;
    if (item[i] == '.') return i;
  }
  return i;
}

void freemem(char **statement)
{
  int i;
  for (i=0;i<50;++i) free(statement[i]);
  free(statement);
}

void printfrac(char *item)
{ // prints the fractional part of a declared 8.8 fixpoint variable
  char getvar[50];
  int i;
  for (i=0;i<numfixpoint88;++i)
  {
    strcpy(getvar,fixpoint88[1][i]);
    if (!strcmp(fixpoint88[0][i],item)) 
    {
      printf("%s\n",getvar);
      return;
    }
  }
  // must be immediate value
  if (findpoint(item) < 50)
    printf("#%d\n",(int)(immed_fixpoint(item)*256.0));
  else printf("#0\n");
}

int isfixpoint(char *item)
{
  // determines if a variable is fixed point, and if so, what kind
  int i;
  removeCR(item);
  for (i=0;i<numfixpoint88;++i)
    if (!strcmp(item, fixpoint88[0][i])) return 8;
  for (i=0;i<numfixpoint44;++i)
    if (!strcmp(item, fixpoint44[0][i])) return 4;
  if (findpoint(item) < 50) return 12;
  return 0;
}

void set_romsize(char *size)
{
  if (!strncmp(size,"2k\0",2))
  {
    strcpy(redefined_variables[numredefvars++],"ROM2k = 1");
  }
  else if (!strncmp(size,"8k\0",2))
  {
    bs=8;
    last_bank=2;
    strcpy(redefined_variables[numredefvars++],"bankswitch_hotspot = $1FF8"); 
    strcpy(redefined_variables[numredefvars++],"bankswitch = 8");
    strcpy(redefined_variables[numredefvars++],"bs_mask = 1");
    if (!strncmp(size,"8kSC\0",4)) 
      {
        strcpy(redefined_variables[numredefvars++],"superchip = 1"); 
	create_includes("superchip.inc");
	superchip=1;
      }
    else
      create_includes("bankswitch.inc");
  }
  else if (!strncmp(size,"16k\0",2))
  {
    bs=16;
    last_bank=4;
    strcpy(redefined_variables[numredefvars++],"bankswitch_hotspot = $1FF6"); 
    strcpy(redefined_variables[numredefvars++],"bankswitch = 16");
    strcpy(redefined_variables[numredefvars++],"bs_mask = 3");
    if (!strncmp(size,"16kSC\0",5)) 
      {
        strcpy(redefined_variables[numredefvars++],"superchip = 1"); 
	create_includes("superchip.inc");
	superchip=1;
      }
    else
      create_includes("bankswitch.inc");
  }
  else if (!strncmp(size,"32k\0",2))
  {
    bs=32;
    last_bank=8;
    strcpy(redefined_variables[numredefvars++],"bankswitch_hotspot = $1FF4"); 
    strcpy(redefined_variables[numredefvars++],"bankswitch = 32");
    strcpy(redefined_variables[numredefvars++],"bs_mask = 7");
//    if (multisprite == 1) create_includes("multisprite_bankswitch.inc");
   // else
    if (!strncmp(size,"32kSC\0",5)) 
      {
        strcpy(redefined_variables[numredefvars++],"superchip = 1"); 
	create_includes("superchip.inc");
	superchip=1;
      }
    else
      create_includes("bankswitch.inc");
  }
  else if (strncmp(size,"4k\0",2))
  {
    fprintf(stderr,"Warning: unsupported ROM size: %s Using 4k.\n",size);
  }
}

void add_includes(char *myinclude)
{
  strcat(user_includes,myinclude);
}

void add_inline(char *myinclude)
{
  printf(" include %s\n",myinclude);
}

void init_includes(char *path)
{
  if (path) strcpy(includespath,path);
  else includespath[0]='\0';
  user_includes[0]='\0';
}

void barf_sprite_data()
{
  int i,j,k;
// go to the last bank before barfing the graphics
  if (!bank) bank=1;
  for (i=bank;i<last_bank;++i) newbank(i+1);
//  {
//    bank=1;
//    printf(" echo \"   \",[(start_bank1 - *)]d , \"bytes of ROM space left in bank 1\"\n");
//  }
//  for (i=bank;i<last_bank;++i)
//    printf("; bB.asm file is split here\n\n\n\n");
  for (i=0;i<sprite_index;++i)
  {
    printf("%s",sprite_data[i]);
  }

  // now we must regurgitate the PF data

  for (i=0;i<playfield_number;++i)
  {
    if (ROMpf)
    {
      printf(" if ((>(*+%d)) > (>*))\n ALIGN 256\n endif\n",playfield_index[i]);
      printf("PF1_data%d\n",i);
      for (j=playfield_index[i]-1;j>=0;j--)
      {
        printf(" .byte %%");

        for (k=15;k>7;k--)
        {
	  if (pfdata[i][j] & (1 << k)) printf("1"); else printf("0");
        }
        printf("\n");
      }

      printf(" if ((>(*+%d)) > (>*))\n ALIGN 256\n endif\n",playfield_index[i]);
      printf("PF2_data%d\n",i);
      for (j=playfield_index[i]-1;j>=0;j--)
      {
        printf(" .byte %%");
        for (k=0;k<8;++k) // reversed bit order!
        {
 	  if (pfdata[i][j] & (1 << k)) printf("1"); else printf("0");
        }
        printf("\n");
      }
    }
    else // RAM pf
    {
 // do nuttin'
    }
  }
}

void create_includes(char *includesfile)
{
  FILE *includesread,*includeswrite;
  char line[500];
  char fullpath[500];
  int i;
  int writeline;
  removeCR(includesfile);
  if (includesfile_already_done) return;
  includesfile_already_done=1;
  fullpath[0]='\0';
  if (includespath)
  {
    strcpy(fullpath,includespath);
    if ((includespath[strlen(includespath)-1]=='\\') || (includespath[strlen(includespath)-1]=='/'))
      strcat(fullpath,"includes/");
    else
      strcat(fullpath,"/includes/");
  }
  strcat(fullpath,includesfile);
//  for (i=0;i<strlen(includesfile);++i) if (includesfile[i]=='\n') includesfile[i]='\0';
  if ((includesread = fopen(includesfile, "r")) == NULL)  // try again in current dir
  {  
    if ((includesread = fopen(fullpath, "r")) == NULL)  // open file
    {  
      fprintf(stderr,"Cannot open %s for reading\n",includesfile);
      exit(1);
    }
  }  
  if ((includeswrite = fopen("includes.bB", "w")) == NULL)  // open file
  {  
    fprintf(stderr,"Cannot open includes.bB for writing\n");
    exit(1);
  }  
  
  while (fgets(line,500,includesread))
  {
    for (i=0;i<500;++i)
    {
      if (line[i]==';') 
      {
	writeline=0;
	break;
      }
      if (line[i]==(unsigned char)0x0A) 
      {
	writeline=0;
	break;
      }
      if (line[i]==(unsigned char)0x0D) 
      {
	writeline=0;
	break;
      }
      if (line[i]>(unsigned char)0x20) 
      {
	writeline=1;
	break;
      }
      writeline=0;
    }
    if (writeline) 
    {
      if (!strncasecmp(line,"bb.asm\0",6)) 
	if (user_includes[0] != '\0') fprintf(includeswrite,user_includes);
      fprintf(includeswrite,line);
    }
  }
  fclose(includesread);
  fclose(includeswrite);
}

void printindex(char *mystatement, int myindex)
{
  if (!myindex)
  {
    printimmed(mystatement);
    printf("%s\n",mystatement);
  }
  else printf("%s,x\n",mystatement); // indexed with x!
}

void loadindex(char *myindex)
{
  printf("	LDX "); // get index
  printimmed(myindex);
  printf("%s\n",myindex);
}

int getindex(char *mystatement, char *myindex)
{
  int i,j,index=0;
  for (i=0;i<50;++i) 
  {
    if (mystatement[i]=='\0') {i=50;break;}
    if (mystatement[i]=='[') {index=1;break;}
  }
  if (i<50)
  {
    strcpy(myindex,mystatement+i+1);
    myindex[strlen(myindex)-1]='\0';
    if (myindex[strlen(myindex)-2]==']')
      myindex[strlen(myindex)-2]='\0';
    if (myindex[strlen(myindex)-1]==']')
      myindex[strlen(myindex)-1]='\0';
    for (j=i;j<50;++j) mystatement[j]='\0';
  }
  return index;
}

int checkmul(int value)
{
// check to see if value can be optimized to save cycles
  if (value < 11) return 1;  // always optimize these
  if (!(value % 2)) return 1;

  while (value!=1)
  {
    if (!(value%9)) value/=9;
    else if (!(value%7)) value/=7;
    else if (!(value%5)) value/=5;
    else if (!(value%3)) value/=3;
    else if (!(value%2)) value/=2;
    else break;
    if (!(value%2) && (optimization&1) != 1) break; // do not optimize multplications
  }
  if (value == 1) return 1; else return 0;
}

int checkdiv(int value)
{
// check to see if value is a power of two - if not, run standard div routine
  while (value!=1)
  {
    if (value%2) break;
    else value/=2;
  }
  if (value == 1) return 1; else return 0;
}


void mul(char **statement, int bits)
{
  // this will attempt to output optimized code depending on the multiplicand
  int multiplicand=atoi(statement[6]);
  int tempstorage=0;
  // we will optimize specifically for 2,3,5,7,9
  if (bits == 16)
  {
    printf("	ldx #0\n");
    printf("	stx temp1\n");
  }
  while (multiplicand != 1)
  {
    if (!(multiplicand % 9))
    {
      if (tempstorage) 
      {
	strcpy(statement[4],"temp2");
	printf("	sta temp2\n");
      }
      multiplicand /= 9;
      printf("	asl\n");
      if (bits == 16) printf("  rol temp1\n");
      printf("	asl\n");
      if (bits == 16) printf("  rol temp1\n");
      printf("	asl\n");
      if (bits == 16) printf("  rol temp1\n");
      printf("	clc\n");
      printf("	adc ");
      printimmed(statement[4]);
      printf("%s\n",statement[4]);
      if (bits == 16)
      {
        printf("	tax\n");
        printf("	lda temp1\n");
        printf("	adc #0\n");
        printf("	sta temp1\n");
        printf("	txa\n");
      }
      tempstorage=1;
    }
    else if (!(multiplicand % 7))
    {
      if (tempstorage) 
      {
	strcpy(statement[4],"temp2");
	printf("	sta temp2\n");
      }
      multiplicand /= 7;
      printf("	asl\n");
      if (bits == 16) printf("  rol temp1\n");
      printf("	asl\n");
      if (bits == 16) printf("  rol temp1\n");
      printf("	asl\n");
      if (bits == 16) printf("  rol temp1\n");
      printf("	sec\n");
      printf("	sbc ");
      printimmed(statement[4]);
      printf("%s\n",statement[4]);
      if (bits == 16)
      {
        printf("	tax\n");
        printf("	lda temp1\n");
        printf("	sbc #0\n");
        printf("	sta temp1\n");
        printf("	txa\n");
      }
      tempstorage=1;
    }
    else if (!(multiplicand % 5))
    {
      if (tempstorage) 
      {
	strcpy(statement[4],"temp2");
	printf("	sta temp2\n");
      }
      multiplicand /= 5;
      printf("	asl\n");
      if (bits == 16) printf("  rol temp1\n");
      printf("	asl\n");
      if (bits == 16) printf("  rol temp1\n");
      printf("	clc\n");
      printf("	adc ");
      printimmed(statement[4]);
      printf("%s\n",statement[4]);
      if (bits == 16)
      {
        printf("	tax\n");
        printf("	lda temp1\n");
        printf("	adc #0\n");
        printf("	sta temp1\n");
        printf("	txa\n");
      }
      tempstorage=1;
    }
    else if (!(multiplicand % 3))
    {
      if (tempstorage) 
      {
	strcpy(statement[4],"temp2");
	printf("	sta temp2\n");
      }
      multiplicand /= 3;
      printf("	asl\n");
      if (bits == 16) printf("  rol temp1\n");
      printf("	clc\n");
      printf("	adc ");
      printimmed(statement[4]);
      printf("%s\n",statement[4]);
      if (bits == 16)
      {
        printf("	tax\n");
        printf("	lda temp1\n");
        printf("	adc #0\n");
        printf("	sta temp1\n");
        printf("	txa\n");
      }
      tempstorage=1;
    }
    else if (!(multiplicand % 2))
    {
      multiplicand /= 2;
      printf("	asl\n");
      if (bits == 16) printf("  rol temp1\n");
    }
    else 
    {
      printf("	LDY #%d\n",multiplicand);
      printf("	jsr mul%d\n",bits);
      fprintf(stderr,"Warning - there seems to be a problem.  Your code may not run properly.\n");
      fprintf(stderr,"If you are seeing this message, please report it - it could be a bug.\n");
// WARNING: not fixed up for bankswitching

    }
  }
}

void divd(char **statement, int bits)
{
  int divisor=atoi(statement[6]);
  if (bits == 16)
  {
    printf("	ldx #0\n");
    printf("	stx temp1\n");
  }
  while (divisor != 1)
  {
    if (!(divisor % 2)) // div by power of two is the only easy thing
    {
      divisor /= 2;
      printf("	lsr\n");
      if (bits == 16) printf("  rol temp1\n");  // I am not sure if this is actually correct
    }
    else 
    {
      printf("	LDY #%d\n",divisor);
      printf("	jsr div%d\n",bits);
      fprintf(stderr,"Warning - there seems to be a problem.  Your code may not run properly.\n");
      fprintf(stderr,"If you are seeing this message, please report it - it could be a bug.\n");
// WARNING: Not fixed up for bankswitching

    }
  }
  
}

void endfunction()
{
  if (!doingfunction) prerror("extraneous end keyword encountered\n");
  doingfunction=0;
}

void function(char **statement)
{
  // declares a function - only needed if function is in bB.  For asm functions, see
  // the help.html file.
  // determine number of args, then run until we get an end. 
  doingfunction=1;
  printf("%s\n",statement[2]);
  printf("	STA temp1\n");
  printf("	STY temp2\n");
}

void callfunction(char **statement)
{
  // called by assignment to a variable
  // does not work as an argument in another function or an if-then... yet.
  int i,j,index=0;
  char getindex0[50];
  int arguments=0;
  int argnum[10];
  for (i=6;statement[i][0]!=')';++i)
  {
    if (statement[i][0] != ',') 
    {
      argnum[arguments++]=i;
    }
    if (i > 45) prerror("missing \")\" at end of function call\n");
  }
  if (!arguments) fprintf(stderr,"(%d) Warning: function call with no arguments\n",line);
  while (arguments)
  {
    // get [index]
    index=0;
    index |= getindex(statement[argnum[--arguments]],&getindex0[0]);
    if (index) loadindex(&getindex0[0]);

    if (arguments == 1) printf("	LDY ");
    else printf("	LDA ");
    printindex(statement[argnum[arguments]],index);

    if (arguments>1) printf("	STA temp%d\n",arguments+1);
//    arguments--;
  }
//  jsr(statement[4]);
// need to fix above for proper function support
  printf(" jsr %s\n",statement[4]);

  strcpy(Areg,"invalid");

}

void incline()
{
  line++;
}

int getline()
{
  return line;
}

void invalidate_Areg()
{
  strcpy(Areg,"invalid");
}

int islabel(char **statement)
{ // this is for determining if the item after a "then" is a label or a statement
  // return of 0=label, 1=statement
  int i;
  // find the "then" or a "goto"
  for (i=0;i<48;) if (!strncmp(statement[i++],"then\0",4)) break;
  return findlabel(statement,i);
}

int islabelelse(char **statement)
{ // this is for determining if the item after an "else" is a label or a statement
  // return of 0=label, 1=statement
  int i;
  // find the "else"
  for (i=0;i<48;) if (!strncmp(statement[i++],"else\0",4)) break;
  return findlabel(statement,i);
}

int findlabel(char **statement, int i)
{
 // 0 if label, 1 if not
  if ((statement[i][0]>(unsigned char)0x2F) && (statement[i][0]<(unsigned char)0x3B)) return 0;
  if ((statement[i+1][0]==':') && (strncmp(statement[i+2],"rem\0",3))) return 1;
  if ((statement[i+1][0]==':') && (!strncmp(statement[i+2],"rem\0",3))) return 0;

  if (!strncmp(statement[i+1],"else\0",4)) return 0;
//  if (!strncmp(statement[i+1],"bank\0",4)) return 0;
// uncomment to allow then label bankx (needs work)
  if (statement[i+1][0]!='\0') return 1;
  // only possibilities left are: drawscreen, player0:, player1:, asm, next, return, maybe others added later?
  if (!strncmp(statement[i],"drawscreen\0",10)) return 1;
  if (!strncmp(statement[i],"lives:\0",6)) return 1;
  if (!strncmp(statement[i],"player0color:\0",13)) return 1;
  if (!strncmp(statement[i],"player1color:\0",13)) return 1;
  if (!strncmp(statement[i],"player0:\0",8)) return 1;
  if (!strncmp(statement[i],"player1:\0",8)) return 1;
  if (!strncmp(statement[i],"player2:\0",8)) return 1;
  if (!strncmp(statement[i],"player3:\0",8)) return 1;
  if (!strncmp(statement[i],"player4:\0",8)) return 1;
  if (!strncmp(statement[i],"player5:\0",8)) return 1;
  if (!strncmp(statement[i],"playfield:\0",10)) return 1;
  if (!strncmp(statement[i],"pfcolors:\0",9)) return 1;
  if (!strncmp(statement[i],"pfheights:\0",10)) return 1;
  if (!strncmp(statement[i],"asm\0",3)) return 1;
  if (!strncmp(statement[i],"rem\0",3)) return 1;
  if (!strncmp(statement[i],"next\0",4)) return 1;
  if (!strncmp(statement[i],"reboot\0",6)) return 1;
  if (!strncmp(statement[i],"return\0",6)) return 1;
  return 0; // I really hope we've got a label !!!!
}

void sread(char **statement)
{
  // read sequential data
  printf("	ldx #%s\n",statement[6]);
  printf("	lda (0,x)\n");
  printf("	inc 0,x\n");
  printf("	bne *+4\n");
  printf("	inc 1,x\n");
  strcpy(Areg,"invalid");
}

void sdata(char **statement)
{
 // sequential data, works like regular basics and doesn't have the 256 byte restriction
  char data[200];
  int i;
  removeCR(statement[4]);
  sprintf(redefined_variables[numredefvars++],"%s = %s",statement[2],statement[4]);
  printf("	lda #<%s_begin\n",statement[2]);
  printf("	sta %s\n",statement[4]); 
  printf("	lda #>%s_begin\n",statement[2]);
  printf("	sta %s+1\n",statement[4]); 

  printf("	JMP .skip%s\n",statement[0]);
 // not affected by noinlinedata

  printf("%s_begin\n",statement[2]);
  while (1) {
    if ( ((!fgets(data,200,stdin))
       || ( (data[0]<(unsigned char)0x3A) && (data[0]>(unsigned char)0x2F) )) 
          && (data[0] != 'e')) {
      prerror("Error: Missing \"end\" keyword at end of data\n");
      exit(1);
    }
    line++;
    if (!strncmp(data,"end\0",3)) break;
    remove_trailing_commas(data);
    for (i=0;i<200;++i)
    {
      if ((int)data[i]>32) break;
      if ((int)data[i]<14) i=200;
    }
    if (i<200) printf("	.byte %s\n",data);
  }
  printf(".skip%s\n",statement[0]);

}

void data(char **statement)
{
  char data[200];
  char **data_length;
  char **deallocdata_length;
  int i;
  data_length=(char **)malloc(sizeof(char *)*50);
  for (i=0;i<50;++i) data_length[i]=(char*)malloc(sizeof(char)*50);
  deallocdata_length=data_length;
  removeCR(statement[2]);

  if (!(optimization&4)) printf("	JMP .skip%s\n",statement[0]);
  // if optimization level >=4 then data cannot be placed inline with code!

  printf("%s\n",statement[2]);
  while (1) {
    if ( ((!fgets(data,200,stdin))
       || ( (data[0]<(unsigned char)0x3A) && (data[0]>(unsigned char)0x2F) )) 
          && (data[0] != 'e')) {
      prerror("Error: Missing \"end\" keyword at end of data\n");
      exit(1);
    }
    line++;
    if (!strncmp(data,"end\0",3)) break;
    remove_trailing_commas(data);
    for (i=0;i<200;++i)
    {
      if ((int)data[i]>32) break;
      if ((int)data[i]<14) i=200;
    }
    if (i<200) printf("	.byte %s\n",data);
  }
  printf(".skip%s\n",statement[0]);
  strcpy(data_length[0]," ");
  strcpy(data_length[1],"const");
  sprintf(data_length[2],"%s_length",statement[2]);
  strcpy(data_length[3],"=");
  sprintf(data_length[4],".skip%s-%s",statement[0],statement[2]);
  strcpy(data_length[5],"\n");
  data_length[6][0]='\0';
  keywords(data_length);
  freemem(deallocdata_length);
}

void shiftdata(char **statement, int num)
{
  int i,j;
  for (i=49;i>num;i--)
    for (j=0;j<50;++j)
      statement[i][j]=statement[i-1][j];
}

void compressdata(char **statement, int num1, int num2)
{
  int i,j;
  for (i=num1;i<50-num2;i++)
    for (j=0;j<50;++j)
      statement[i][j]=statement[i+num2][j];
}

void ongoto(char **statement)
{
// warning!!! bankswitching not yet supported
  int k,i=4;
  if (strcmp(statement[2],Areg)) printf("	LDX %s\n",statement[2]);
  //printf("	ASL\n");
  //printf("	TAX\n");
  printf("	LDA .%sjumptablehi,x\n",statement[0]);
  printf("	PHA\n");
  //printf("	INX\n");
  printf("	LDA .%sjumptablelo,x\n",statement[0]);
  printf("	PHA\n");
  printf("	RTS\n");
  printf(".%sjumptablehi\n",statement[0]);
  while ((statement[i][0]!=':') && (statement[i][0]!='\0'))
  {
    for (k=0;k<100;++k)
      if ((statement[i][k]==(unsigned char)0x0A) || (statement[i][k]==(unsigned char)0x0D))
	statement[i][k]='\0';
    printf("	.byte >(.%s-1)\n",statement[i++]);
  }
  printf(".%sjumptablelo\n",statement[0]);
  i=4;
  while ((statement[i][0]!=':') && (statement[i][0]!='\0'))
  {
    for (k=0;k<100;++k)
      if ((statement[i][k]==(unsigned char)0x0A) || (statement[i][k]==(unsigned char)0x0D))
	statement[i][k]='\0';
    printf("	.byte <(.%s-1)\n",statement[i++]);
  }
}

void dofor(char **statement)
{
  if (strcmp(statement[4],Areg)) 
  {
    printf("	LDA ");
    printimmed(statement[4]);
    printf("%s\n",statement[4]);
  }
  
  printf("	STA %s\n",statement[2]);
  
  forlabel[numfors][0]='\0';
  sprintf(forlabel[numfors],"%sfor%s",statement[0],statement[2]);
  printf(".%s\n",forlabel[numfors]);

  forend[numfors][0]='\0';
  strcpy(forend[numfors],statement[6]);

  forvar[numfors][0]='\0';
  strcpy(forvar[numfors],statement[2]);

  forstep[numfors][0]='\0';

  if (!strncasecmp(statement[7],"step\0",4)) strcpy(forstep[numfors],statement[8]);
  else strcpy(forstep[numfors],"1");

  numfors++;
}

void next(char **statement)
{
  int immed=0;
  int immedend=0;
  int failsafe=0;
  char failsafelabel[60];

  invalidate_Areg();

  if (!numfors) {
    fprintf(stderr,"(%d) next without for\n",line);
    exit(1);
  }
  numfors--;
  if (!strncmp(forstep[numfors],"1\0",2)) // step 1
  {
    printf("	LDA %s\n",forvar[numfors]);
    printf("	CMP ");
    printimmed(forend[numfors]);
    printf("%s\n",forend[numfors]);
    printf("	INC %s\n",forvar[numfors]);
    bcc(forlabel[numfors]);
  }
  else if ((!strncmp(forstep[numfors],"-1\0",3)) || (!strncmp(forstep[numfors],"255\0",4)))
  { // step -1
    printf("	DEC %s\n",forvar[numfors]);
    if (strncmp(forend[numfors],"1\0",2))
    {
      printf("	LDA %s\n",forvar[numfors]);
      printf("	CMP ");
      printimmed(forend[numfors]);
      printf("%s\n",forend[numfors]);
      bcs(forlabel[numfors]);    
    }
    else
      bne(forlabel[numfors]);    
  }
  else // step other than 1 or -1
  {
    // normally, the generated code adds to or subtracts from the for variable, and checks
    // to see if it's less than the ending value.
    // however, if the step would make the variable less than zero or more than 255 
    // then this check will not work.  The compiler will attempt to detect this situation
    // if the step and end are known.  If the step and end are not known (that is, 
    // either is a variable) then much more complex code must be generated.

    printf("	LDA %s\n",forvar[numfors]);
    printf("	CLC\n");
    printf("	ADC ");
    immed=printimmed(forstep[numfors]);
    printf("%s\n",forstep[numfors]);

    if (immed && isimmed(forend[numfors])) // the step and end are immediate
    {
      if (atoi(forstep[numfors])&128) // step is negative
      {
	if ((256-(atoi(forstep[numfors])&255)) >= atoi(forend[numfors]))
	{ // if we are in danger of going < 0...we will have carry clear after ADC
	  failsafe=1;
	  sprintf(failsafelabel,"%s_failsafe",forlabel[numfors]);
	  bcc(failsafelabel);
	}
      }
      else { // step is positive
	if ((atoi(forstep[numfors]) + atoi(forend[numfors])) > 255)
	{ // if we are in danger of going > 255...we will have carry set after ADC
	  failsafe=1;
	  sprintf(failsafelabel,"%s_failsafe",forlabel[numfors]);
	  bcs(failsafelabel);
	}
      }

    }
    printf("	STA %s\n",forvar[numfors]);

    printf("	CMP ");
    immedend=printimmed(forend[numfors]);
    // add 1 to immediate compare for increasing loops
    if (immedend && !(atoi(forstep[numfors])&128))
      strcat(forend[numfors],"+1");
    printf("%s\n",forend[numfors]);
// if step number is 1 to 127 then add 1 and use bcc, otherwise bcs
// if step is a variable, we'll need to check for every loop iteration
//
// Warning! no failsafe checks with variables as step or end - it's the
// programmer's job to make sure the end value doesn't overflow
    if (!immed) 
    {
      printf("	LDX %s\n",forstep[numfors]);
      printf("	BMI .%sbcs\n",statement[0]);
      bcc(forlabel[numfors]);
      printf("	CLC\n");
      printf(".%sbcs\n",statement[0]);
      bcs(forlabel[numfors]);
    }
    else if (atoi(forstep[numfors])&128) bcs(forlabel[numfors]);
    else
    {
      bcc(forlabel[numfors]);
      if (!immedend) beq(forlabel[numfors]);
    }
  }
  if (failsafe) printf(".%s\n",failsafelabel);
}

void dim(char **statement)
{
  // just take the statement and pass it right to a header file
  int i;
  char *fixpointvar1;
  char fixpointvar2[50];
  // check for fixedpoint variables
  i=findpoint(statement[4]);
  if (i<50)
  {
    removeCR(statement[4]);
    strcpy(fixpointvar2,statement[4]);
    fixpointvar1=fixpointvar2+i+1; 
    fixpointvar2[i]='\0'; 
    if (!strcmp(fixpointvar1,fixpointvar2)) // we have 4.4
      {
	strcpy(fixpoint44[1][numfixpoint44],fixpointvar1);
	strcpy(fixpoint44[0][numfixpoint44++],statement[2]);
      }
    else // we have 8.8
      {
	strcpy(fixpoint88[1][numfixpoint88],fixpointvar1);
	strcpy(fixpoint88[0][numfixpoint88++],statement[2]);
      }
    statement[4][i]='\0'; // terminate string at '.'
  }
  i=2;
  redefined_variables[numredefvars][0]='\0';
  while (statement[i][0] != '\0') 
  {
    strcat(redefined_variables[numredefvars],statement[i++]);
    strcat(redefined_variables[numredefvars]," ");
  }
  numredefvars++;
}

void doconst(char **statement)
{
  // basically the same as dim, except we keep a queue of variable names that are constant
  int i=2;
  redefined_variables[numredefvars][0]='\0';
  while (statement[i][0] != '\0')
  {
    strcat(redefined_variables[numredefvars],statement[i++]);
    strcat(redefined_variables[numredefvars]," ");
  }
  numredefvars++;
  strcpy(constants[numconstants++],statement[2]); // record to queue
}

void doreturn(char **statement)
{
  int i,j,index=0;
  char getindex0[50];
  int bankedreturn=0;
  // 0=no special action
  // 1=return thisbank
  // 2=return otherbank

  if (!strncmp(statement[2],"thisbank\0",8) ||
      !strncmp(statement[3],"thisbank\0",8)) bankedreturn=1;
  else if (!strncmp(statement[2],"otherbank\0",9) ||
      !strncmp(statement[3],"otherbank\0",9)) bankedreturn=2;

  // several types of returns:
  // return by itself (or with a value) can return to any calling bank
  // this one has the most overhead in terms of cycles and ROM space
  // use sparingly if cycles are space are an issue

  // return [value] thisbank will only return within the same bank
  // this one is the fastest

  // return [value] otherbank will slow down returns to the same bank
  // but speed up returns to other banks - use if you are primarily returning
  // across banks

  if (statement[2][0] && (statement[2][0]!=' ') && 
      (strncmp(statement[2],"thisbank\0",8)) &&
      (strncmp(statement[2],"otherbank\0",9)))
  {

    index |= getindex(statement[2],&getindex0[0]);

    if (index&1) loadindex(&getindex0[0]);

    if (!bankedreturn) printf("	LDY ");
    else printf("	LDA ");
    printindex(statement[2],index&1);
    
  }

  if (bankedreturn == 1)
  {
    printf("	RTS\n");
    return;
  }
  if (bankedreturn == 2)
  {
    printf("	JMP BS_return\n");
    return;
  }

  if (bs) // check if sub was called from the same bank
  {
    printf("	tsx\n");
    printf("	lda 2,x ; check return address\n");
    printf("	eor #(>*) ; vs. current PCH\n");
    printf("	and #$E0 ;  mask off all but top 3 bits\n");
    // if zero, then banks are the same
    if (statement[2][0] && (statement[2][0]!=' ')) 
    {
      printf("	beq *+6 ; if equal, do normal return\n");
      printf("	tya\n");
    }
    else printf("	beq *+5 ; if equal, do normal return\n");
    printf("	JMP BS_return\n");
  }

  if (statement[2][0] && (statement[2][0]!=' ')) printf("	tya\n");
  printf("	RTS\n");
}

void pfread(char **statement)
{
  char getindex0[50];
  char getindex1[50];
  int i,j,index=0;

  invalidate_Areg();

  index |= getindex(statement[3],&getindex0[0]);
  index |= getindex(statement[4],&getindex1[0])<<1;

  if (index&1) loadindex(&getindex0[0]);

  printf("	LDA ");
  printindex(statement[4],index&1);

  if (index&2) loadindex(&getindex1[0]);

  printf("	LDY ");
  printindex(statement[6],index&2);

  jsr("pfread");
}

void pfpixel(char **statement)
{
  char getindex0[50];
  char getindex1[50];
  int i,j,index=0;
  if (multisprite==1) {prerror("Error: pfpixel not supported in multisprite kernel\n");exit(1);}
  invalidate_Areg();

  index |= getindex(statement[2],&getindex0[0]);
  index |= getindex(statement[3],&getindex1[0])<<1;

  if (index&1) loadindex(&getindex0[0]);

  printf("	LDA ");
  printindex(statement[2],index&1);

  if (index&2) loadindex(&getindex1[0]);

  printf("	LDY ");
  printindex(statement[3],index&2);

  printf("	LDX #");
  if (!strncmp(statement[4],"flip",2)) printf("2\n");
  else if (!strncmp(statement[4],"off",2)) printf("1\n");
  else printf("0\n");
  jsr("pfpixel");
}

void pfhline(char **statement)
{
  char getindex0[50];
  char getindex1[50];
  char getindex2[50];
  int i,j,index=0;
  if (multisprite==1) {prerror("Error: pfhline not supported in multisprite kernel\n");exit(1);}

  invalidate_Areg();

  index |= getindex(statement[2],&getindex0[0]);
  index |= getindex(statement[3],&getindex1[0])<<1;
  index |= getindex(statement[4],&getindex2[0])<<2;

  if (index&4) loadindex(&getindex2[0]);
  printf("	LDA ");
  printindex(statement[4],index&4);
  printf("	STA temp3\n");

  if (index&1) loadindex(&getindex0[0]);
  printf("	LDA ");
  printindex(statement[2],index&1);

  if (index&2) loadindex(&getindex1[0]);
  printf("	LDY ");
  printindex(statement[3],index&2);

  printf("	LDX #");
  if (!strncmp(statement[5],"flip",2)) printf("2\n");
  else if (!strncmp(statement[5],"off",2)) printf("1\n");
  else printf("0\n");

  jsr("pfhline");
}

void pfvline(char **statement)
{
  char getindex0[50];
  char getindex1[50];
  char getindex2[50];
  int i,j,index=0;
  if (multisprite==1) {prerror("Error: pfvline not supported in multisprite kernel\n");exit(1);}

  invalidate_Areg();

  index |= getindex(statement[2],&getindex0[0]);
  index |= getindex(statement[3],&getindex1[0])<<1;
  index |= getindex(statement[4],&getindex2[0])<<2;

  if (index&4) loadindex(&getindex2[0]);
  printf("	LDA ");
  printindex(statement[4],index&4);
  printf("	STA temp3\n");

  if (index&1) loadindex(&getindex0[0]);
  printf("	LDA ");
  printindex(statement[2],index&1);

  if (index&2) loadindex(&getindex1[0]);
  printf("	LDY ");
  printindex(statement[3],index&2);

  printf("	LDX #");
  if (!strncmp(statement[5],"flip",2)) printf("2\n");
  else if (!strncmp(statement[5],"off",2)) printf("1\n");
  else printf("0\n");

  jsr("pfvline");
}

void pfscroll(char **statement)
{
  if (multisprite==1) {prerror("Error: pfscroll support not yet implemented in multisprite kernel\n");exit(1);}
  invalidate_Areg();

  printf("	LDA #");
  if (!strncmp(statement[2],"left",2)) printf("0\n");
  else if (!strncmp(statement[2],"right",2)) printf("1\n");
  else if (!strncmp(statement[2],"upup\0",4)) printf("6\n");
  else if (!strncmp(statement[2],"downdown",6)) printf("8\n");
  else if (!strncmp(statement[2],"up\0",2)) printf("2\n");
  else if (!strncmp(statement[2],"down",2)) printf("4\n");
  else {fprintf(stderr,"(%d) pfscroll direction unknown\n",line);exit(1);}

  jsr("pfscroll");
}

void doasm()
{
  char data[100];
  while (1) {
    if ( ((!fgets(data,100,stdin))
       || ( (data[0]<(unsigned char)0x3A) && (data[0]>(unsigned char)0x2F) )) 
          && (data[0] != 'e')) {
      prerror("Error: Missing \"end\" keyword at end of inline asm\n");
      exit(1);
    }
    line++;
    if (!strncmp(data,"end\0",3)) break;
    printf("%s\n",data);  

  }
}

void player(char **statement)
{
  int height=0,i=0; //calculating sprite height
  int doingcolor=0; //doing player colors?
  char label[50];
  char data[100];
//  char bytes[1000];
//  char byte[100];
  char pl=statement[1][6];
  if (statement[1][7] == 'c') doingcolor=1;
//  bytes[0]='\0';
  int heightrecord;
  if (!doingcolor) sprintf(label,"player%s_%c\n",statement[0],pl);
  else sprintf(label,"playercolor%s_%c\n",statement[0],pl);
  
  printf("	LDA #<%s\n",label);
  if (!doingcolor) printf("	STA player%cpointerlo\n",pl);
  else printf("	STA player%ccolor\n",pl);

  printf("	LDA #>%s\n",label);
  if (!doingcolor) printf("	STA player%cpointerhi\n",pl);
  else printf("	STA player%ccolor+1\n",pl);


  //printf("	JMP .%sjump%c\n",statement[0],pl);
 
  // insert DASM stuff to prevent page-wrapping of player data
  // stick this in a data file instead of displaying



  heightrecord=sprite_index++;
  sprite_index+=2;
  // record index for creation of the line below

  if (multisprite == 1)
  {
    sprintf(sprite_data[sprite_index++]," if (<*) < 100\n");
    sprintf(sprite_data[sprite_index++],"	repeat (100-<*)\n	.byte 0\n");
    sprintf(sprite_data[sprite_index++],"	repend\n	endif\n");
  }

  sprintf(sprite_data[sprite_index++],"%s\n",label);
  sprintf(sprite_data[sprite_index++],"	.byte 0\n");

  while (1) {
    if ( ((!fgets(data,100,stdin))
       || ( (data[0]<(unsigned char)0x3A) && (data[0]>(unsigned char)0x2F) )) 
          && (data[0] != 'e')) {

      prerror("Error: Missing \"end\" keyword at end of player declaration\n");
      exit(1);
    }
    line++;
    if (!strncmp(data,"end\0",3)) break;
    height++;
    sprintf(sprite_data[sprite_index++],"	.byte %s",data);  
    //strcat(bytes,byte);
 
  }

// record height
  sprintf(sprite_data[heightrecord]," if (<*) > (<(*+%d))\n",height+1);
  sprintf(sprite_data[heightrecord+1],"	repeat ($100-<*)\n	.byte 0\n");
  sprintf(sprite_data[heightrecord+2],"	repend\n	endif\n");

//  printf(".%sjump%c\n",statement[0],pl);
  if (multisprite == 1) printf("	LDA #%d\n",height+2);
  else if (!doingcolor) printf("	LDA #%d\n",height);
  if (!doingcolor) printf("	STA player%cheight\n",pl);
}

void lives(char **statement)
{
  int height=8,i=0; //height is always 8
  char label[50];
  char data[100];
  if (!lifekernel) 
  {
    lifekernel=1;
    //strcpy(redefined_variables[numredefvars++],"lifekernel = 1"); 
  }
  
  sprintf(label,"lives__%s\n",statement[0]);
 removeCR(label);  

  printf("	LDA #<%s\n",label);
  printf("	STA lifepointer\n");

  printf("	LDA lifepointer+1\n");
  printf("	AND #$E0\n");
  printf("	ORA #(>%s)&($1F)\n",label);
  printf("	STA lifepointer+1\n");
  
  sprintf(sprite_data[sprite_index++]," if (<*) > (<(*+8))\n");
  sprintf(sprite_data[sprite_index++],"	repeat ($100-<*)\n	.byte 0\n");
  sprintf(sprite_data[sprite_index++],"	repend\n	endif\n");

  sprintf(sprite_data[sprite_index++],"%s\n",label);

  for (i=0;i<9;++i) {
    if ( ((!fgets(data,100,stdin))
       || ( (data[0]<(unsigned char)0x3A) && (data[0]>(unsigned char)0x2F) )) 
          && (data[0] != 'e')) {

      prerror("Error: Not enough data or missing \"end\" keyword at end of lives declaration\n");
      exit(1);
    }
    line++;
    if (!strncmp(data,"end\0",3)) break;
    sprintf(sprite_data[sprite_index++],"	.byte %s",data);  
  }
}

void doif(char **statement)
{
  int index=0;
  char getindex0[50];
  char getindex1[50];
  int not=0;
  int i,j,k;
  int bit=0;
  int Aregmatch=0;
  char Aregcopy[50];
  char **cstatement; //conditional statement
  char **dealloccstatement; //for deallocation
  
  strcpy(Aregcopy,"index-invalid");

  cstatement = (char **)malloc(sizeof(char*)*50);
  for (k=0;k<50;++k) cstatement[k]=(char*)malloc(sizeof(char)*50);
  dealloccstatement=cstatement;
  for (k=0;k<50;++k) for (j=0;j<50;++j) cstatement[j][k]='\0';
  if (statement[2][0] == '!') {
    not=1;
    for (i=0;i<49;++i) {
      statement[2][i]=statement[2][i+1];
    }
  }
  if ( (!strncmp(statement[2],"joy\0",3)) 
   || (!strncmp(statement[2],"switch\0",6)) ) {
    switchjoy(statement[2]);
    if (!islabel(statement)) {
      if (not) bne(statement[4]); else beq(statement[4]);
//printf("	BNE "); else printf("	BEQ ");
//      printf(".%s\n",statement[4]);
      freemem(dealloccstatement);
      return;
    }
    else // then statement
    {
      if (not) printf("	BEQ "); else printf("	BNE ");

      printf(".skip%s\n",statement[0]);
      // separate statement
      for (i=3;i<50;++i)
      {
        for (k=0;k<50;++k) {
          cstatement[i-3][k]=statement[i][k];
	}
      }
      printf(".condpart%d\n",condpart++);
      keywords(cstatement);
      printf(".skip%s\n",statement[0]);
 
      freemem(dealloccstatement);
      return;      
    }
  }

  if (!strncmp(statement[2],"pfread\0",6)) { 
    pfread(statement);
    if (!islabel(statement)) {
      if (not) bne(statement[9]); else beq(statement[9]);
      freemem(dealloccstatement);
      return;
    }
    else // then statement
    {
      if (not) printf("	BEQ "); else printf("	BNE ");

      printf(".skip%s\n",statement[0]);
      // separate statement
      for (i=8;i<50;++i)
      {
        for (k=0;k<50;++k) {
          cstatement[i-8][k]=statement[i][k];
	}
      }
      printf(".condpart%d\n",condpart++);
      keywords(cstatement);
      printf(".skip%s\n",statement[0]);
      freemem(dealloccstatement);
      return;      
    }
  }

  if  (!strncmp(statement[2],"collision(\0",10)) {
    if (!strncmp(statement[2],"collision(missile0,player1)\0",27)) {
      printf("	BIT CXM0P\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(missile0,player0)\0",27)) {
      printf("	BIT CXM0P\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(missile1,player0)\0",27)) {
      printf("	BIT CXM1P\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(missile1,player1)\0",27)) {
      printf("	BIT CXM1P\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(player0,playfield)\0",28)) {
      printf("	BIT CXP0FB\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(player0,ball)\0",23)) {
      printf("	BIT CXP0FB\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(player1,playfield)\0",28)) {
      printf("	BIT CXP1FB\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(player1,ball)\0",23)) {
      printf("	BIT CXP1FB\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(missile0,playfield)\0",29)) {
      printf("	BIT CXM0FB\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(missile0,ball)\0",24)) {
      printf("	BIT CXM0FB\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(missile1,playfield)\0",29)) {
      printf("	BIT CXM1FB\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(missile1,ball)\0",24)) {
      printf("	BIT CXM1FB\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(ball,playfield)\0",25)) {
      printf("	BIT CXBLPF\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(player0,player1)\0",26)) {
      printf("	BIT CXPPMM\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(missile0,missile1)\0",28)) {
      printf("	BIT CXPPMM\n");
      bit=6;
    } // now repeat everything in reverse...


    else if (!strncmp(statement[2],"collision(player1,missile0)\0",27)) {
      printf("	BIT CXM0P\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(player0,missile0)\0",27)) {
      printf("	BIT CXM0P\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(player0,missile1)\0",27)) {
      printf("	BIT CXM1P\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(player1,missile1)\0",27)) {
      printf("	BIT CXM1P\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(playfield,player0)\0",28)) {
      printf("	BIT CXP0FB\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(ball,player0)\0",23)) {
      printf("	BIT CXP0FB\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(playfield,player1)\0",28)) {
      printf("	BIT CXP1FB\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(ball,player1)\0",23)) {
      printf("	BIT CXP1FB\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(playfield,missile0)\0",29)) {
      printf("	BIT CXM0FB\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(ball,missile0)\0",24)) {
      printf("	BIT CXM0FB\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(playfield,missile1)\0",29)) {
      printf("	BIT CXM1FB\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(ball,missile1)\0",24)) {
      printf("	BIT CXM1FB\n");
      bit=6;
    }
    else if (!strncmp(statement[2],"collision(playfield,ball)\0",25)) {
      printf("	BIT CXBLPF\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(player1,player0)\0",26)) {
      printf("	BIT CXPPMM\n");
      bit=7;
    }
    else if (!strncmp(statement[2],"collision(missile1,missile0)\0",28)) {
      printf("	BIT CXPPMM\n");
      bit=6;
    }



    else //error
    {
      fprintf(stderr, "(%d) Error: Unknown collision type: %s\n",line,statement[2]+9);
      exit(1);
    }


    if (!islabel(statement)) {
      if (!not) {if (bit==7) bmi(statement[4]); else bvs(statement[4]);}
      else {if (bit==7) bpl(statement[4]); else bvc(statement[4]);}
//{if (bit==7) printf("	BMI "); else printf("	BVS ");}
//      else {if (bit==7) printf("	BPL "); else printf("	BVC ");}
//      printf(".%s\n",statement[4]);
      freemem(dealloccstatement);
      return;
    }
    else // then statement
    {
      if (not) {if (bit==7) printf("	BMI "); else printf("	BVS ");}
      else {if (bit==7) printf("	BPL "); else printf("	BVC ");}

      printf(".skip%s\n",statement[0]);
      // separate statement
      for (i=3;i<50;++i)
      {
        for (k=0;k<50;++k) {
          cstatement[i-3][k]=statement[i][k];
	}
      }
      printf(".condpart%d\n",condpart++);
      keywords(cstatement);
      printf(".skip%s\n",statement[0]);
 
      freemem(dealloccstatement);
      return;      
    }
  }


  // check for array, e.g. x{1} to get bit 1 of x
  for (i=0;i<50;++i) 
  {
    if (statement[2][i]=='\0') {i=50;break;}
    if (statement[2][i]=='}') break;
  }
  if (i<50) // found array
  {
    // extract expression in parantheses - for now just whole numbers allowed
    bit=(int)statement[2][i-1]-48;
    if ((bit>9)||(bit<0))
    {
      fprintf(stderr,"(%d) Error: variables in bit access not supported\n",line);
      exit(1);
    }
    if ((bit==7) || (bit==6)) // if bit 6 or 7, we can use BIT and save 2 bytes
    {
      printf("	BIT ");
      for (i=0;i<50;++i) 
      {
	if (statement[2][i]=='{') break;
	printf("%c",statement[2][i]);
      }
      printf("\n");
      if (!islabel(statement)) {
        if (!not) {if (bit==7) bmi(statement[4]); else bvs(statement[4]);}
        else {if (bit==7) bpl(statement[4]); else bvc(statement[4]);}
        freemem(dealloccstatement);
        return;
      }
      else // then statement
      {
        if (not) {if (bit==7) printf("	BMI "); else printf("	BVS ");}
        else {if (bit==7) printf("	BPL "); else printf("	BVC ");}

        printf(".skip%s\n",statement[0]);
        // separate statement
        for (i=3;i<50;++i)
        {
          for (k=0;k<50;++k) {
            cstatement[i-3][k]=statement[i][k];
  	  }
        }
        printf(".condpart%d\n",condpart++);
        keywords(cstatement);
        printf(".skip%s\n",statement[0]);
 
        freemem(dealloccstatement);
        return;      
      }
    }
    else
    { 
      Aregmatch=0;
      printf("	LDA ");
      for (i=0;i<50;++i) 
      {
	if (statement[2][i]=='{') break;
	printf("%c",statement[2][i]);
      }
      printf("\n");
      if (!bit)      // if bit 0, we can use LSR and save a byte
	printf("	LSR\n");
      else printf("	AND #%d\n",1<<bit);//(int)pow(2,bit));
      if (!islabel(statement)) {
        if (not) {if (!bit) bcc(statement[4]); else beq(statement[4]);}
        else {if (!bit) bcs(statement[4]); else bne(statement[4]);}
        freemem(dealloccstatement);
        return;
      }
      else // then statement
      {
        if (not) {if (!bit) printf("	BCS "); else printf("	BNE ");}
        else {if (!bit) printf("	BCC "); else printf("	BEQ ");}

        printf(".skip%s\n",statement[0]);
        // separate statement
        for (i=3;i<50;++i)
        {
          for (k=0;k<50;++k) {
            cstatement[i-3][k]=statement[i][k];
  	  }
        }
        printf(".condpart%d\n",condpart++);
        keywords(cstatement);
        printf(".skip%s\n",statement[0]);
 
        freemem(dealloccstatement);
        return;      
      }



    }

  }
  
// generic if-then (no special considerations)
  //check for [indexing]
  strcpy(Aregcopy,statement[2]);
  if (!strcmp(statement[2],Areg)) Aregmatch=1; // do we already have the correct value in A?

  index |= getindex(statement[2],&getindex0[0]);
  if (strncmp(statement[3],"then\0",4))
    index |= getindex(statement[4],&getindex1[0])<<1;

    if (!Aregmatch) // do we already have the correct value in A?
    {
      if (index&1) loadindex(&getindex0[0]);
      printf("	LDA ");
      printindex(statement[2],index&1);
      strcpy(Areg,Aregcopy);
    }
    if (index&2) loadindex(&getindex1[0]);
//todo:check for cmp #0--useless except for <, > to clear carry
  if (strncmp(statement[3],"then\0",4))
  {
    printf("	CMP ");
    printindex(statement[4],index&2);
  }

  if (!islabel(statement)){ // then linenumber
    if (statement[3][0] == '=') beq(statement[6]);
    if (!strncmp(statement[3],"<>\0",2)) bne(statement[6]);
    else if (statement[3][0] == '<') bcc(statement[6]);
    if (statement[3][0] == '>') bcs(statement[6]);
    if (!strncmp(statement[3],"then\0",4)) 
      if (not) beq(statement[4]); else bne(statement[4]);

  }
  else // then statement
  { // first, take negative of condition and branch around statement
    if (statement[3][0] == '=') printf ("     BNE ");
    if (!strcmp(statement[3],"<>")) printf ("     BEQ ");
    else if (statement[3][0] == '<') printf ("     BCS ");
    if (statement[3][0] == '>') printf ("     BCC ");
    j=5;

    if (!strncmp(statement[3],"then\0",4))
    {
      j=3; 
      if (not) printf("	BNE "); else printf("	BEQ ");
    }
    printf(".skip%s\n",statement[0]);
    // separate statement

      // separate statement
      for (i=j;i<50;++i)
      {
        for (k=0;k<50;++k) {
          cstatement[i-j][k]=statement[i][k];
	}
      }
      printf(".condpart%d\n",condpart++);
      keywords(cstatement);
      printf(".skip%s\n",statement[0]);
 
      freemem(dealloccstatement);
      return;      


//    i=4;
//    while (statement[i][0]!='\0')
//    {
//      cstatement[i-4]=statement[i++];
//    }
//    keywords(cstatement);
//    printf(".skip%s\n",statement[0]);
  }
  freemem(dealloccstatement);
}

int getcondpart()
{
  return condpart;
}

int orderofoperations(char op1, char op2)
{
// specify order of operations for complex equations
// i.e.: parens, divmul (*/), +-, logical (^&|)
 if (op1 == '(') return 0;
 else if (op2 == '(') return 0;
 else if (op2 == ')') return 1;
 else if ( ((op1 == '^') || (op1 == '|') || (op1 == '&'))
         && ((op2 == '^') || (op2 == '|') || (op2 == '&')))
   return 0;
// else if (op1 == '^') return 1;
// else if (op1 == '&') return 1;
// else if (op1 == '|') return 1;
// else if (op2 == '^') return 0;
// else if (op2 == '&') return 0;
// else if (op2 == '|') return 0;
 else if ((op1 == '*') || (op1 == '/')) return 1;
 else if ((op2 == '*') || (op2 == '/')) return 0;
 else if ((op1 == '+') || (op1 == '-')) return 1;
 else if ((op2 == '+') || (op2 == '-')) return 0;
 else return 1;
}

int isoperator(char op)
{
  if (!((op == '+') || (op == '-') || (op == '/') ||
     (op == '*') || (op == '&') || (op == '|') ||
     (op == '^') || (op == ')') || (op == '(')))
    return 0;
  else return 1;
}

void displayoperation(char *opcode, char *operand, int index)
{
  if (!strncmp(operand,"stackpull\0",9))
  {
    if (opcode[0] == '-')
    {
      // operands swapped 
      printf("	TAY\n");
      printf("	PLA\n");
      printf("	TSX\n");
      printf("	STY $00,x\n",opcode+1);
      printf("	SEC\n");
      printf("	SBC $100,x\n",opcode+1);
    }
    else if (opcode[0] == '/')
    {
      // operands swapped
      printf("	TAY\n");
      printf("	PLA\n");
    }
    else
    {
      printf("	TSX\n");
      printf("	INX\n");
      printf("	TXS\n");
      printf("	%s $100,x\n",opcode+1);
    }
  }
  else
  {
    printf("	%s ",opcode+1);
    printindex(operand,index);
  }
}


void let(char **cstatement)
{
  int i,j=0,bit=0,k;
  int index=0;
  int immediate=0;
  char getindex0[50];
  char getindex1[50];
  char getindex2[50];
  int score[6]={0,0,0,0,0,0};
  char **statement;
  char *getbitvar;
  int Aregmatch=0;
  char Aregcopy[50];
  int fixpoint1;
  int fixpoint2;
  int fixpoint3=0;
  char **deallocstatement;
  char **rpn_statement; // expression in rpn
  char rpn_stack[100][100]; // prolly doesn't need to be this big
  int sp=0; // stack pointer for converting to rpn
  int numrpn=0;
  char **atomic_statement; // singular statements to recurse back to here
  char tempstatement1[100],tempstatement2[100];

  strcpy(Aregcopy,"index-invalid");

  statement = (char **)malloc(sizeof(char*)*50);
  deallocstatement=statement;
  if (!strncmp(cstatement[2],"=\0",1)) {
    for (i=48;i>0;--i) {
      statement[i+1]=cstatement[i];
    }
  }
  else statement=cstatement;

  // check for unary minus (e.g. a=-a) and insert zero before it
  if ((statement[4][0] == '-') && (statement[5][0])>(unsigned char)0x3F) 
  {
    shiftdata(statement,4);
    statement[4][0]='0';
  }


  fixpoint1=isfixpoint(statement[2]);
  fixpoint2=isfixpoint(statement[4]);
  removeCR(statement[4]);

  // check for complex statement
  if ( (!((statement[4][0] == '-') && (statement[6][0] == ':'))) &&
       (statement[5][0]!=':') && (statement[7][0]!=':') && ( !((statement[5][0]=='(') && (statement[4][0] != '('))) &&
      ((unsigned char)statement[5][0] > (unsigned char)0x20) &&
      ((unsigned char)statement[7][0] > (unsigned char)0x20))
  {
printf("; complex statement detected\n");
    // complex statement here, hopefully.
    // convert equation to reverse-polish notation so we can express it in terms of
    // atomic equations and stack pushes/pulls
    rpn_statement=(char **)malloc(sizeof(char *)*100);
    for (i=0;i<100;++i){rpn_statement[i]=(char *)malloc(sizeof(char)*100);
    for (k=0;k<100;++k)rpn_statement[i][k]='\0';}

    atomic_statement=(char **)malloc(sizeof(char *)*10);
    for (i=0;i<10;++i){atomic_statement[i]=(char *)malloc(sizeof(char)*100);
    for (k=0;k<100;++k)atomic_statement[i][k]='\0';}

   // this converts expression to rpn
    for (k = 4; (statement[k][0]!='\0') && (statement[k][0]!=':'); k++)
    {
      // ignore CR/LF
      if (statement[k][0]==(unsigned char)0x0A) continue;
      if (statement[k][0]==(unsigned char)0x0D) continue;

      strcpy(tempstatement1,statement[k]);
      if (!isoperator(tempstatement1[0]))
        {strcpy(rpn_statement[numrpn++], tempstatement1);}
      else
      {
        while ((sp) && (orderofoperations(rpn_stack[sp-1][0], tempstatement1[0])))
        {
          strcpy(tempstatement2, rpn_stack[sp-1]);
          sp--;
          strcpy(rpn_statement[numrpn++], tempstatement2);
        }
        if ((sp) && (tempstatement1[0] == ')'))
          sp--;
        else
	  strcpy(rpn_stack[sp++],tempstatement1);
      }
    }



     // get stuff off of our rpn stack
    while (sp)
    {
      strcpy(tempstatement2, rpn_stack[sp-1]);
      sp--;
      strcpy(rpn_statement[numrpn++], tempstatement2);
    }

//for(i=0;i<20;++i)fprintf(stderr,"%s ",rpn_statement[i]);i=0;
     
    // now parse rpn statement 

//    strcpy(atomic_statement[2],"Areg");
//    strcpy(atomic_statement[3],"=");
//    strcpy(atomic_statement[4],rpn_statement[0]);
//    strcpy(atomic_statement[5],rpn_statement[2]);
//    strcpy(atomic_statement[6],rpn_statement[1]);
//    let(atomic_statement);

    sp=0; // now use as pointer into rpn_statement
    while (sp < numrpn)
    {
      // clear atomic statement cache
      for (i=0;i<10;++i)for (k=0;k<100;++k)atomic_statement[i][k]='\0';
      if (isoperator(rpn_statement[sp][0]))
      {
        // operator: need stack pull as 2nd arg
        // Areg=Areg (op) stackpull
        strcpy(atomic_statement[2],"Areg");
        strcpy(atomic_statement[3],"=");
        strcpy(atomic_statement[4],"Areg");
        strcpy(atomic_statement[5],rpn_statement[sp++]);
        strcpy(atomic_statement[6],"stackpull");
	let(atomic_statement);
      }
      else if (isoperator(rpn_statement[sp+1][0]))
      {
        // val,operator: Areg=Areg (op) val
        strcpy(atomic_statement[2],"Areg");
        strcpy(atomic_statement[3],"=");
        strcpy(atomic_statement[4],"Areg");
        strcpy(atomic_statement[6],rpn_statement[sp++]);
        strcpy(atomic_statement[5],rpn_statement[sp++]);
	let(atomic_statement);
      }
      else if (isoperator(rpn_statement[sp+2][0]))
      {
        // val,val,operator: stackpush, then Areg=val1 (op) val2
	if (sp) printf("	PHA\n");
        strcpy(atomic_statement[2],"Areg");
        strcpy(atomic_statement[3],"=");
        strcpy(atomic_statement[4],rpn_statement[sp++]);
        strcpy(atomic_statement[6],rpn_statement[sp++]);
        strcpy(atomic_statement[5],rpn_statement[sp++]);
	let(atomic_statement);
      }
      else
      {
        if ((rpn_statement[sp]=='\0') || (rpn_statement[sp+1]=='\0') || (rpn_statement[sp+2]=='\0'))
	{
	  // incomplete or unrecognized expression
	  prerror("Cannot evaluate expression\n");
	  exit(1);
	}
        // val,val,val: stackpush, then load of next value
	if (sp) printf("	PHA\n");
        strcpy(atomic_statement[2],"Areg");
        strcpy(atomic_statement[3],"=");
        strcpy(atomic_statement[4],rpn_statement[sp++]);
	let(atomic_statement);
      }
    } 
    // done, now assign A-reg to original value
    for (i=0;i<10;++i)for (k=0;k<100;++k)atomic_statement[i][k]='\0';
    strcpy(atomic_statement[2],statement[2]);
    strcpy(atomic_statement[3],"=");
    strcpy(atomic_statement[4],"Areg");
    let(atomic_statement);
    return; // bye-bye!
  }


  //check for [indexing]
  strcpy(Aregcopy,statement[4]);
  if (!strcmp(statement[4],Areg)) Aregmatch=1; // do we already have the correct value in A?

  index |= getindex(statement[2],&getindex0[0]);
  index |= getindex(statement[4],&getindex1[0])<<1;
  if (statement[5][0]!=':')
    index |= getindex(statement[6],&getindex2[0])<<2;


  // check for array, e.g. x(1) to access bit 1 of x
  for (i=0;i<50;++i) 
  {
    if (statement[2][i]=='\0') {i=50;break;}
    if (statement[2][i]=='}') break;
  }
  if (i<50) // found bit
  {
    strcpy(Areg,"invalid");
    // extract expression in parantheses - for now just whole numbers allowed
    bit=(int)statement[2][i-1]-48;
    if ((bit>9)||(bit<0))
    {
      fprintf(stderr,"(%d) Error: variables in bit access not supported\n",line);
      exit(1);
    }

    if (statement[4][0]=='0')
    {
      printf("	LDA ");
      for (i=0;i<50;++i) 
      {
        if (statement[2][i]=='{') break;
        printf("%c",statement[2][i]);
      }
      printf("\n");
      printf("	AND #%d\n",255^(1<<bit));//(int)pow(2,bit));
    }
    else if (statement[4][0]=='1')
    {
      printf("	LDA ");
      for (i=0;i<50;++i) 
      {
        if (statement[2][i]=='{') break;
        printf("%c",statement[2][i]);
      }
      printf("\n");
      printf("	ORA #%d\n",1<<bit);//(int)pow(2,bit));
    }
    else if (getbitvar=strtok(statement[4],"{"))
    {  // assign one bit to another
	// I haven't a clue if this will actually work!
      if (getbitvar[0] == '!') 
	printf("	LDA %s\n",getbitvar+1);
      else
	printf("	LDA %s\n",getbitvar);
      printf("	AND #%d\n",(1<<((int)statement[4][strlen(getbitvar)+1]-48)));
      printf("  PHP\n");
      printf("	LDA ");
      for (i=0;i<50;++i) 
      {
        if (statement[2][i]=='{') break;
        printf("%c",statement[2][i]);
      }
      printf("\n  PLP\n");
      if (getbitvar[0] == '!') 
        printf("	.byte $F0, $03\n"); //bad, bad way to do BEQ addr+5
      else
        printf("	.byte $D0, $03\n"); //bad, bad way to do BNE addr+5

      printf("	AND #%d\n",255^(1<<bit));//(int)pow(2,bit));
      printf("	.byte $0C\n"); // NOP absoulte to skip next two bytes
      printf("	ORA #%d\n",1<<bit);//(int)pow(2,bit));

    }
    else 
    {
      fprintf(stderr,"(%d) Error: can only assign 0, 1 or another bit to a bit\n",line);
      exit(1);
    }
    printf("	STA ");
    for (i=0;i<50;++i) 
    {
      if (statement[2][i]=='{') break;
      printf("%c",statement[2][i]);
    }
    printf("\n");
    free(deallocstatement);
    return;
  }

  if (statement[4][0]=='-') // assignment to negative
  {
    strcpy(Areg,"invalid");
    if ((!fixpoint1) && (isfixpoint(statement[5]) != 12))
    {
      if (statement[5][0]>(unsigned char)0x39) // perhaps include constants too?
      {
        printf("	LDA #0\n");
        printf("  SEC\n");
        printf("	SBC %s\n",statement[5]);
      }
      else printf("	LDA #%d\n",256-atoi(statement[5]));
    }
    else
    {
      if (fixpoint1 == 4)
      {
      if (statement[5][0]>(unsigned char)0x39) // perhaps include constants too?
	{
          printf("	LDA #0\n");
          printf("  SEC\n");
          printf("	SBC %s\n",statement[5]);
	}
	else
	  printf("	LDA #%d\n",(int)((16-atof(statement[5]))*16));
	printf("	STA %s\n",statement[2]);
	free(deallocstatement);
	return;
      }
      if (fixpoint1 == 8)
      {
	printf("	LDX ");
	sprintf(statement[4],"%f",256.0-atof(statement[5]));
	printfrac(statement[4]);
	printf("	STX ");
	printfrac(statement[2]);
	printf("	LDA #%s\n",statement[4]);
	printf("	STA %s\n",statement[2]);
	free(deallocstatement);
	return;
      }
    }
  }
  else if (!strncmp(statement[4],"rand\0",4)) {
    strcpy(Areg,"invalid");
    if (optimization & 8)
    {
      printf("        lda rand\n");
      printf("        lsr\n");
      printf(" ifconst rand16\n");
      printf("        rol rand16\n");
      printf(" endif\n");
      printf("        bcc *+4\n");
      printf("        eor #$B4\n");
      printf("        sta rand\n");
      printf(" ifconst rand16\n");
      printf("        eor rand16\n");
      printf(" endif\n");
    }
    else jsr("randomize");
  }
  else if ( (!strncmp(statement[2],"score\0",5))
            && (strncmp(statement[2],"scorec\0",6)))
  {
// break up into three parts
    strcpy(Areg,"invalid");
    if (statement[5][0] == '+')
    {
      printf("	SED\n");
      printf("	CLC\n");
      for (i=5;i>=0;i--)
      {
  	if (statement[6][i] != '\0') score[j]=number(statement[6][i]);
	score[j]=number(statement[6][i]);
	if ((score[j]<10) && (score[j]>=0)) j++;
      }
      printf("	LDA score+2\n");
      if (statement[6][0]>(unsigned char)0x3F) printf("	ADC %s\n",statement[6]);
      else printf("	ADC #$%d%d\n",score[1],score[0]);
      printf("	STA score+2\n");      
      printf("	LDA score+1\n");      
      if (score[0]>9) printf("	ADC #0\n");
      else printf("	ADC #$%d%d\n",score[3],score[2]);
      printf("	STA score+1\n");      
      printf("	LDA score\n");      
      if (score[0]>9) printf("	ADC #0\n");
      else printf("	ADC #$%d%d\n",score[5],score[4]);
      printf("	STA score\n");      
      printf("	CLD\n");
    }
    else if (statement[5][0] == '-')
    {
      printf("	SED\n");
      printf("	SEC\n");
      for (i=5;i>=0;i--)
      {
	if (statement[6][i] != '\0') score[j]=number(statement[6][i]);
	score[j]=number(statement[6][i]);
	if ((score[j]<10) && (score[j]>=0)) j++;
      }
      printf("	LDA score+2\n");      
      if (score[0]>9) printf("	SBC %s\n",statement[6]);
      else printf("	SBC #$%d%d\n",score[1],score[0]);
      printf("	STA score+2\n");      
      printf("	LDA score+1\n");      
      if (score[0]>9) printf("	SBC #0\n");
      else printf("	SBC #$%d%d\n",score[3],score[2]);
      printf("	STA score+1\n");      
      printf("	LDA score\n");      
      if (score[0]>9) printf("	SBC #0\n");
      else printf("	SBC #$%d%d\n",score[5],score[4]);
      printf("	STA score\n");      
      printf("	CLD\n");
    }
    else
    {
      for (i=5;i>=0;i--)
      {
	if (statement[4][i] != '\0') score[j]=number(statement[4][i]);
	score[j]=number(statement[4][i]);
	if ((score[j]<10) && (score[j]>=0)) j++;
      }
      printf("	LDA #$%d%d\n",score[1],score[0]);
      printf("	STA score+2\n");      
      printf("	LDA #$%d%d\n",score[3],score[2]);
      printf("	STA score+1\n");      
      printf("	LDA #$%d%d\n",score[5],score[4]);
      printf("	STA score\n");      
    }
    free(deallocstatement);
    return;

  }
  else if ( (statement[6][0] == '1') && ((statement[6][1] > (unsigned char)0x39) 
          || (statement[6][1] < (unsigned char)0x30)) &&
          ((statement[5][0] == '+') || (statement[5][0] == '-')) && 
	  (!strncmp(statement[2], statement[4], 50)) && 
          (strncmp(statement[2],"Areg\0",4)) &&
          (statement[6][1]=='\0' || statement[6][1]==' ' || statement[6][1]=='\n'))
  { // var=var +/- something
    strcpy(Areg,"invalid");
    if ((fixpoint1 == 4) && (fixpoint2 == 4))
    {
      if (statement[5][0] == '+')
      {
	printf("	LDA %s\n",statement[2]);
	printf("	CLC %s\n");
	printf("	ADC #16\n");
	printf("	STA %s\n",statement[2]);
	free(deallocstatement);
	return;
      }
      if (statement[5][0] == '-')
      {
	printf("	LDA %s\n",statement[2]);
	printf("	SEC %s\n");
	printf("	SBC #16\n");
	printf("	STA %s\n",statement[2]);
	free(deallocstatement);
	return;
      }    
    }

    if (index&1) loadindex(&getindex0[0]);
    if (statement[5][0] == '+') printf("	INC ");
    else printf("	DEC ");
    if (!(index&1))
      printf("%s\n",statement[2]);
    else printf("%s,x\n",statement[4]); // indexed with x!
    free(deallocstatement);

    return;
  }
  else 
  {// This is generic x=num or var

    if (!Aregmatch) // do we already have the correct value in A?
    {
      if (index&2) loadindex(&getindex1[0]);

// if 8.8=8.8+8.8: this LDA will be superfluous - fix this at some point   

//      if (!fixpoint1 && !fixpoint2 && statement[5][0]!='(')
      if (((!fixpoint1 && !fixpoint2) || (!fixpoint1 && fixpoint2 == 8)) && statement[5][0]!='(')

//	printfrac(statement[4]);
//      else
      {
        if (strncmp(statement[4],"Areg\n",4))
        {
	  printf("	LDA ");
	  printindex(statement[4],index&2);
	}
      }
      strcpy(Areg,Aregcopy);
    }
  }
  if ((statement[5][0] != '\0') && (statement[5][0] != ':')) {  // An operator was found
    fixpoint3=isfixpoint(statement[6]);
    strcpy(Areg,"invalid");
    if (index&4) loadindex(&getindex2[0]);
    if (statement[5][0] == '+') {
//      if ((fixpoint1 == 4) && (fixpoint2 == 4))
//      {

//      }
//      else
//      {
	if ((fixpoint1 == 8) && ((fixpoint2 & 8) == 8) && ((fixpoint3 & 8) == 8))
	{ //8.8=8.8+8.8
	  printf("	LDA ");
	  printfrac(statement[4]);
	  printf("	CLC \n");
	  printf("	ADC ");
	  printfrac(statement[6]);
	  printf("	STA ");
	  printfrac(statement[2]);
	  printf("	LDA ");
	  printimmed(statement[4]);
	  printf("%s\n",statement[4]);
          printf("	ADC ");
	  printimmed(statement[6]);
	  printf("%s\n",statement[6]);
	}
	else if ((fixpoint1 == 8) && ((fixpoint2 & 8) == 8) && (fixpoint3 == 4))
	{
	  printf("	LDY %s\n",statement[6]);
	  printf("	LDX ");
	  printfrac(statement[4]);
	  printf("	LDA ");
	  printimmed(statement[4]);
	  printf("%s\n",statement[4]);
	  jsrbank1("Add44to88");
	  printf("	STX ");
	  printfrac(statement[2]);
	}
	else if ((fixpoint1 == 4) && (fixpoint2 == 8) && ((fixpoint3 & 4) == 4))
	{
	  if (fixpoint3 == 4)
	    printf("	LDY %s\n",statement[6]);
	  else
	    printf("	LDY #%d\n",(int)(atof(statement[6])*16.0));
	  printf("	LDA %s\n",statement[4]);
	  printf("	LDX ");
	  printfrac(statement[4]);
	  jsrbank1("Add88to44");
	}
	else if ((fixpoint1 == 4) && (fixpoint2 == 4) && (fixpoint3 == 12))
	{
	  printf("	CLC\n");
	  printf("	LDA %s\n",statement[4]);
	  printf("	ADC #%d\n",(int)(atof(statement[6])*16.0));
	}
	else if ((fixpoint1 == 4) && (fixpoint2 == 12) && (fixpoint3 == 4))
	{
	  printf("	CLC\n");
	  printf("	LDA %s\n",statement[6]);
	  printf("	ADC #%d\n",(int)(atof(statement[4])*16.0));
	}
	else // this needs work - 44+8+44 and probably others are screwy
        {
	  if (fixpoint2 == 4) printf("	LDA %s\n",statement[4]);
	  if ((fixpoint3 == 4) && (fixpoint2 == 0)) 
	  {
	    printf("	LDA "); // this LDA might be superfluous
	    printimmed(statement[4]);
	    printf("%s\n",statement[4]);
	  }
          displayoperation("+CLC\n	ADC",statement[6],index&4);
        }
//    }
    }
    else if (statement[5][0] == '-') {
	if ((fixpoint1 == 8) && ((fixpoint2 & 8) == 8) && ((fixpoint3 & 8) == 8))
	{ //8.8=8.8-8.8
	  printf("	LDA ");
	  printfrac(statement[4]);
	  printf("	SEC \n");
	  printf("	SBC ");
	  printfrac(statement[6]);
	  printf("	STA ");
	  printfrac(statement[2]);
	  printf("	LDA ");
	  printimmed(statement[4]);
	  printf("%s\n",statement[4]);
          printf("	SBC ");
	  printimmed(statement[6]);
	  printf("%s\n",statement[6]);
	}
	else if ((fixpoint1 == 8) && ((fixpoint2 & 8) == 8) && (fixpoint3 == 4))
	{
	  printf("	LDY %s\n",statement[6]);
	  printf("	LDX ");
	  printfrac(statement[4]);
	  printf("	LDA ");
	  printimmed(statement[4]);
	  printf("%s\n",statement[4]);
	  jsrbank1("Sub44from88");
	  printf("	STX ");
	  printfrac(statement[2]);
	}
	else if ((fixpoint1 == 4) && (fixpoint2 == 8) && ((fixpoint3 & 4) == 4))
	{
	  if (fixpoint3 == 4)
	    printf("	LDY %s\n",statement[6]);
	  else
	    printf("	LDY #%d\n",(int)(atof(statement[6])*16.0));
	  printf("	LDA %s\n",statement[4]);
	  printf("	LDX ");
	  printfrac(statement[4]);
	  jsrbank1("Sub88from44");
	}
	else if ((fixpoint1 == 4) && (fixpoint2 == 4) && (fixpoint3 == 12))
	{
	  printf("	SEC\n");
	  printf("	LDA %s\n",statement[4]);
	  printf("	SBC #%d\n",(int)(atof(statement[6])*16.0));
	}
	else if ((fixpoint1 == 4) && (fixpoint2 == 12) && (fixpoint3 == 4))
	{
	  printf("	SEC\n");
	  printf("	LDA #%d\n",(int)(atof(statement[4])*16.0));
	  printf("	SBC %s\n",statement[6]);
	}
	else
        {
	  if (fixpoint2 == 4) printf("	LDA %s\n",statement[4]);
	  if ((fixpoint3 == 4) && (fixpoint2 == 0)) printf("	LDA #%s\n",statement[4]);
          displayoperation("-SEC\n	SBC",statement[6],index&4);
	}
    }
    else if (statement[5][0] == '&') {
      if (fixpoint2 == 4) printf("	LDA %s\n",statement[4]);
      displayoperation("&AND",statement[6],index&4);
    }
    else if (statement[5][0] == '^') {
      if (fixpoint2 == 4) printf("	LDA %s\n",statement[4]);
      displayoperation("^EOR",statement[6],index&4);
    }
    else if (statement[5][0] == '|') {
      if (fixpoint2 == 4) printf("	LDA %s\n",statement[4]);
      displayoperation("|ORA",statement[6],index&4);
    }
    else if (statement[5][0] == '*') {
      if (fixpoint2 == 4) printf("	LDA %s\n",statement[4]);
      if ((!isimmed(statement[6])) || (!checkmul(atoi(statement[6]))))
      {
        displayoperation("*LDY",statement[6],index&4);
        if (statement[5][1] == '*') jsrbank1("mul16"); // general mul routine
        else jsrbank1("mul8");
      }
      else if (statement[5][1] == '*') mul(statement, 16);
      else mul(statement, 8);// attempt to optimize - may need to call mul anyway
      
    }
    else if (statement[5][0] == '/') {
      if (fixpoint2 == 4) printf("	LDA %s\n",statement[4]);
      if ((!isimmed(statement[6])) || (!checkdiv(atoi(statement[6]))))
      {
        displayoperation("/LDY",statement[6],index&4);
        if (statement[5][1] == '/') jsrbank1("div16"); // general div routine
        else jsrbank1("div8");
      }
      else if (statement[5][1] == '/') divd(statement, 16);
      else divd(statement, 8);// attempt to optimize - may need to call divd anyway
      
    }
    else if (statement[5][0] == ':'){
      strcpy(Areg,Aregcopy); // A reg is not invalid
    } 
    else if (statement[5][0] == '('){
    // we've called a function, hopefully
      strcpy(Areg,"invalid");
      if (!strncmp(statement[4],"sread\0",5)) sread(statement);
      else callfunction(statement);
    }
    else 
    {
      fprintf(stderr, "(%d) Error: invalid operator: %s\n",line,statement[5]);
      exit(1);
    }

  }
  else // simple assignment
  {
    // check for fixed point stuff here
 // bugfix: forgot the LDA (?) did I do this correctly???
    if ((fixpoint1 == 4) && (fixpoint2 == 0))
    {
      printf("	LDA ");
      printimmed(statement[4]);
      printf("%s\n",statement[4]);
      printf("  ASL\n");
      printf("  ASL\n");
      printf("  ASL\n");
      printf("  ASL\n");
    }
    else if ((fixpoint1 == 0) && (fixpoint2 == 4))
    {   
      printf("	LDA ");
      printimmed(statement[4]);
      printf("%s\n",statement[4]);
      printf("  LSR\n");
      printf("  LSR\n");
      printf("  LSR\n");
      printf("  LSR\n");
    }  
    else if ((fixpoint1 == 4) && (fixpoint2 == 8))
    {
      printf("	LDA ");
      printimmed(statement[4]);
      printf("%s\n",statement[4]);
      printf("  LDX ");
      printfrac(statement[4]);
// note: this shouldn't be changed to jsr(); (why???)
      printf(" jsr Assign88to44");
      if (bs) printf("bs");
      printf("\n");
    }
    else if ((fixpoint1 == 8) && (fixpoint2 == 4))
    {
// note: this shouldn't be changed to jsr();
      printf("	LDA ");
      printimmed(statement[4]);
      printf("%s\n",statement[4]);
      printf("  JSR Assign44to88");
      if (bs) printf("bs");
      printf("\n");
      printf("  STX ");
      printfrac(statement[2]);
    }
    else if ((fixpoint1 == 8) && ((fixpoint2 & 8) == 8))
    {
      printf("	LDX ");
      printfrac(statement[4]);
      printf("	STX ");
      printfrac(statement[2]);
      printf("	LDA ");
      printimmed(statement[4]);
      printf("%s\n",statement[4]);
    }
    else if ((fixpoint1 == 4) && ((fixpoint2 & 4) == 4))
    {
      if (fixpoint2 == 4)
        printf("	LDA %s\n",statement[4]);
      else
        printf("	LDA #%d\n",(int)(atof(statement[4])*16));
    }
//    else if ((fixpoint1 == 0) && (fixpoint1 == 8))
//    {
//      printf("	LDA %s\n",statement[4]);
//    }
  }
    if (index&1) loadindex(&getindex0[0]);
    if (strncmp(statement[2],"Areg\0",4))
    {
      printf("	STA ");
      printindex(statement[2],index&1);
    }
    free(deallocstatement);
}

void dogoto(char **statement)
{
  int anotherbank=0;
  if (!strncmp(statement[3],"bank",4)) anotherbank=(int)(statement[3][4])-48;
  else
  {
    printf(" jmp .%s\n",statement[2]);
    return;
  }

// if here, we're jmp'ing to another bank
// we need to switch banks
  printf(" sta temp7\n");
// next we must push the place to jmp to
  printf(" lda #>(.%s-1)\n",statement[2]);
  printf(" pha\n");
  printf(" lda #<(.%s-1)\n",statement[2]);
  printf(" pha\n");
// now store regs on stack
  printf(" lda temp7\n");
  printf(" pha\n");
  printf(" txa\n");
  printf(" pha\n");
// select bank to switch to
  printf(" ldx #%d\n",anotherbank);
  printf(" jmp BS_jsr\n"); // also works for jmps
}

void gosub(char **statement)
{
  int anotherbank=0;
  invalidate_Areg();
  //if (numgosubs++>3) {
   // fprintf(stderr,"Max. nested gosubs exceeded in line %s\n",statement[0]);
   // exit(1);
  //}
  if (!strncmp(statement[3],"bank",4)) anotherbank=(int)(statement[3][4])-48;
  else
  {
    printf(" jsr .%s\n",statement[2]);
    return;
  }

// if here, we're jsr'ing to another bank
// we need to switch banks
  printf(" sta temp7\n");
// first create virtual return address
  printf(" lda #>(ret_point%d-1)\n",++numjsrs);
  printf(" pha\n");
  printf(" lda #<(ret_point%d-1)\n",numjsrs);
  printf(" pha\n");
// next we must push the place to jsr to
  printf(" lda #>(.%s-1)\n",statement[2]);
  printf(" pha\n");
  printf(" lda #<(.%s-1)\n",statement[2]);
  printf(" pha\n");
// now store regs on stack
  printf(" lda temp7\n");
  printf(" pha\n");
  printf(" txa\n");
  printf(" pha\n");
// select bank to switch to
  printf(" ldx #%d\n",anotherbank);
  printf(" jmp BS_jsr\n");
  printf("ret_point%d\n",numjsrs); 
}

void set(char **statement)
{
  char **pass2const;
  int i;
  int valid_kernel_combos[]=
    { // C preprocessor should turn these into numbers!
	_player1colors,
	_no_blank_lines,
	_pfcolors,
	_pfheights,
	_pfcolors | _pfheights,
	_pfcolors | _pfheights | _background,
	_pfcolors | _no_blank_lines,
	_pfcolors | _no_blank_lines | _background,
	_player1colors | _no_blank_lines,
	_player1colors | _pfcolors,
	_player1colors | _pfheights,
	_player1colors | _pfcolors | _pfheights,
	_player1colors | _pfcolors | _background,
	_player1colors | _pfheights | _background,
	_player1colors | _pfcolors | _pfheights | _background,
	_player1colors | _no_blank_lines | _readpaddle,
	_player1colors | _no_blank_lines | _pfcolors,
	_player1colors | _no_blank_lines | _pfcolors | _background,
	_playercolors | _player1colors | _pfcolors,
	_playercolors | _player1colors | _pfheights,
	_playercolors | _player1colors | _pfcolors | _pfheights,
	_playercolors | _player1colors | _pfcolors | _background,
	_playercolors | _player1colors | _pfheights | _background,
	_playercolors | _player1colors | _pfcolors | _pfheights | _background,
	_no_blank_lines | _readpaddle,
	255 };
  pass2const=(char **)malloc(sizeof(char *)*5);
  if (!strncasecmp(statement[2],"tv\0",2))
  {
    if (!strncasecmp(statement[3],"ntsc\0",4)) 
    {
      // pick constant timer values for now, later maybe add more lines
      strcpy(redefined_variables[numredefvars++],"overscan_time = 37"); 
      strcpy(redefined_variables[numredefvars++],"vblank_time = 43"); 
    }
    else if (!strncasecmp(statement[3],"pal\0",3)) 
    {
      // 36 and 48 scanlines, respectively
      strcpy(redefined_variables[numredefvars++],"overscan_time = 82");
      strcpy(redefined_variables[numredefvars++],"vblank_time = 57");
    }
    else prerror("set TV: invalid TV type\n");
  }
  else if (!strncmp(statement[2],"smartbranching\0",14))
  {
    if (!strncmp(statement[3],"on\0",2)) smartbranching=1;
    else smartbranching=0;
  }
  else if (!strncmp(statement[2],"romsize\0",7))
  {
    set_romsize(statement[3]);
  }
  else if (!strncmp(statement[2],"optimization\0",5))
  {
    if (!strncmp(statement[3],"speed\0",5))
    {
      optimization=1;
    }
    if (!strncmp(statement[3],"size\0",4))
    {
      optimization=2;
    }
    if (!strncmp(statement[3],"noinlinedata\0",4))
    {
      optimization|=4;
    }
    if (!strncmp(statement[3],"inlinerand\0",4))
    {
      optimization|=8;
    }
    if (!strncmp(statement[3],"none\0",4))
    {
      optimization=0;
    }
  }
  else if (!strncmp(statement[2],"kernal\0",6))
  {
    prerror("The proper spelling is \"kernel.\"  With an e.  Please make a note of this to save yourself from further embarassment.\n");
  }
  else if (!strncmp(statement[2],"kernel_options\0",10))
  {
    i=3;kernel_options=0;
    while (((unsigned char)statement[i][0]>(unsigned char)64) && ((unsigned char)statement[i][0]<(unsigned char)123))
    {
      if (!strncmp(statement[i],"readpaddle\0",10))
      {
	strcpy(redefined_variables[numredefvars++],"readpaddle = 1"); 
	kernel_options|=1;
      }
      else if (!strncmp(statement[i],"player1colors\0",13))
      {
	strcpy(redefined_variables[numredefvars++],"player1colors = 1"); 
	kernel_options|=2;
      }
      else if (!strncmp(statement[i],"playercolors\0",12))
      {
	strcpy(redefined_variables[numredefvars++],"playercolors = 1"); 
	strcpy(redefined_variables[numredefvars++],"player1colors = 1"); 
	kernel_options|=6;
      }
      else if (!strncmp(statement[i],"no_blank_lines\0",13))
      {
	strcpy(redefined_variables[numredefvars++],"no_blank_lines = 1"); 
	kernel_options|=8;
      }
      else if (!strncasecmp(statement[i],"pfcolors\0",8))
      {
	kernel_options|=16;
      }
      else if (!strncasecmp(statement[i],"pfheights\0",9))
      {
	kernel_options|=32;
      }
      else if (!strncasecmp(statement[i],"backgroundchange\0",10))
      {
	strcpy(redefined_variables[numredefvars++],"backgroundchange = 1"); 
	kernel_options|=64;
      }
      else {prerror("set kernel_options: Options unknown or invalid\n");exit(1);}
      i++;
    }
    if ((kernel_options&48)==32) strcpy(redefined_variables[numredefvars++],"PFheights = 1"); 
    else if ((kernel_options&48)==16) strcpy(redefined_variables[numredefvars++],"PFcolors = 1"); 
    else if ((kernel_options&48)==48) strcpy(redefined_variables[numredefvars++],"PFcolorandheight = 1"); 
//fprintf(stderr,"%d\n",kernel_options);
    // check for valid combinations
    if (kernel_options==1) {prerror("set kernel_options: readpaddle must be used with no_blank_lines\n");exit(1);}
    i=0;while (1)
    {
      if (valid_kernel_combos[i]==255) {prerror("set kernel_options: Invalid cambination of options\n");exit(1);}
      if (kernel_options==valid_kernel_combos[i++]) break;
    }
  }
  else if (!strncmp(statement[2],"kernel\0",6))
  {
    if (!strncmp(statement[3],"multisprite\0",11))
    {
      multisprite=1;
      strcpy(redefined_variables[numredefvars++],"multisprite = 1"); 
      create_includes("multisprite.inc");
      ROMpf=1;
    }
    else if (!strncmp(statement[3],"multisprite_no_include\0",11))
    {
      multisprite=1;
      strcpy(redefined_variables[numredefvars++],"multisprite = 1"); 
      ROMpf=1;
    }
    else prerror("set kernel: kernel name unknown or unspecified\n");
  }
  else if (!strncmp(statement[2],"debug\0",5))
  {
    if (!strncmp(statement[3],"cyclescore\0",10))
    {
      strcpy(redefined_variables[numredefvars++],"debugscore = 1"); 
    }
    else if (!strncmp(statement[3],"cycles\0",6))
    {
      strcpy(redefined_variables[numredefvars++],"debugcycles = 1"); 
    }
    else prerror("set debug: debugging mode unknown\n");
  }
  else if (!strncmp(statement[2],"legacy\0",6))
  {
      sprintf(redefined_variables[numredefvars++],"legacy = %d",(int)(100*(atof(statement[3]))));
  }
  else prerror("set: unknown parameter\n");
  
}

void rem(char **statement)
{
  if (!strncmp(statement[2],"smartbranching\0",14))
  {
    if (!strncmp(statement[3],"on\0",2)) smartbranching=1;
    else smartbranching=0;
  }
}

void dopop()
{
  printf("	pla\n");
  printf("	pla\n");
}

void bmi(char *linenumber)
{
  removeCR(linenumber);
  if (smartbranching) {
    printf(" if ((* - .%s) < 127) && ((* - .%s) > -128)\n	bmi .%s\n",linenumber,linenumber,linenumber);
    // branches might be allowed as below - check carefully to make sure!
    // printf(" if ((* - .%s) < 127) && ((* - .%s) > -129)\n	bmi .%s\n",linenumber,linenumber,linenumber);
    printf(" else\n	bpl .%dskip%s\n	jmp .%s\n",branchtargetnumber,linenumber,linenumber);
    printf(".%dskip%s\n",branchtargetnumber++,linenumber);
    printf(" endif\n");
  }
  else printf("	bmi .%s\n",linenumber);
}

void bpl(char *linenumber)
{
  removeCR(linenumber);
  if (smartbranching) {
    printf(" if ((* - .%s) < 127) && ((* - .%s) > -128)\n	bpl .%s\n",linenumber,linenumber,linenumber);
    printf(" else\n	bmi .%dskip%s\n	jmp .%s\n",branchtargetnumber,linenumber,linenumber);
    printf(".%dskip%s\n",branchtargetnumber++,linenumber);
    printf(" endif\n");
  }
  else printf("	bpl .%s\n",linenumber);
}

void bne(char *linenumber)
{
  removeCR(linenumber);
  if (smartbranching) {
    printf(" if ((* - .%s) < 127) && ((* - .%s) > -128)\n	BNE .%s\n",linenumber,linenumber,linenumber);
    printf(" else\n	beq .%dskip%s\n	jmp .%s\n",branchtargetnumber,linenumber,linenumber);
    printf(".%dskip%s\n",branchtargetnumber++,linenumber);
    printf(" endif\n");
  }
  else printf("	bne .%s\n",linenumber);
}

void beq(char *linenumber)
{
  removeCR(linenumber);
  if (smartbranching) {
    printf(" if ((* - .%s) < 127) && ((* - .%s) > -128)\n	BEQ .%s\n",linenumber,linenumber,linenumber);
    printf(" else\n	bne .%dskip%s\n	jmp .%s\n",branchtargetnumber,linenumber,linenumber);
    printf(".%dskip%s\n",branchtargetnumber++,linenumber);
    printf(" endif\n");
  }
  else printf("	beq .%s\n",linenumber);
}

void bcc(char *linenumber)
{
  removeCR(linenumber);
  if (smartbranching) {
    printf(" if ((* - .%s) < 127) && ((* - .%s) > -128)\n	bcc .%s\n",linenumber,linenumber,linenumber);
    printf(" else\n	bcs .%dskip%s\n	jmp .%s\n",branchtargetnumber,linenumber,linenumber);
    printf(".%dskip%s\n",branchtargetnumber++,linenumber);
    printf(" endif\n");
  }
  else printf("	bcc .%s\n",linenumber);
}

void bcs(char *linenumber)
{
  removeCR(linenumber);
  if (smartbranching) {
    printf(" if ((* - .%s) < 127) && ((* - .%s) > -128)\n	bcs .%s\n",linenumber,linenumber,linenumber);
    printf(" else\n	bcc .%dskip%s\n	jmp .%s\n",branchtargetnumber,linenumber,linenumber);
    printf(".%dskip%s\n",branchtargetnumber++,linenumber);
    printf(" endif\n");
  }
  else printf("	bcs .%s\n",linenumber);
}

void bvc(char *linenumber)
{
  removeCR(linenumber);
  if (smartbranching) {
    printf(" if ((* - .%s) < 127) && ((* - .%s) > -128)\n	bvc .%s\n",linenumber,linenumber,linenumber);
    printf(" else\n	bvs .%dskip%s\n	jmp .%s\n",branchtargetnumber,linenumber,linenumber);
    printf(".%dskip%s\n",branchtargetnumber++,linenumber);
    printf(" endif\n");
  }
  else printf("	bvc .%s\n",linenumber);
}

void bvs(char *linenumber)
{
  removeCR(linenumber);
  if (smartbranching) {
    printf(" if ((* - .%s) < 127) && ((* - .%s) > -128)\n	bvs .%s\n",linenumber,linenumber,linenumber);
    printf(" else\n	bvc .%dskip%s\n	jmp .%s\n",branchtargetnumber,linenumber,linenumber);
    printf(".%dskip%s\n",branchtargetnumber++,linenumber);
    printf(" endif\n");
  }
  else printf("	bvs .%s\n",linenumber);
}

void drawscreen()
{
  invalidate_Areg();
  jsr("drawscreen");
}

void prerror(char *myerror)
{
  fprintf(stderr, "(%d): %s\n", line, myerror);
}

int printimmed(char *value)
{
  int immed=isimmed(value);
  if (immed) printf("#");
  return immed;
}

int isimmed(char *value)
{
  // search queue of constants
  int i;
  // removeCR(value);
  for (i=0;i<numconstants;++i) 
    if (!strcmp(value,constants[i]))
    {
      // a constant should be treated as an immediate
      return 1;
    }

  if ((value[0] == '$') || (value[0] == '%')
     || (value[0] < (unsigned char)0x3A)) 
  {
    return 1;
  }
  else return 0;
}

int number(unsigned char value)
{
  return ((int)value)-48;
}

void removeCR(char *linenumber)  // remove trailing CR from string
{
  while ((linenumber[strlen(linenumber)-1]==(unsigned char)0x0A) ||
      (linenumber[strlen(linenumber)-1]==(unsigned char)0x0D))
      linenumber[strlen(linenumber)-1]='\0';
}

void remove_trailing_commas(char *linenumber)  // remove trailing commas from string
{
  int i;
  for (i=strlen(linenumber)-1;i>0;i--)
  {
    if ((linenumber[i] != ',') &&
      (linenumber[i] != ' ') &&
      (linenumber[i] != (unsigned char)0x0A) &&
      (linenumber[i] != (unsigned char)0x0D)) break;
    if (linenumber[i] == ',')
    {
      linenumber[i] = ' ';
      break;
    }
  }
}

void header_open(FILE *header)
{
}

void header_write(FILE *header, char *filename)
{
  int i;
  if ((header = fopen(filename, "w")) == NULL)  // open file
    {
      fprintf(stderr,"Cannot open %s for writing\n",filename);
      exit(1);
    }

  strcpy(redefined_variables[numredefvars],"; This file contains variable mapping and other information for the current project.\n");

  for (i=numredefvars;i>=0;i--)
  {
    fprintf(header, "%s\n", redefined_variables[i]);
  }
  fclose(header);
 
}
