/**
 *****************************************************************************
 *  @file param.c
 *  Parameter parsing functions.
 *
 *****************************************************************************
 */

#include "local.h"

static int parse_comma(const char *forwhom, int *optional, int doit);

/*************************************************************************
*
*  parse_paramlist()
*    Parses a comma-separated list of parameters as specified in a format
*    string.
*
*   const char *format - The format specifier string containing one or
*     more of the format codes listed below.
*
*   const char *forwhom - Name of statement for whom we are parsing.
*
*   Param **paramlist - Pointer to an array of Param structs to receive
*     parameters.
*
*   Format codes:
*     B - Block of statements.
*     E - Numeric expression.
*     S - Quoted string literal.
*     T - Named token value - integer token value is returned.
*     O - Next param is optional - do not report an error if not found.
*     ; - Expect a semicolon.
*
*************************************************************************/
int parse_paramlist(const char *format, const char *forwhom,
	ParamList *paramlist)
{
	int			nparams, optional, getcomma, token;
	ParamList	*plist;
	char		c;
	const char	*cp;

	assert(paramlist != NULL);
	assert(format != NULL);
	nparams = 0;
	optional = 0;
	getcomma = 0;
	cp = format;
	plist = paramlist;

	for (c = *cp++; c != '\0'; c = *cp++)
	{
		switch (toupper(c))
		{
			case 'O':
				optional = 1;
				break;

			case 'B':
				plist->data.block = parse_vm_block();
				if (plist->data.block == NULL)
				{
					if (!optional)
					{
						logerror("%s: Expecting a block of one or more statements.",
							forwhom);
						file_PrintFileAndLineNumber();
					}
				}
				else
				{
					plist->type = PARAM_BLOCK;
					nparams++;
					plist++;
					getcomma = 1;
				}
				optional = 0;
				break;

			case 'E':
				if (parse_comma(forwhom, &optional, getcomma))
				{
					plist->data.expr = parse_exprtree();
					if (plist->data.expr == NULL)
					{
						if (!optional)
						{
							logerror("%s: Numeric expression expected.",
								forwhom);
							file_PrintFileAndLineNumber();
						}
					}
					else
					{
						plist->type = PARAM_EXPR;
						nparams++;
						plist++;
						getcomma = 1;
					}
				}
				optional = 0;
				break;

			case 'S':
				if (parse_comma(forwhom, &optional, getcomma))
				{
					token = gettoken();
					if (token != TK_QUOTESTRING)
					{
						if (!optional)
						{
							logerror("%s: Quoted string literal expected.",
								forwhom);
							file_PrintFileAndLineNumber();
						}
						else
							gettoken_Unget();
					}
					else
					{
						plist->data.str = (char *)malloc(strlen(g_token_buffer)+1);
						if (plist->data.str != NULL)
						{
							strcpy(plist->data.str, g_token_buffer);
							plist->type = PARAM_STRING;
							nparams++;
							plist++;
						}
						else
							logmemerror(forwhom);
						getcomma = 1;
					}
				}
				optional = 0;
				break;

			case 'T':
				if (parse_comma(forwhom, &optional, getcomma))
				{
					plist->data.token = gettoken();
					plist->type = PARAM_TOKEN;
					nparams++;
					plist++;
					getcomma = 1;
				}
				optional = 0;
				break;

			case ';':
				if ((token = gettoken()) != OP_SEMICOLON)
					gettoken_ErrUnknown(token, ";");
				break;

#ifndef NDEBUG
			default:
				logmsg("debug: ParseParams(): Unknown param format character.");
				break;
#endif
		}
	}
	 
	return nparams;
}

int parse_comma(const char *forwhom, int *optional, int doit)
{
	if (doit)
	{
		int token;

		if ((token = gettoken()) != OP_COMMA)
		{
			if (!optional)
			{
				logerror("%s: Expecting ',' found '%s'.",
					forwhom, g_token_buffer);
				file_PrintFileAndLineNumber();
			}
			else
				gettoken_Unget();
	
			return 0;
		}
		*optional = 0;
	}

	return 1;
}
