#pragma once


typedef struct Var
{
	char name[64];
	int value;
}
Var;


#define MAX_VARS 20


extern int lastRet;

extern Var vars[MAX_VARS];
extern int nVars;


Var * getVar( char *name );
BOOL setVar( char *name, int value );
