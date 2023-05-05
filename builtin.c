#include "external.h"
#include "builtin.h"
#include "parse.h"
#include "var.h"


/* from lsh.c */
void printLastRet( void );
void setVarInSuper( Context *cxt, int value );


typedef enum ConditionId
{
	COND_NONE,
	COND_EQ,
	COND_NEQ,
	COND_LT,
	COND_GT,
	COND_LTE,
	COND_GTE
}
ConditionId;


BOOL builtin_cond( Context *cxt )
{
	ConditionId cond = COND_NONE;
	char **params;
	int left, right;
	BOOL result = 0;

	if      ( ACTION_NP_IS(cxt, "==") ) cond = COND_EQ;
	else if ( ACTION_NP_IS(cxt, "!=") ) cond = COND_NEQ;
	else if ( ACTION_NP_IS(cxt, "<") )  cond = COND_LT;
	else if ( ACTION_NP_IS(cxt, ">") )  cond = COND_GT;
	else if ( ACTION_NP_IS(cxt, "<=") ) cond = COND_LTE;
	else if ( ACTION_NP_IS(cxt, ">=") ) cond = COND_GTE;
	else return FALSE;

	if ( cxt->nParams != 2 )
	{
		printf("Error: expected 2 parameters, got %d\n", cxt->nParams);
		return TRUE;
	}

	params = paramsToList(cxt);
	left = params[0];
	right = params[1];

	switch ( cond )
	{
	case COND_EQ:  result = (left == right); break;
	case COND_NEQ: result = (left != right); break;
	case COND_LT:  result = (left < right);  break;
	case COND_GT:  result = (left > right);  break;
	case COND_LTE: result = (left <= right); break;
	case COND_GTE: result = (left >= right); break;
	}

	lastRet = result;
	setVarInSuper(cxt, result);
	printLastRet();

	free(params);
	return TRUE;
}

BOOL builtin_oper( Context *cxt )
{
	char *param;
	int result = 0;

	if ( ACTION_NP_IS(cxt, "Val") )
	{
		if ( cxt->nParams != 1 )
		{
			printf("Error: expected 1 parameter, got %d\n", cxt->nParams);
			return TRUE;
		}
		
		param = PARAM(cxt);

		switch ( PARAM_TYPE(param) )
		{
		case PTYPE_ANY:
		case PTYPE_STRING:
			{
				char *end;
				int val = strtol(param, &end, 0);
				result = (end == strchr(param, 0)) ? val : 0;
			}
			break;

		case PTYPE_VAR:
			{
				Var *var = getVar(param);
				result = (var) ? var->value : 0;
			}
			break;

		case PTYPE_SPECIAL:
			result = (!_stricmp(param, "$Ret")) ? lastRet : 0;
			break;
		}

		lastRet = result;
		setVarInSuper(cxt, result);
		printLastRet();
		return TRUE;
	}

	return FALSE;
}