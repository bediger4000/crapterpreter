/*
Solaris 2.8, probably original AT&T Bell Labs lex, needed this:
%a 10000
%o 10000
*/
%{

/* $Id: lex.l,v 1.6 2010/03/26 21:01:40 bediger Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>    /* errno */
#include <string.h>   /* strlen(), strerror() */

#include <node.h>
#include <hashtable.h>
#include <atom.h>

#include "y.tab.h"
int lineno = 0;
char *unescape_string(char *s);

extern int defining_function;

%}

%%

\#.*$		{  }
\n		    {  }
;           { return TK_EOL; }
\,          { return TK_COMMA; }
=           { return TK_EQUAL; }
==          { return TK_EQUALITY; }
\>          { return TK_GREATER; }
\<          { return TK_LESSER; }
\+          { return TK_PLUS; }
\-          { return TK_MINUS; }
\*          { return TK_MULT; }
\/          { return TK_DIV; }
\\\n	    { /* Just eat it. */ }
\(		    { return TK_LPAREN; }
\)		    { return TK_RPAREN; }
\{		    { return TK_LBRACE; }
\}		    { return TK_RBRACE; }
\.          { return TK_CONCAT; }
"||"        { return TK_OR; }
"&&"        { return TK_AND; }
"!"         { return TK_NOT; }
"->"        { return TK_OUT; }
"TRUE"      { return TK_TRUE; }
"FALSE"     { return TK_FALSE; }
"print"     { return TK_PRINT; }
"fun"       { defining_function = 1; return TK_FUNC; }
"return"    { return TK_RETURN; }
"run"       { return TK_RUN; }
"if"        { return TK_IF; }
"while"     { return TK_WHILE; }
"else"      { return TK_ELSE; }

[0-9][0-9]* { yylval.numerical_constant = strtol(yytext, NULL, 10);
				return NUMERICAL_CONSTANT; }
\"(\\.|[^\\"])*\" {
	/* quoted string, for file names with spaces, escaped characters, etc */
	char *tmp;
	tmp = unescape_string(yytext);
	yylval.literal_string = Atom_string(tmp);
	free(tmp);
	return STRING_LITERAL;
}

[a-zA-Z_][a-zA-Z_0-9]* {
	yylval.identifier = Atom_string(yytext);
	return TK_VARIABLE;
}

\*		    { /* Ignore asterisks in input: output uses them to mark reducible atoms. */ }
.		    { /* Just eat it. */ }

%%

char *
unescape_string(char *s)
{
	char *n = NULL;
	if (s)
	{
		int i, j, l;
		if ('"' == s[0])
			++s;
		if ('"' == s[strlen(s) - 1])
			s[strlen(s) - 1] = '\0';
		n = malloc(strlen(s) + 1);
		l = strlen(s);
		for (i = 0, j = 0; i < l; ++i)
		{
			if ('\\' == s[i])
			{
				++i;
				switch (s[i])
				{
				/* XXX - doesn't do '\0' or other numerical escapes */
				case 't': n[j++] = '\t';  break;
				case 'r': n[j++] = '\r';  break;
				case 'n': n[j++] = '\n';  break;
				case '\\': n[j++] = '\\'; break;
				default:
					n[j++] = s[i];
					break;
				}
			} else {
				n[j++] = s[i];
			}
		}
		n[j] = '\0';
	}
	return n;
}
