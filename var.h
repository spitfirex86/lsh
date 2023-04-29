#pragma once


typedef struct Var
{
	char name[64];
	int value;
	unsigned int flags;
}
Var;


#define MAX_VARS 20

#define VFLAG_SECTION	2


extern int lastRet;

extern Var vars[MAX_VARS];
extern int nVars;

extern Var *localVars;
extern int nLocalVars;


Var * getVar( char *name );
BOOL setVar( char *name, int value, unsigned int flags );
