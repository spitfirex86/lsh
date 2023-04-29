#pragma once


typedef struct Var
{
	char name[64];
	int value;
}
Var;

typedef struct SaveCxt
{
	char *action;
	char *params;
	char *super;
	int nParams;
}
SaveCxt;

typedef struct Section
{
	char name[64];
	SaveCxt *cxts;
	int nCxts;
}
Section;


#define MAX_VARS 20


extern int lastRet;

extern Var vars[MAX_VARS];
extern int nVars;

extern Var *localVars;
extern int nLocalVars;


Var * getVar( char *name );
BOOL setVar( char *name, int value );

Section * getSection( char *name );
Section * getCreateSection( char *name );
