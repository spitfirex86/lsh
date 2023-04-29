#include "external.h"
#include "parse.h"
#include "var.h"


#define PROMPT() printf("(%s); ", libName)
#define PROMPT_SECTION(psct,name) printf("  %d: (%s?); ", (psct)->nCxts, (name));


char libPath[MAX_PATH] = "";
char libName[MAX_PATH] = "";
HMODULE hLib = NULL;

BOOL printRet = TRUE;
BOOL exitShell = FALSE;


void freeSection( SaveSection *section );
void execSection( Context *cxt, SaveSection *section );
void readSection( Context *sectionCxt );


void pathToLibName( char *out, char *path )
{
	char *tmp;
	int nChars;

	if ( tmp = strrchr(path, '\\') )
		path = tmp + 1;
	
	if ( tmp = strrchr(path, '.') )
		nChars = tmp - path;
	else
		nChars = strlen(path);

	strncpy(out, path, nChars);
	out[nChars] = 0;
}


BOOL loadLib( char *name )
{
	HMODULE newLib = LoadLibrary(name);
	if ( newLib )
	{
		FreeLibrary(hLib);
		hLib = newLib;
		strcpy(libPath, name);
		pathToLibName(libName, name);
		return TRUE;
	}
	else
	{
		printf("Cannot load library '%s'\n", name);
		return FALSE;
	}
}

void printLastRet( void )
{
	if ( printRet )
		printf("Return: %d (0x%X)\n\n", lastRet, lastRet);
}

void setVarInSuper( Context *cxt, int value, int flags )
{
	if ( SUPER_IS_VAR(cxt) )
	{
		if ( !setVar(SUPER(cxt), value, flags) )
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
		setVarInSuper(cxt, result, 0);
		printLastRet();

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
		setVarInSuper(cxt, lastRet, 0);
		printLastRet();
	}
}

void varAction( Context *cxt )
{
	Context *sectionCxt;
	Var *localVars_save;
	int nLocalVars_save;

	Var *var = getVar(ACTION(cxt));
	if ( var )
	{
		if ( var->flags & VFLAG_SECTION )
		{
			int nParams = cxt->nParams;
			char **params = paramsToList(cxt);
			int i;

			sectionCxt = parserInit();
			localVars_save = localVars;
			nLocalVars_save = nLocalVars;

			localVars = calloc(nParams, sizeof(Var));
			nLocalVars = nParams;
			for ( i = 0; i < nParams; ++i )
			{
				Var *local = &localVars[i];
				sprintf(local->name, "?%d", i+1);
				local->value = params[i];
			}

			printRet = FALSE;
			execSection(sectionCxt, var->value);
			setVarInSuper(cxt, lastRet, 0);
			printRet = TRUE;
			printLastRet();

			free(localVars);
			localVars = localVars_save;
			nLocalVars = nLocalVars_save;
			parserFree(sectionCxt);
		}
		else
		{
			lastRet = var->value;
			setVarInSuper(cxt, var->value, 0);
			printf("%s: %d (0x%X)\n\n", var->name, var->value, var->value);
		}
	}
	else
		printf("Error: var '%s' does not exist\n", ACTION(cxt));
}

void execute( Context *cxt )
{
	if ( ACTION_IS(cxt, "Exit") )
		exitShell = TRUE;
	else if ( ACTION_IS_SPECIAL(cxt) )
		directiveAction(cxt);
	else if ( ACTION_IS_VAR(cxt) )
		varAction(cxt);
	else if ( *ACTION(cxt) )
		runProc(cxt);
}

void freeSection( SaveSection *section )
{
	SaveCxt *saveCxt;
	int i;

	for ( i = 0; i < section->nCxts; ++i )
	{
		free(ACTION(saveCxt));
		free(SUPER(saveCxt));
		free(saveCxt->params);
	}

	free(section->cxts);
	free(section);
}

void execSection( Context *cxt, SaveSection *section )
{
	SaveCxt *saveCxt;
	int i;

	for ( i = 0; i < section->nCxts; ++i )
	{
		saveCxt = &section->cxts[i];

		strcpy(ACTION(cxt), ACTION(saveCxt));
		strcpy(SUPER(cxt), SUPER(saveCxt));

		cxt->nParams = saveCxt->nParams;
		if ( saveCxt->nParams )
			memcpy(cxt->params, saveCxt->params, 256);

		execute(cxt);
	}
}

void readSection( Context *sectionCxt )
{
	SaveSection *section = sectionCxt->section;
	SaveCxt *saveCxt;
	Context *cxt;
	char buffer[256];
	char newLibName[MAX_PATH];
	int i;

	cxt = parserInit();

	strcpy(newLibName, libName);
	PROMPT_SECTION(section, newLibName);

	while ( fgets(buffer, sizeof(buffer), stdin) != NULL )
	{
		if ( parse(cxt, buffer) )
		{
			if ( ACTION_IS_SECTION_END(cxt) )
				break;

			if ( ACTION_IS_SPECIAL(cxt) && ACTION_IS(cxt, "$Lib") && cxt->nParams )
				pathToLibName(newLibName, PARAM(cxt));

			i = section->nCxts++;
			section->cxts = realloc(section->cxts, section->nCxts * sizeof(SaveCxt));
			saveCxt = &section->cxts[i];

			ACTION(saveCxt) = malloc(strlen(cxt->action)+1);
			strcpy(ACTION(saveCxt), ACTION(cxt));

			saveCxt->nParams = cxt->nParams;
			if ( cxt->nParams )
			{
				saveCxt->params = malloc(256);
				memcpy(saveCxt->params, cxt->params, 256);
			}

			SUPER(saveCxt) = malloc(strlen(cxt->super)+1);
			strcpy(SUPER(saveCxt), SUPER(cxt));
		}
		PROMPT_SECTION(section, newLibName);
	}

	if ( SUPER(sectionCxt) )
	{
		setVarInSuper(sectionCxt, section, VFLAG_SECTION);
	}
	else
	{
		printRet = FALSE;

		execSection(cxt, section);
		freeSection(section);

		printRet = TRUE;
		printLastRet();
	}

	parserFree(cxt);
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
			if ( ACTION_IS_SECTION(cxt) )
				readSection(cxt);
			else
				execute(cxt);
		}

		if ( exitShell )
			break;

		PROMPT();
	}

	parserFree(cxt);
	FreeLibrary(hLib);
	return 0;
}
