#include "external.h"
#include "var.h"


int lastRet = 0;

Var vars[MAX_VARS] = { 0 };
int nVars = 0;

Var *localVars = NULL;
int nLocalVars = 0;


Var * getVar( char *name )
{
	int i;

	if ( localVars )
	{
		for ( i = 0; i < nLocalVars; ++i )
		{
			if ( !_stricmp(name, localVars[i].name) )
				return &localVars[i];
		}
	}

	for ( i = 0; i < nVars; ++i )
	{
		if ( !_stricmp(name, vars[i].name) )
			return &vars[i];
	}

	return NULL;
}

BOOL setVar( char *name, int value, unsigned int flags )
{
	Var *var = getVar(name);
	if ( !var )
	{
		if ( nVars == MAX_VARS )
			return FALSE;

		var = &vars[nVars++];
		strncpy(var->name, name, sizeof(var->name));
		var->name[sizeof(var->name)-1] = 0;
	}

	var->value = value;
	var->flags = flags;
	return TRUE;
}
