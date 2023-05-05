#include "external.h"
#include "parse.h"
#include "var.h"


#define PTYPE_ANY		0
#define PTYPE_STRING	1
#define PTYPE_SPECIAL	2
#define PTYPE_VAR		3

#define IS_VALID(pch) (validChars[*(pch)] & 1)
#define IS_SPACE(pch) (validChars[*(pch)] & 2)
#define IS_BEGIN(pch) (validChars[*(pch)] & 4)
#define IS_NUM(pch) (validChars[*(pch)] & 8)

#define SKIP_SPACE(pch) {		\
	while ( IS_SPACE(pch) )		\
		++(pch);				\
}


char validChars[128] = {
	0,0,0,0,0,0,0,0,0,/*\t*/2,/*\n*/2,0,0,/*\r*/2,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	/* */2,/*!*/0,0,0,/*$*/4,/*%*/0,/*&*/0,0,/*(*/0,/*)*/0,0,0,0,0,0,0,
	/*0-9*/9,9,9,9,9,9,9,9,9,9,/*:*/4,/*;*/0,0,0,0,/*?*/4,
	/*@*/1,/*A-Z*/5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	0,0,0,0,/*_*/5,
	0,/*a-z*/5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	/*{*/0,0,/*}*/0,0,0
};

/* section */
BOOL parserInSection = FALSE;
//SaveSection *currentSection = NULL;


char * parse_validWord( char *out, char *ch )
{
	while ( IS_VALID(ch) )
		*out++ = *ch++;
	*out = 0;
	SKIP_SPACE(ch);
	return ch;
}

char * parse_params( Context *cxt, char *ch )
{
	int n = 0;
	char *out = PARAM(cxt);
	char *outp = out;
	PARAM_TYPE(outp) = PTYPE_ANY;

	while ( *ch != ')' && *ch )
	{
		++ch;
		SKIP_SPACE(ch);

		switch ( *ch )
		{
		case ')':
			if ( out == PARAM(cxt) ) /* no params */
				break;
		case ',':
			if ( out == outp )
				*out++ = ' ';
			*out++ = 0;
			*out++ = 0; /* type */
			outp = out;
			++n;
			break;

		case '"':
			++ch;
			PARAM_TYPE(outp) = PTYPE_STRING;
			while ( *ch != '"' && *ch )
				*out++ = *ch++;
			if ( out == outp )
				*out++ = ' ';

			if ( *ch != '"' )
				return 0;
			break;

		case '$':
			if ( out == outp )
				PARAM_TYPE(outp) = PTYPE_SPECIAL;
			goto _outch;

		case '?':
			if ( out == outp )
				PARAM_TYPE(outp) = PTYPE_VAR;
			goto _outch;

		default:
		_outch:
			*out++ = *ch;
			break;

		case 0:
			return 0;
		}
	}

	cxt->nParams = n;
	++ch;
	SKIP_SPACE(ch);
	return ch;
}

char * parse_super( Context *cxt, char *ch )
{
	char *out = SUPER(cxt);
	char *chSave = ch;

	if ( *ch == '?' ) /* super: var assign */
	{
		*out++ = *ch++;
		ch = parse_validWord(out, ch);

		if ( *ch == '=' )
			ch++;
		else /* discard super */
		{
			*cxt->super = 0;
			ch = chSave;
		}
	}

	SKIP_SPACE(ch);
	return ch;
}

char * parse_section( Context *cxt, char *ch )
{
	char *out = ACTION(cxt);
	//Section *section;

	if ( !parserInSection )
	{
		if ( *ch == '{' ) /* section begin */
		{
			*out++ = *ch++;
			*out = 0;

			SKIP_SPACE(ch);
			ch = parse_validWord(out, ch); /* name */

			//if ( !*out )
			//	return (!*ch) ? 0 : ch;

			if ( *ch == ':' )
			{
				ch++;
				parserInSection = TRUE;
			}
			else
				return (!*ch) ? 0 : ch;
		}
	}
	else
	{
		if ( *ch == '}' ) /* section end */
		{
			*out++ = *ch++;
			*out = 0;

			parserInSection = FALSE;
		}
	}

	SKIP_SPACE(ch);
	return ch;
}

BOOL parse( Context *cxt, char *str )
{
	char *ch = str;
	char *out;

	*cxt->super = 0;
	*cxt->action = 0;
	*cxt->params = 0;
	cxt->nParams = 0;

	SKIP_SPACE(ch);
	if ( !*ch )
		return FALSE;

	ch = parse_section(cxt, ch);
	if ( *ACTION(cxt) )
		goto _end;

	ch = parse_super(cxt, ch);

	if ( IS_BEGIN(ch) ) /* action */
	{
		out = ACTION(cxt);
		*out++ = *ch++;
		ch = parse_validWord(out, ch);
		SKIP_SPACE(ch);
	}
	else goto _end;

	if ( *ch == '(' && !ACTION_IS_VAR(cxt) ) /* params */
	{
		ch = parse_params(cxt, ch);
		if ( !ch )
			goto _end;
	}

	SKIP_SPACE(ch);

_end:
	if ( !ch )
	{
		printf("Error: unexpected end of line\n");
		return FALSE;
	}
	if ( *ch )
	{
		printf("Error: unexpected char '%c'\n", *ch);
		return FALSE;
	}
	if ( *SUPER(cxt) && !*ACTION(cxt) )
	{
		printf("Error: unexpected end of line\n");
		return FALSE;
	}

	return TRUE;
}

Context * parserInit( void )
{
	Context *cxt = malloc(sizeof(Context));
	memset(cxt->action, 0, sizeof(cxt->action));
	memset(cxt->params, 0, sizeof(cxt->params));
	memset(cxt->super, 0, sizeof(cxt->super));
	cxt->nParams = 0;

	return cxt;
}

void parserFree( Context *cxt )
{
	free(cxt);
}

void parserReset( Context *cxt )
{
	if ( cxt )
	{
		*cxt->super = 0;
		*cxt->action = 0;
		*cxt->params = 0;
		cxt->nParams = 0;
	}

	parserInSection = FALSE;
}

char ** paramsToList( Context *cxt )
{
	char **list = calloc(cxt->nParams, sizeof(char *));
	char *param = PARAM(cxt);
	int i;

	for ( i = 0; i < cxt->nParams; ++i )
	{
		switch ( PARAM_TYPE(param) )
		{
		case PTYPE_ANY:
			{
				char *end;
				int val = strtol(param, &end, 0);
				list[i] = (end == strchr(param, 0)) ? val : param;
			}
			break;

		case PTYPE_VAR:
			{
				Var *var = getVar(param);
				list[i] = (var) ? var->value : NULL;
			}
			break;

		case PTYPE_SPECIAL:
			if ( !_stricmp(param, "$Ret") )
			{
				list[i] = lastRet;
				break;
			}
		case PTYPE_STRING:
			list[i] = param;
			break;

		}

		param = PARAM_NEXT(param);
	}

	return list;
}