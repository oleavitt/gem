/***************************************************************
*
*	gettoken.h - The keyword tokenizer.
*
***************************************************************/

#ifndef __GETTOKEN_H__
#define __GETTOKEN_H__

enum
{
	TK_COMMA,
	TK_LEFTBRACE,
	TK_RIGHTBRACE,
	TK_LEFTPAREN,
	TK_RIGHTPAREN,

	TK_NUMBER,
	TK_QUOTESTRING,
	TK_UNKNOWN,

	TK_BACKGROUND,
	TK_INCLUDE,
	TK_LIGHT,
	TK_SPHERE,
	TK_VIEWPORT,
	NUM_TOKENS
};

#endif /* __GETTOKEN_H__ */