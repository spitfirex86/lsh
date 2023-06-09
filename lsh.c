#include "external.h"
#include "parse.h"
#include "var.h"
#include "builtin.h"


#define PROMPT()						\
{										\
	if ( !usingFile )					\
		printf("(%s); ", libName);		\
}

#define PROMPT_SECTION(psct,name)							\
{															\
	if ( !usingFile )										\
		printf("  %d: (%s?); ", (psct)->nCxts, (name));		\
}


FILE *inStream;
BOOL usingFile = FALSE;

char libPath[MAX_PATH] = "";
char libName[MAX_PATH] = "";
HMODULE hLib = NULL;

int suppressRet = 0;
BOOL exitShell = FALSE;


void execSection( Context *cxt, Section *section );
void readSection( Section *section );

void execSpecialSection( Context *sectionCxt, Section *section );
Section * readSpecialSection( Context *sectionCxt );


void execute( Context *cxt );


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
	HMODULE newLib;

	if ( !_stricmp(name, libPath) )
		return TRUE;

	if ( *name == 0 || !_stricmp(name, " ") )
	{
		FreeLibrary(hLib);
		*libPath = 0;
		*libName = 0;
		return TRUE;
	}

	if ( newLib = LoadLibrary(name) )
	{
		FreeLibrary(hLib);
		hLib = newLib;
		strcpy(libPath, name);
		pathToLibName(libName, name);
		return TRUE;
	}
	else
	{
		printf("Error: cannot load library '%s'\n", name);
		return FALSE;
	}
}

void printLastRet( void )
{
	if ( !suppressRet )
		printf("Return: %d (0x%X)\n\n", lastRet, lastRet);
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
		printLastRet();

		free(params);
		return TRUE;
	}
	else
	{
		printf("Error: cannot find procedure '%s' in lib '%s'\n", ACTION(cxt), libName);
		return FALSE;
	}
}

void directiveAction( Context *cxt )
{
	if ( ACTION_NP_IS(cxt, "Debug") )
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
	else if ( ACTION_NP_IS(cxt, "Lib") )
	{
		if ( cxt->nParams )
			loadLib(PARAM(cxt));
		else
			printf("Usage: $Lib(\"pathToDll\")\n");
	}
	else if ( ACTION_NP_IS(cxt, "Ret") )
	{
		setVarInSuper(cxt, lastRet);
		printLastRet();
	}
	else
	{
		BOOL match = builtin_cond(cxt) || builtin_oper(cxt);

		if ( !match )
			printf("Error: unknown directive '%s'\n", ACTION(cxt));
	}
}

void varAction( Context *cxt )
{
	Var *var = getVar(ACTION(cxt));
	if ( var )
	{
		lastRet = var->value;
		setVarInSuper(cxt, var->value);
		if ( !suppressRet )
			printf("%s: %d (0x%X)\n\n", var->name, var->value, var->value);
	}
	else
		printf("Error: var '%s' does not exist\n", ACTION(cxt));
}

void callSection( Context *sectionCxt )
{
	Context *cxt;
	Var *localVars_save;
	int nLocalVars_save;

	Section *section = getSection(ACTION_NP(sectionCxt));
	if ( section )
	{
		int nParams = sectionCxt->nParams;
		char **params = paramsToList(sectionCxt);
		int i;

		cxt = parserInit();
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

		++suppressRet;
		execSection(cxt, section);
		setVarInSuper(sectionCxt, lastRet);
		--suppressRet;
		printLastRet();

		free(localVars);
		localVars = localVars_save;
		nLocalVars = nLocalVars_save;
		parserFree(cxt);
		free(params);
	}
	else
		printf("Error: section '%s' does not exist\n", ACTION_NP(sectionCxt));
}

void execSection( Context *cxt, Section *section )
{
	SaveCxt *saveCxt;
	char saveLib[MAX_PATH];
	int i;

	strcpy(saveLib, libPath);

	for ( i = 0; i < section->nCxts; ++i )
	{
		saveCxt = &section->cxts[i];

		strcpy(ACTION(cxt), ACTION(saveCxt));
		strcpy(SUPER(cxt), SUPER(saveCxt));

		cxt->nParams = saveCxt->nParams;
		if ( saveCxt->nParams )
			memcpy(cxt->params, saveCxt->params, 256);

		if ( ACTION_IS_SECTION(cxt) && SECTION_IS_SPECIAL(cxt) )
			execSpecialSection(cxt, saveCxt->section);
		else
			execute(cxt);
	}

	loadLib(saveLib);
}

void execSpecialSection( Context *sectionCxt, Section *section )
{
	Context *cxt;
	BOOL cond;

	if ( ACTION_NP_IS(sectionCxt, "$If") )
	{
		if ( sectionCxt->nParams )
		{
			char **param = paramsToList(sectionCxt);
			cond = param[0];
			free(param);
		}
		else
			cond = lastRet;

		if ( cond )
		{
			cxt = parserInit();

			++suppressRet;
			execSection(cxt, section);
			setVarInSuper(sectionCxt, lastRet);
			--suppressRet;
			printLastRet();

			parserFree(cxt);
		}
	}
}

void readSection( Section *section )
{
	SaveCxt *saveCxt;
	Context *cxt;
	Section *specialSection;
	char buffer[256];
	char newLibName[MAX_PATH];
	int i;

	cxt = parserInit();

	strcpy(newLibName, libName);
	PROMPT_SECTION(section, newLibName);

	while ( fgets(buffer, sizeof(buffer), inStream) != NULL )
	{
		specialSection = NULL;
		if ( parse(cxt, buffer) )
		{
			if ( ACTION_IS_SECTION(cxt) )
			{
				if ( !SECTION_IS_SPECIAL(cxt) )
				{
					printf("Error: nested procedures are not allowed\n");
					goto _next;
				}

				specialSection = readSpecialSection(cxt);
				if ( !specialSection )
					goto _next;
			}

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

			saveCxt->section = specialSection;
		}
_next:
		PROMPT_SECTION(section, newLibName);
	}

	parserFree(cxt);
}

void readProcSection( Context *sectionCxt )
{
	Section *section;

	if ( !*ACTION_NP(sectionCxt) )
	{
		printf("Error: section name cannot be empty\n");
		parserPrevLevel();
		return;
	}

	section = createStaticSection(ACTION_NP(sectionCxt));
	if ( !section )
	{
		printf("Error: cannot create section '%s'\n", ACTION_NP(sectionCxt));
		parserPrevLevel();
		return;
	}

	readSection(section);
}

Section * readSpecialSection( Context *sectionCxt )
{
	Section *section = NULL;

	if ( ACTION_NP_IS(sectionCxt, "$If") )
	{
		if ( sectionCxt->nParams > 1 )
		{
			printf("Error: expected 1 parameter, got %d\n", sectionCxt->nParams);
			parserPrevLevel();
			return NULL;
		}

		section = createDynamicSection();
		readSection(section);
		return section;
	}
	else
	{
		printf("Error: unknown section directive '%s'\n", ACTION_NP(sectionCxt));
		parserPrevLevel();
		return NULL;
	}
}

void readAndExecSpecialSection( Context *sectionCxt )
{
	Section *section;

	section = readSpecialSection(sectionCxt);
	if ( section )
	{
		execSpecialSection(sectionCxt, section);
		freeDynamicSection(section);
	}
}


void execute( Context *cxt )
{
	if ( ACTION_IS(cxt, "Exit") )
		exitShell = TRUE;
	else if ( ACTION_IS_SPECIAL(cxt) )
		directiveAction(cxt);
	else if ( ACTION_IS_VAR(cxt) )
		varAction(cxt);
	else if ( ACTION_IS_CALL(cxt) )
		callSection(cxt);
	else if ( *ACTION(cxt) )
		runProc(cxt);
}

int main( int argc, char **argv )
{
	char buffer[256];
	Context *cxt;

	//loadLib("kernel32.dll");
	//if ( argc > 1 )
	//	loadLib(argv[1]);

	inStream = stdin;

	if ( argc > 1 )
	{
		FILE *file = fopen(argv[1], "r");
		if ( file )
		{
			usingFile = TRUE;
			inStream = file;
			suppressRet++;
		}
	}

	cxt = parserInit();

	PROMPT();
	while ( fgets(buffer, sizeof(buffer), inStream) != NULL )
	{
		if ( parse(cxt, buffer) )
		{
			if ( ACTION_IS_SECTION(cxt) )
			{
				if ( SECTION_IS_SPECIAL(cxt) )
					readAndExecSpecialSection(cxt);
				else
					readProcSection(cxt);
			}
			else
				execute(cxt);
		}

		if ( exitShell )
			break;

		PROMPT();
	}

	parserFree(cxt);
	FreeLibrary(hLib);

	if ( usingFile )
	{
		fclose(inStream);
		suppressRet--;
		printLastRet();
	}

	return lastRet;
}
