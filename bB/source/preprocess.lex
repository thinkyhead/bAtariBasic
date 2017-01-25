%{
#include <stdlib.h>  
int linenumber=1;
//void yyerror(char *);  
%}    
%x comment
%x asm
%x data
%x player
%x lives
%x playercolor
%x playfield
%x pfcolors
%x pfheights
%x includes
%x collision
%%    
[ \t]+ putchar(' ');
[ \t\r]+$

"rem" {printf("rem");BEGIN(comment);}
<comment>^\n* printf("%s",yytext);
<comment>\n {linenumber++;printf("\n");BEGIN(INITIAL);}

"asm" {printf("%s",yytext);BEGIN(asm);}
<asm>^"\nend" printf("%s",yytext);
<asm>"\nend" {linenumber++;printf("\nend");BEGIN(INITIAL);}
<asm>"\n" {linenumber++;printf("\n");}

"data" {printf("%s",yytext);BEGIN(data);}
<data>^"\nend" printf("%s",yytext);
<data>"\nend" {linenumber++;printf("\nend");BEGIN(INITIAL);}
<data>"\n" {linenumber++;printf("\n");}

"include" {printf("%s",yytext);BEGIN(includes);}
<includes>^\n* printf("%s",yytext);
<includes>\n {linenumber++;printf("\n");BEGIN(INITIAL);}

"player"[012345]: {printf("%s",yytext);BEGIN(player);}
<player>^"\nend" printf("%s",yytext);
<player>"\nend" {linenumber++;printf("\nend");BEGIN(INITIAL);}
<player>"\n" {linenumber++;printf("\n");}

"lives:" {printf("%s",yytext);BEGIN(lives);}
<lives>^"\nend" printf("%s",yytext);
<lives>"\nend" {linenumber++;printf("\nend");BEGIN(INITIAL);}
<lives>"\n" {linenumber++;printf("\n");}

"player"[01]"color:" {printf("%s",yytext);BEGIN(playercolor);}
<playercolor>^"\nend" printf("%s",yytext);
<playercolor>"\nend" {linenumber++;printf("\nend");BEGIN(INITIAL);}
<playercolor>"\n" {linenumber++;printf("\n");}

"playfield:" {printf("%s",yytext);BEGIN(playfield);}
<playfield>[ \t]+ ;
<playfield>^"\nend" printf(" %s",yytext);
<playfield>"\nend" {linenumber++;printf("\nend");BEGIN(INITIAL);}
<playfield>"\n" {linenumber++;printf("\n");}

[pP][fF]"colors:" {printf("%s",yytext);BEGIN(pfcolors);}
<pfcolors>[ \t]+ printf(" ");
<pfcolors>^"\nend" printf(" %s",yytext);
<pfcolors>"\nend" {linenumber++;printf("\nend");BEGIN(INITIAL);}
<pfcolors>"\n" {linenumber++;printf("\n");}

[pP][fF]"heights:" {printf("%s",yytext);BEGIN(pfheights);}
<pfheights>[ \t]+ printf(" ");
<pfheights>^"\nend" printf(" %s",yytext);
<pfheights>"\nend" {linenumber++;printf("\nend");BEGIN(INITIAL);}
<pfheights>"\n" {linenumber++;printf("\n");}

"collision(" {printf("%s",yytext);BEGIN(collision);}
<collision>" "+
<collision>":\t\n"+ {fprintf(stderr,"%d: Missing )\n",linenumber);exit(1);}
<collision>^")" printf("%s",yytext);
<collision>")" {printf("%s",yytext);BEGIN(INITIAL);}

"step"[ ]+"-" printf("step -",yytext);
"#"            printf("%s", yytext);  
"$"            printf("%s", yytext);  
"%"            printf("%s", yytext);  
"["            printf("%s", yytext);  
"]"            printf("%s", yytext);  
"!"            printf("%s", yytext);  
"."            printf("%s", yytext);  
"_"            printf("%s", yytext);  
"{"          printf("%s", yytext);  
"}"          printf("%s", yytext);  


","            printf(" %s ", yytext);  
"("            printf(" %s ", yytext);  
")"            printf(" %s ", yytext);  
">="            printf(" %s ", yytext);  
"<="            printf(" %s ", yytext);  
"="            printf(" %s ", yytext);  
"<>"            printf(" %s ", yytext);  
"<"            printf(" %s ", yytext);  
"+"            printf(" %s ", yytext);  
"-"            printf(" %s ", yytext);  
"/"+            printf(" %s ", yytext);  
"*"+            printf(" %s ", yytext);  
">"            printf(" %s ", yytext);  
":"            printf(" %s ", yytext);  
"&"+          printf(" %s ", yytext);  
"|"+          printf(" %s ", yytext);  
"^"          printf(" %s ", yytext);  

[A-Z]+ printf("%s",yytext);
[a-z]+       {       printf("%s", yytext);}
[0-9]+      {       printf("%s", yytext);}
[\n] {printf("\n"); linenumber++;}
.               {fprintf(stderr,"(%d) Parse error: unrecognized character \"%s\"\n",linenumber,yytext);  exit(1);}
%%
  int yywrap(void) {      return 1;  } 
main(){yylex();}
