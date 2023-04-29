#pragma once


typedef struct Context
{
	char action[128];
	char params[256];
	char super[64];
	int nParams;
}
Context;


#define ACTION(pcxt) ((pcxt)->action)
#define ACTION_IS(pcxt, str) (!_stricmp(ACTION(pcxt), (str)))
#define ACTION_IS_SPECIAL(pcxt) (*ACTION(pcxt) == '$')
#define ACTION_IS_VAR(pcxt) (*ACTION(pcxt) == '?')
#define ACTION_IS_SECTION(pcxt) (*ACTION(pcxt) == '{')
#define ACTION_IS_SECTION_END(pcxt) (*ACTION(pcxt) == '}')
#define ACTION_NOPRE(pcxt) (ACTION(pcxt) + 1)
#define ACTION_IS_CALL(pcxt) (*ACTION(pcxt) == ':')

#define PARAM(pcxt) ((pcxt)->params + 1)
#define PARAM_NEXT(p) (strchr((p), 0) + 2)
#define PARAM_TYPE(p) (*((p) - 1))

#define SUPER(pcxt) ((pcxt)->super)
#define SUPER_IS_VAR(pcxt) (*SUPER(pcxt) == '?')


Context * parserInit( void );
void parserFree( Context *cxt );
BOOL parse( Context *cxt, char *str );
char ** paramsToList( Context *cxt );
