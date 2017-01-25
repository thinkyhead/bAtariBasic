#include "keywords.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void keywords(char **cstatement)
{
  char errorcode[100];
  char **statement;
  int i,j,k;
  int colons=0;
  int currentcolon=0;
  char **pass2elstatement;
  char **elstatement;
  char **orstatement;
  char **swapstatement;
  char **deallocstatement;
  char **deallocorstatement;
  char **deallocelstatement;
  int door;
  int foundelse=0;
  statement=(char**)malloc(sizeof(char*)*60);
  orstatement=(char **)malloc(sizeof(char*)*60);
  for (i=0;i<60;++i) {
    orstatement[i]=(char*)malloc(sizeof(char)*60);
  }  
  elstatement=(char **)malloc(sizeof(char*)*60);
  for (i=0;i<60;++i) {
    elstatement[i]=(char*)malloc(sizeof(char)*60);
  }  

//for (i=0;i<10;++i)fprintf(stderr,"%s ",cstatement[i]);
//fprintf(stderr,"\n");
//  deallocstatement=statement; // for deallocation purposes
//  deallocorstatement=orstatement; // for deallocation purposes
//  deallocelstatement=elstatement; // for deallocation purposes
  // check if there are boolean && or || in an if-then.
  // change && to "then if"
  // change || to two if thens
  // also change operands around to allow <= and >, since 
  // currently all we can do is < and >=
  // if we encounter an else, break into two lines, and the first line jumps ahead.
  door=0;
  for (k=0;k<=40;++k) // reversed loop since last build, need to check for rems first!
  {
    if (!strncmp(cstatement[k+1],"rem\0",3)) break; // if statement has a rem, do not process it
    if (!strncmp(cstatement[k+1],"if\0",2))
    {
      for (i=k+2;i<50;++i) 
      {
        if (!strncmp(cstatement[i],"if\0",2)) break;
        if (!strncmp(cstatement[i],"else\0",4)) foundelse=i;
      }
      if (!strncmp(cstatement[k+3],">\0",2))
      {
        // swap operands and switch compare
        strcpy(cstatement[k+3],cstatement[k+2]); // stick 1st operand here temporarily
        strcpy(cstatement[k+2],cstatement[k+4]);
        strcpy(cstatement[k+4],cstatement[k+3]);// get it back
        strcpy(cstatement[k+3],"<"); // replace compare
      }
      else if (!strncmp(cstatement[k+3],"<=\0",2))
      {
        // swap operands and switch compare
        strcpy(cstatement[k+3],cstatement[k+2]);
        strcpy(cstatement[k+2],cstatement[k+4]);
        strcpy(cstatement[k+4],cstatement[k+3]);
        strcpy(cstatement[k+3],">=");
      }
      if (!strncmp(cstatement[k+3],"&&\0",2))
      {
        shiftdata(cstatement,k+3);
        sprintf(cstatement[k+3],"then%d",ors++);
        strcpy(cstatement[k+4],"if");
      }
      else if (!strncmp(cstatement[k+5],"&&\0",2))
      {
      
	shiftdata(cstatement,k+5);
        sprintf(cstatement[k+5],"then%d",ors++);
        strcpy(cstatement[k+6],"if");
      }
      else if (!strncmp(cstatement[k+3],"||\0",2))
      {
        if (!strncmp(cstatement[k+5],">\0",2))
        {
          // swap operands and switch compare
          strcpy(cstatement[k+5],cstatement[k+4]); // stick 1st operand here temporarily
          strcpy(cstatement[k+4],cstatement[k+6]);
          strcpy(cstatement[k+6],cstatement[k+5]);// get it back
          strcpy(cstatement[k+5],"<"); // replace compare
        }
        else if (!strncmp(cstatement[k+5],"<=\0",2))
        {
          // swap operands and switch compare
          strcpy(cstatement[k+5],cstatement[k+4]);
          strcpy(cstatement[k+4],cstatement[k+6]);
          strcpy(cstatement[k+6],cstatement[k+5]);
          strcpy(cstatement[k+5],">=");
        }

        for (i=2;i<48-k;++i) strcpy(orstatement[i],cstatement[k+i+2]);
        if (!strncmp(cstatement[k+5],"then\0",4)) compressdata(cstatement,k+3,k+2);
        else if (!strncmp(cstatement[k+7],"then\0",4)) compressdata(cstatement,k+3,k+4);
        strcpy(cstatement[k+3],"then");
        sprintf(orstatement[0],"%dOR",ors++);
        strcpy(orstatement[1],"if");
        door=1;
// todo: need to skip over the next statement!

      }
      else if (!strncmp(cstatement[k+5],"||\0",2))
      {
        if (!strncmp(cstatement[k+7],">\0",2))
        {
          // swap operands and switch compare
          strcpy(cstatement[k+7],cstatement[k+6]); // stick 1st operand here temporarily
          strcpy(cstatement[k+6],cstatement[k+8]);
          strcpy(cstatement[k+8],cstatement[k+7]);// get it back
          strcpy(cstatement[k+7],"<"); // replace compare
        }
        else if (!strncmp(cstatement[k+7],"<=\0",2))
        {
          // swap operands and switch compare
          strcpy(cstatement[k+7],cstatement[k+6]);
          strcpy(cstatement[k+6],cstatement[k+8]);
          strcpy(cstatement[k+8],cstatement[k+7]);
          strcpy(cstatement[k+7],">=");
        }
        for (i=2;i<46-k;++i) strcpy(orstatement[i],cstatement[k+i+4]);
        if (!strncmp(cstatement[k+7],"then\0",4)) compressdata(cstatement,k+5,k+2);
        else if (!strncmp(cstatement[k+9],"then\0",4)) compressdata(cstatement,k+5,k+4);
        strcpy(cstatement[k+5],"then");
        sprintf(orstatement[0],"%dOR",ors++);
        strcpy(orstatement[1],"if");
        door=1;
      }
    }
    if (door) break;
  }
  if (foundelse)
  {
    if (door) pass2elstatement=orstatement;
    else pass2elstatement=cstatement;
    for (i=1;i<50;++i) 
      if (!strncmp(pass2elstatement[i],"else\0",4)) 
      {
	foundelse=i; 
	break;
      }

    for (i=foundelse;i<50;++i) strcpy(elstatement[i-foundelse],pass2elstatement[i]);
    if (islabelelse(pass2elstatement))
    {
      strcpy(pass2elstatement[foundelse++],":");
      strcpy(pass2elstatement[foundelse++],"goto");
      sprintf(pass2elstatement[foundelse++],"skipelse%d",numelses);
    }
    for (i=foundelse;i<50;++i) pass2elstatement[i][0]='\0';
    if (!islabelelse(elstatement))
    {
      strcpy(elstatement[2],elstatement[1]);
      strcpy(elstatement[1],"goto");
    }
    if (door)
    { 
      for (i=1;i<50;++i) 
        if (!strncmp(cstatement[i],"else\0",4)) break;
      for (k=i;k<50;++k) cstatement[k][0]='\0'; 
    }

  }


  if (door)
  {
    swapstatement=orstatement; // swap statements because of recursion
    orstatement=cstatement;
    cstatement=swapstatement;
    // this hacks off the conditional statement from the copy of the statement we just created
   // and replaces it with a goto.  This can be improved (i.e., there is no need to copy...)
    if (islabel(orstatement)) 
    {
      // find end of statement
      i=3;
//      while (orstatement[i++][0]){} // not sure if this will work...
      while (strncmp(orstatement[i++],"then\0",4)){} // not sure if this will work...
      // add goto to it
      if (i>40) {i=40;fprintf(stderr,"%d: Cannot find end of line - statement may have been truncated\n",linenum());}
      //strcpy(orstatement[i++],":");
      strcpy(orstatement[i++],"goto");
      sprintf(orstatement[i++],"condpart%d",getcondpart()+1); // goto unnamed line number for then statemtent
      for (;i<50;++i) orstatement[i][0]='\0'; // clear out rest of statement
    }
//for (i=0;i<50;++i)printf("%s ",cstatement[i]);
//printf("\n");
//for (i=0;i<50;++i)printf("%s ",orstatement[i]);
//printf("\n");


    keywords(orstatement); // recurse
  }
  if (foundelse)
  {
    swapstatement=elstatement; // swap statements because of recursion
    elstatement=cstatement;
    cstatement=swapstatement;
    keywords(elstatement); // recurse
  }
  for (i=0;i<50;++i)
  {
    statement[i]=cstatement[i];
    if (statement[i][0]!='\0') if (statement[i][0]==':') colons++;

//printf("%s ",cstatement[i]);
  }
//printf("\n");


  if (!strncmp(statement[0],"then\0",4)) 
    sprintf(statement[0],"%dthen",numthens++);
  invalidate_Areg();
  while (1) {

//for (i=0;i<50;++i){for(j=0;j<10;++j)printf("%x ",statement[i][j]);
//printf("\n");}



    i=0;
    if (statement[1][0]=='\0') {return;}
    else if (!strncmp(statement[0],"end\0",3)) endfunction();
    else if (!strncmp(statement[1],"includesfile\0",12)) create_includes(statement[2]);
    else if (!strncmp(statement[1],"include\0",7)) add_includes(statement[2]);
    else if (!strncmp(statement[1],"inline\0",6)) add_inline(statement[2]);
    else if (!strncmp(statement[1],"function\0",8)) function(statement);
    else if (statement[1][0]==' ') {return;}
    else if (!strncmp(statement[1],"if\0",2)) {doif(statement);break;}
    else if (!strncmp(statement[1],"goto\0",4)) dogoto(statement);
    else if (!strncmp(statement[1],"bank\0",4)) newbank(atoi(statement[2]));
    else if (!strncmp(statement[1],"sdata\0",5)) sdata(statement);
    else if (!strncmp(statement[1],"data\0",4)) data(statement);
    else if ((!strncmp(statement[1],"on\0",2)) &&
	     (!strncmp(statement[3],"goto\0",4))) ongoto(statement);
    else if (!strncmp(statement[1],"const\0",5)) doconst(statement);
    else if (!strncmp(statement[1],"dim\0",3)) dim(statement);
    else if (!strncmp(statement[1],"for\0",3)) dofor(statement);
    else if (!strncmp(statement[1],"next\0",4)) next(statement);
    else if (!strncmp(statement[1],"gosub\0",5)) gosub(statement);
    else if (!strncmp(statement[1],"pfpixel\0",7)) pfpixel(statement);
    else if (!strncmp(statement[1],"pfhline\0",7)) pfhline(statement);
    else if (!strncmp(statement[1],"pfclear\0",7)) pfclear(statement);
    else if (!strncmp(statement[1],"pfvline\0",7)) pfvline(statement);
    else if (!strncmp(statement[1],"pfscroll\0",8)) pfscroll(statement);
    else if (!strncmp(statement[1],"drawscreen\0",10)) drawscreen();
    else if (!strncmp(statement[1],"asm\0",3)) doasm();
    else if (!strncmp(statement[1],"pop\0",3)) dopop();
    else if (!strncmp(statement[1],"rem\0",3)) {rem(statement);return;}
    else if (!strncmp(statement[1],"set\0",3)) set(statement);
    else if (!strncmp(statement[1],"return\0",6)) doreturn(statement);
    else if (!strncmp(statement[1],"reboot\0",6)) doreboot();
    else if (!strncmp(statement[1],"vblank\0",6)) vblank();
    else if ((!strncasecmp(statement[1],"pfcolors:\0",9)) 
            || (!strncasecmp(statement[1],"pfheights:\0",9))) playfieldcolorandheight(statement); 
    else if (!strncmp(statement[1],"playfield:\0",10)) playfield(statement); 
    else if (!strncmp(statement[1],"lives:\0",6)) lives(statement); 
    else if ((!strncmp(statement[1],"player0:\0",8)) 
            || (!strncmp(statement[1],"player1:\0",8))
            || (!strncmp(statement[1],"player2:\0",8))
            || (!strncmp(statement[1],"player3:\0",8))
            || (!strncmp(statement[1],"player4:\0",8))
            || (!strncmp(statement[1],"player5:\0",8))
            || (!strncmp(statement[1],"player0color:\0",13))
            || (!strncmp(statement[1],"player1color:\0",13))) player(statement);
    else if (!strncmp(statement[2],"=\0",1)) let(statement);
    else if (!strncmp(statement[1],"let\0",3)) let(statement);
    else {
      sprintf(errorcode, "Error: Unknown keyword: %s\n", statement[1]);
      prerror(&errorcode[0]);
      exit(1);
    }
    // see if there is a colon
    if ((!colons) || (currentcolon==colons)) break;
    currentcolon++;
    i=0;k=0;
    while (i!=currentcolon) {
      if (cstatement[k++][0]==':') i++;
    }

    for (j=k;j<50;++j) statement[j-k+1]=cstatement[j];
    

  }
  if (foundelse)
    printf(".skipelse%d\n",numelses++);
}
