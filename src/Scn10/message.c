/*************************************************************************
*
*  message.c - The "message" statement.
*
*************************************************************************/

#include "local.h"

typedef struct tag_messagedata
{
	struct tag_messagedata *next;
	char *str;
	Expr *expr;
} MessageData;


static void ExecMessageStmt(Stmt *stmt);
static void DeleteMessageStmt(Stmt *stmt);
static MessageData *NewMessageData(const char *str);

StmtProcs message_stmt_procs =
{
	TK_MESSAGE,
	ExecMessageStmt,
	DeleteMessageStmt
};

Stmt *ParseMessageStmt(void)
{
	Stmt *stmt;
	Expr *expr;
	MessageData *mdlast, *md;
	int token;

	if((stmt = NewStmt()) != NULL)
	{
		stmt->procs = &message_stmt_procs;
		mdlast = NULL;
		while((token = GetToken()) != OP_SEMICOLON)
		{
			switch(token)
			{
				case TK_QUOTESTRING:
					if((md = NewMessageData(token_buffer)) != NULL)
					{
						if(mdlast != NULL)
							mdlast->next = md;
						else
							stmt->data = (void *)md;
						mdlast = md;
					}
					else
					{
						LogMemError("message");
						DeleteStmt(stmt);
						return NULL;
					}
					break;

				default:
					/* See if there is a valid expression... */
					UngetToken();
					if((expr = ExprParse()) != NULL)
					{
						if((md = NewMessageData(NULL)) != NULL)
						{
							md->expr = expr;
							if(mdlast != NULL)
								mdlast->next = md;
							else
								stmt->data = (void *)md;
							mdlast = md;
						}
						else
						{
							LogMemError("message");
							DeleteStmt(stmt);
							return NULL;
						}
					}
					else
					{
						ErrUnknown(token, ";", "message");
						DeleteStmt(stmt);
						return NULL;
					}
					break;
			}
		}
	}
	return stmt;
}

void ExecMessageStmt(Stmt *stmt)
{
	static char msg[SCN_MSG_MAX];
	static char buf[SCN_MSG_MAX];
	int len;
	MessageData *md = (MessageData *)stmt->data;
	msg[0] = '\0';
	len = 0;
	while(md != NULL)
	{
		if(md->str != NULL)
		{
			strcpy(buf, md->str);
		}
		else if(md->expr != NULL)
		{
			char result[EXPR_RESULT_SIZE_MAX];
			void *presult = &result;
			switch(ExprEval(md->expr, presult))
			{
				case EXPR_VECTOR:
					sprintf(buf, "<%.15g, %.15g, %.15g>",
						((Vec3 *)presult)->x,
						((Vec3 *)presult)->y,
						((Vec3 *)presult)->z);
					break;
				case EXPR_FLOAT:
					sprintf(buf, "%.15g", *((double *)presult));
					break;
				case EXPR_INT:
					sprintf(buf, "%d", *((int *)presult));
					break;
				default:
					buf[0] = '\0';
					break;
			}
		}
		len += strlen(buf);
		if(len >= SCN_MSG_MAX)
		{
			/* split up long lines */
			LogMessage(msg);
			msg[0] = '\0';
			len = 0;
			continue;
		}
		strcat(msg, buf);
		md = md->next;
	}
	LogMessageNoFmt(msg);
}

void DeleteMessageStmt(Stmt *stmt)
{
	MessageData *md = (MessageData *)stmt->data;
	MessageData *mdtmp;
	while(md != NULL)
	{
		mdtmp = md;
		md = md->next;
		if(mdtmp->str != NULL)
			free(mdtmp->str);
		ExprDelete(mdtmp->expr);
		free(mdtmp);	
	}
}

MessageData *NewMessageData(const char *str)
{
	MessageData *md = (MessageData *)calloc(1, sizeof(MessageData));
	if(md != NULL)
	{
		if(str != NULL)
		{
			md->str = (char *)calloc((strlen(str) + 1), sizeof(char));
			if(md->str != NULL)
				strcpy(md->str, str);
			else
			{
				free(md);
				return NULL;
			}
		}
		else
			md->str = NULL;
		md->expr = NULL;
	}
	return md;
}