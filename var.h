#pragma once


typedef struct Var Var;
typedef struct SaveCxt SaveCxt;
typedef struct Section Section;


struct Var
{
	char name[64];
	int value;
};

struct SaveCxt
{
	char *action;
	char *params;
	char *super;
	int nParams;
	Section *section;
};

struct Section
{
	char name[64];
	SaveCxt *cxts;
	int nCxts;
};


#define MAX_VARS 20


extern int lastRet;

extern Var vars[MAX_VARS];
extern int nVars;

extern Var *localVars;
extern int nLocalVars;


Var * getVar( char *name );
BOOL setVar( char *name, int value );

Section * getSection( char *name );
Section * createStaticSection( char *name );
void freeSectionInner( Section *section );

Section * createDynamicSection( void );
void freeDynamicSection( Section *section );
