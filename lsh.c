#include "external.h"
#include "parse.h"
#include "var.h"


#define PROMPT() printf("(%s); ", libName)


char libName[MAX_PATH] = "";
HMODULE hLib = NULL;




BOOL loadLib( char *name )
{
	char *tmp;
	int nChars;

	HMODULE newLib = LoadLibrary(name);
	if ( newLib )
	{
		FreeLibrary(hLib);
		hLib = newLib;

		if ( tmp = strrchr(name, '\\') )
			name = tmp + 1;
		
		if ( tmp = strrchr(name, '.') )
			nChars = tmp - name;
		else
			nChars = strlen(name);

		strncpy(libName, name, nChars);
		libName[nChars] = 0;

		return TRUE;
	}
	else
	{
		printf("Cannot load library '%s'\n", name);
		return FALSE;
	}
}

void setVarInSuper( Context *cxt, int value )
{
	if ( SUPER_IS_VAR(cxt) )
	{
		if ( !setVar(SUPER(cxt), value) )
			printf("Error: cannot set var '%s'\n", SUPER(cxt));
	}
}

BOOL runProc( Context *cxt )
{
	int (*proc)() = (int(*)())GetProcAddress(hLib, ACTION(cxt));
	if ( proc )
	{
		int nParams = cxt->nParams;
		char **params = paramsToList(cxt);
		unsigned espSave;
		int result;

		__asm
		{
			mov espSave, esp
			mov ecx, nParams
			lea eax, [ecx*4]
			sub esp, eax
			mov esi, [params]
			mov edi, esp
			rep movsd
			call proc
			mov result, eax
			mov esp, espSave
		};

		lastRet = result;
		setVarInSuper(cxt, result);	
		printf("Return: %d (0x%X)\n\n", result, result);

		free(params);
		return TRUE;
	}
	else
	{
		printf("Cannot find procedure '%s' in lib '%s'\n", ACTION(cxt), libName);
		return FALSE;
	}
}

void directiveAction( Context *cxt )
{
	if ( ACTION_IS(cxt, "$Debug") )
	{
		int i;
		char *param;
		printf("Action: '%s'\n", ACTION(cxt));
		printf("Super: '%s'\n", SUPER(cxt));
		printf("Nb params: %d\n", cxt->nParams);
		param = PARAM(cxt);
		for ( i = 0; i < cxt->nParams; ++i )
		{
			printf("  Param %d: type %d, '%s'\n", i, PARAM_TYPE(param), param);
			param = PARAM_NEXT(param);
		}
	}
	else if ( ACTION_IS(cxt, "$Lib") )
	{
		if ( cxt->nParams )
			loadLib(PARAM(cxt));
		else
			printf("Usage: $Lib(\"pathToDll\")\n");
	}
	else if ( ACTION_IS(cxt, "$Ret") )
	{
		setVarInSuper(cxt, lastRet);
		printf("Return: %d (0x%X)\n\n", lastRet, lastRet);
	}
}

void varAction( Context *cxt )
{
	Var *var = getVar(ACTION(cxt));
	if ( var )
	{
		setVarInSuper(cxt, var->value);
		printf("%s: %d (0x%X)\n\n", var->name, var->value, var->value);
	}
	else
		printf("Error: var '%s' does not exist\n", ACTION(cxt));
}


int main( int argc, char** argv )
{
	char buffer[256];
	Context *cxt;

	//loadLib("kernel32.dll");
	if ( argc > 1 )
		loadLib(argv[1]);

	cxt = parserInit();

	PROMPT();
	while ( fgets(buffer, sizeof(buffer), stdin) != NULL )
	{
		if ( parse(cxt, buffer) )
		{
			if ( ACTION_IS(cxt, "Exit") )
				break;
			else if ( ACTION_IS_SPECIAL(cxt) )
				directiveAction(cxt);
			else if ( ACTION_IS_VAR(cxt) )
				varAction(cxt);
			else if ( *ACTION(cxt) )
				runProc(cxt);
		}
		PROMPT();
	}

	parserFree(cxt);
	FreeLibrary(hLib);
	return 0;
}
