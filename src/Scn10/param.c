/*************************************************************************
*
*  param.c - Parameter parsing functions.
*
*************************************************************************/

#include "local.h"

static int GetComma(const char *forwhom, int *optional, int doit);

/*************************************************************************
*
*  ParseParams()
*    Parses a comma-separated list parameters as specified in a format
*    string,
*
*   const char *format - The format specifier string containing one or
*     more of the format codes listed below.
*
*   const char *forwhom - Name of statement for whom we are parsing.
*
*   int (*ParseDetails)(Stmt **stmt) - Statement-specific details
*     parsing function to be passed to ParseBlock() if a block is parsed.
*
*   Param **paramlist - Pointer to an array of Param structs to receive
*     parameters. If the array is NULL a pointer to our static array
*     is returned.
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
int ParseParams(const char *format, const char *forwhom,
	int (*ParseDetails)(Stmt **stmt), Param *paramlist)
{
	int nparams, optional, getcomma, token;
	Param *plist;
	char c, *cp;
	assert(paramlist != NULL);
	assert(format != NULL);
	nparams = 0;
	optional = 0;
	getcomma = 0;
	cp = (char *)format;
	plist = paramlist;
	for(c = *cp++; c != '\0'; c = *cp++)
	{
		switch(toupper(c))
		{
			case 'O':
				optional = 1;
				break;
			case 'B':
				plist->data.block = ParseBlock(forwhom, ParseDetails);
				if(plist->data.block == NULL)
				{
					if(!optional)
					{
						LogError("%s: Expecting a block of one or more statements.",
							forwhom);
						PrintFileAndLineNumber();
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
				if(GetComma(forwhom, &optional, getcomma))
				{
					plist->data.expr = ExprParse();
					if(plist->data.expr == NULL)
					{
						if(!optional)
						{
							LogError("%s: Numeric expression expected.",
								forwhom);
							PrintFileAndLineNumber();
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
				if(GetComma(forwhom, &optional, getcomma))
				{
					token = GetToken();
					if(token != TK_QUOTESTRING)
					{
						if(!optional)
						{
							LogError("%s: Quoted string literal expected.",
								forwhom);
							PrintFileAndLineNumber();
						}
						else
							UngetToken();
					}
					else
					{
						plist->data.str = (char *)malloc(strlen(token_buffer)+1);
						if(plist->data.str != NULL)
						{
							strcpy(plist->data.str, token_buffer);
							plist->type = PARAM_STRING;
							nparams++;
							plist++;
						}
						else
							LogMemError(forwhom);
						getcomma = 1;
					}
				}
				optional = 0;
				break;
			case 'T':
				if(GetComma(forwhom, &optional, getcomma))
				{
					plist->data.token = GetToken();
					plist->type = PARAM_TOKEN;
					nparams++;
					plist++;
					getcomma = 1;
				}
				optional = 0;
				break;
			case ';':
				if((token = GetToken()) != OP_SEMICOLON)
					ErrUnknown(token, ";", forwhom);
				break;
#ifndef NDEBUG
			default:
				LogMessage("debug: ParseParams(): Unknown param format character.");
				break;
#endif
		}
	} 
	return nparams;
}

int GetComma(const char *forwhom, int *optional, int doit)
{
	if(doit)
	{
		int token;
		if((token = GetToken()) != OP_COMMA)
		{
			if(!optional)
			{
				LogError("%s: Expecting ',' found '%s'.",
					forwhom, token_buffer);
				PrintFileAndLineNumber();
			}
			else
				UngetToken();
			return 0;
		}
		*optional = 0;
	}
	return 1;
}
