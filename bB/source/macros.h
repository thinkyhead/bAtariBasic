typedef unsigned char BOOL;
#define false 0
#define true  1

#define _BV(N) (1 << (N))
#define MATCH(VAR,STR) (!strncmp(VAR, STR, strlen(STR)))
#define SMATCH(IND,STR) MATCH(statement[IND], STR)
#define IMATCH(IND,STR) (!strncasecmp(statement[IND], STR, strlen(STR)))
#define CMATCH(IND,CHR) (statement[IND][0] == CHR)
#define COUNT(X) (sizeof(X)/sizeof(*X))
