%{
/* $Id: grammar.y,v 1.16 2010/04/07 12:33:09 bediger Exp $ */

#ifdef YYBISON
#define YYERROR_VERBOSE
#endif

#include <stdio.h>
#include <unistd.h>  /* getopt() */

#include <node.h>
#include <stmnt.h>
#include <hashtable.h>
#include <atom.h>
#include <abbreviations.h>

int yyerror(const char *s1);
int yywrap(void);

void print_variable(const char *var_name);

static int prompting = 1;
void print_prompt(void);

int defining_function = 0;

struct context *ctxt_stack = NULL;

%}

%union{
    const char *identifier;
    const char *literal_string;
    struct node *code;
	int numerical_constant;
	enum opNodeType op;
	enum booleanType boolean;
}

%token <identifier> TK_VARIABLE
%token <literal_string> STRING_LITERAL
%token TK_EQUAL TK_PLUS TK_MINUS TK_MULT TK_DIV TK_CONCAT TK_OUT
%token TK_EQUALITY TK_LESSER TK_GREATER
%token TK_LPAREN TK_RPAREN TK_LBRACE TK_RBRACE TK_COMMA
%token TK_PRINT TK_FUNC TK_RETURN TK_IF TK_ELSE TK_WHILE
%token TK_EOL TK_RUN
%token TK_TRUE TK_FALSE
%token TK_NOT TK_AND TK_OR
%token <numerical_constant> NUMERICAL_CONSTANT

%type <code> program statement_block
%type <code> single_item expression statement boolean_value
%type <code> assignment_expression logical_or_expression
%type <code> unary_expression logical_and_expression relational_expression
%type <code> equality_expression additive_expression multiplicate_expression
%type <code> function_definition arg_list function_call function_proto
%type <code> selection_statement if_proto if_true while_proto while_true
%type <op> unary_operator

%%

thinger
	: program
	| cmd_statement
	| thinger cmd_statement {
			print_prompt();
		}
	| thinger program {
			print_prompt();
		}
	;

cmd_statement
	: TK_PRINT STRING_LITERAL TK_EOL {
			printf("%s", $2);
		}
	| TK_PRINT TK_VARIABLE TK_EOL {
			print_variable($2);
		}
	| TK_RUN TK_EOL {
			struct stmnt *prgm = pop_context(&ctxt_stack);
			if (prgm)
			{
				struct node *v = execute_prog(prgm);
				free_node(v);
				free_stmnts(prgm);
			}
		}
	;

program
	: statement_block {
			struct stmnt *st = pop_context(&ctxt_stack);
			if (st)
			{
				struct node *v = execute_prog(st);
				free_node(v);
				free_stmnts(st);
			}
		}
	| function_definition
	| program function_definition
	| program statement_block {
			struct stmnt *st = pop_context(&ctxt_stack);
			if (st)
			{
				struct node *v = execute_prog(st);
				free_node(v);
				free_stmnts(st);
			}
		}

	;

function_definition
	: function_proto TK_LBRACE {push_context(&ctxt_stack);} statement_block TK_RBRACE {
			struct node *fn = $$;
			fn->n.function.code = pop_context(&ctxt_stack);
		}
	;

statement_block
	: statement {
			if (ctxt_stack)
				add_stmnt(ctxt_stack, new_stmnt($1));
			else {
				struct node *val, *n = $1;
				val = execute_code(n);
				free_node(n);
				if (val) free_node(val);
			}
			print_prompt();
		}
	| statement_block statement {
			if (ctxt_stack)
				add_stmnt(ctxt_stack, new_stmnt($2));
			else {
				struct node *val, *n = $2;
				val = execute_code(n);
				free_node(n);
				if (val) free_node(val);
			}
			print_prompt();
		}
	| error {
			struct stmnt *st = pop_context(&ctxt_stack);
			free_stmnts(st);
			print_prompt();
		}
	;


statement
	: assignment_expression TK_EOL { $$ = $1; }
	| selection_statement { $$ = $1; }
	| TK_RETURN expression TK_EOL {
			$$ = new_operator(OP_RETURN, $2, NULL);
		}
	| TK_OUT arg_list TK_EOL {
			$$ = new_operator(OP_OUTPUT, $2, NULL);
		}
	;

assignment_expression
	: unary_expression TK_EQUAL expression {
			$$ = new_operator(OP_ASSIGN, $1, $3);
		}
	;

expression
	: logical_or_expression { $$ = $1; }
	| function_call { $$ = $1; }
	;


function_call
	: TK_VARIABLE TK_LPAREN arg_list TK_RPAREN {
			if (!defining_function)
			{
				/* should find a reference in the symbol table */
				struct node *fn = abbreviation_lookup($1);
				struct node *call = new_node();
				if (!fn)
				{
					/* This function call constitutes a "forward ref" */
					fn = new_node();
					fn->typ = NT_FUNCTION;
					fn->n.function.func_name = $1;
					abbreviation_add(fn->n.function.func_name, fn);
				}
				call->typ = NT_CALL;
				call->n.call.function = fn;
				call->n.call.func_name = call->n.call.function->n.function.func_name;
				call->n.call.args = $3;
				call->n.call.function->n.function.args = NULL;
				$$ = call;
				/* In this case, a function_call non-terminal will get
				 * interpreted - it doesn't denote a function definition. */
			} else {
				int add_abbrev = 0;
				struct node *nf = abbreviation_lookup($1);
				if (!nf)
				{
					/* Two mutually recursive functions would end up
					 * leaving a value for nf, but this should constitute
					 * the usual case. */
					add_abbrev = 1;
					nf = new_node();
					nf->typ = NT_FUNCTION;
					nf->n.function.func_name = $1;
				}
				nf->n.function.formal_args = $3;
				if (add_abbrev)
					abbreviation_add(nf->n.function.func_name, nf);
				/* set $$ to the new struct node, so that function_call non-terminal
				 * can pass something up - it has to in the case of a function call site.
				 * In the case of a function definition, this doesn't really get used
				 * for anything. */
				$$ = nf;
				defining_function = 0;
			}
		}
	;

selection_statement
	: if_true { $$ = $1; }
	| while_true { $$ = $1; }
	| if_true TK_ELSE TK_LBRACE {push_context(&ctxt_stack);}statement_block TK_RBRACE {
			struct node *n = $1;
			n->n.control.false_block = pop_context(&ctxt_stack);
			$$ = n;
		}
	;

if_true
	: if_proto TK_LBRACE {push_context(&ctxt_stack);} statement_block TK_RBRACE {
			struct node *n = $1;
			n->n.control.true_block = pop_context(&ctxt_stack);
			n->n.control.false_block = NULL;
			$$ = n;
		}
;

while_true
	: while_proto TK_LBRACE {push_context(&ctxt_stack);} statement_block TK_RBRACE {
			struct node *n = $1;
			n->n.control.true_block = pop_context(&ctxt_stack);
			n->n.control.false_block = NULL;
			$$ = n;
		}
;

if_proto
	: TK_IF TK_LPAREN logical_or_expression TK_RPAREN {
				struct node *n = new_node();
				n->typ = NT_CONTROL;
				n->n.control.typ = CT_IF;
				n->n.control.condition = $3;
				$$ = n;
			}
	;

while_proto
	: TK_WHILE TK_LPAREN logical_or_expression TK_RPAREN {
				struct node *n = new_node();
				n->typ = NT_CONTROL;
				n->n.control.typ = CT_WHILE;
				n->n.control.condition = $3;
				$$ = n;
			}
	;

function_proto
	: TK_FUNC function_call { $$ = $2; }
	;

arg_list
	: { $$ = NULL; }
	| single_item { $$ = $1; }
	| arg_list TK_COMMA single_item {
			struct node *n = $1;
			for (n = $1; n; n = n->mgmt)
			{
				if (!n->mgmt)
				{
					n->mgmt = $3;
					break;
				}
			}
			$$ = $1;
		}
	;

logical_or_expression
	: logical_and_expression  { $$ = $1; }
	| logical_and_expression TK_OR logical_or_expression {
			$$ = new_operator(OP_OR, $1, $3);
		}
	;

logical_and_expression
	: equality_expression { $$ = $1; }
	| logical_and_expression TK_AND equality_expression {
			$$ = new_operator(OP_AND, $1, $3);
		}
	;

equality_expression
	: relational_expression { $$ = $1; }
	| equality_expression TK_EQUALITY relational_expression  {
			$$ = new_operator(OP_EQUALITY, $1, $3);
		}
	;

relational_expression
	: additive_expression { $$ = $1; }
	| relational_expression TK_GREATER  additive_expression {
			$$ = new_operator(OP_GREATER, $1, $3);
		}
	| relational_expression TK_LESSER   additive_expression {
			$$ = new_operator(OP_LESSER, $1, $3);
		}
	;

additive_expression
	: multiplicate_expression { $$ = $1; }
	| additive_expression TK_PLUS multiplicate_expression {
			$$ = new_operator(OP_ADD, $1, $3);
		}
	| additive_expression TK_MINUS multiplicate_expression {
			$$ = new_operator(OP_SUB, $1, $3);
		}
	;

multiplicate_expression
	: unary_expression { $$ = $1; }
	| multiplicate_expression TK_MULT unary_expression {
			$$ = new_operator(OP_MULT, $1, $3);
		}
	| multiplicate_expression TK_DIV  unary_expression {
			$$ = new_operator(OP_DIV, $1, $3);
		}
	| multiplicate_expression TK_CONCAT  unary_expression {
			$$ = new_operator(OP_CONCAT, $1, $3);
		}
	;

unary_expression
	: unary_operator single_item {
			$$ = new_operator($1, $2, NULL);
		}
	| single_item { $$ = $1; }
	;

unary_operator
	: TK_NOT     { $$ = OP_NOT; }
	;

single_item
	: NUMERICAL_CONSTANT {
			struct node *n = new_node();
			n->typ = NT_VALUE;
			n->n.value.typ = VT_NUMBER;
			n->n.value.v.number = $1;
			$$ = n;
		}
	| STRING_LITERAL {
			struct node *n = new_node();
			n->typ = NT_VALUE;
			n->n.value.typ = VT_STRING;
			n->n.value.v.string = $1;
			$$ = n;
		}
	| TK_VARIABLE {
			struct node *n = new_identifier($1);
			$$ = n;
		}
	| boolean_value { $$ = $1; }
	| function_call { $$ = $1; }
	| TK_LPAREN expression TK_RPAREN { $$ = $2; }
	;

boolean_value
	: TK_TRUE {
			struct node *n = new_node();
			n->typ = NT_VALUE;
			n->n.value.typ = VT_BOOLEAN;
			n->n.value.v.boolean = BOOL_TRUE;
			$$ = n;
		}
	| TK_FALSE {
			struct node *n = new_node();
			n->typ = NT_VALUE;
			n->n.value.typ = VT_BOOLEAN;
			n->n.value.v.boolean = BOOL_FALSE;
			$$ = n;
		}
	;

%%

int
main(int ac, char **av)
{
	int r;
	extern int yydebug;
	int c;

	struct hashtable *h = init_hashtable(64, 10);

	setup_atom_table(h);
	setup_abbreviation_table(h);

	while (-1 != (c = getopt(ac, av, "dp")))
	{
		switch (c)
		{
		case 'd':
			yydebug = 1;
			break;
		case 'p':
			prompting = 0;
			break;
		}
	}

	print_prompt();
	r = yyparse();

	while (ctxt_stack) 
	{
		struct stmnt *st = pop_context(&ctxt_stack);
		free_stmnts(st);
	}

	/* free_hashtable() and free_all_stmnt() can put structs node
	 * on the node free list. */
	free_hashtable(h);
	free_all_contexts();
	free_all_stmnt();

	free_all_nodes();


	return r;
}

void
print_prompt(void)
{
	if (prompting)
		printf("> ");
}

void
print_variable(const char *var)
{
	struct node *n = abbreviation_lookup(var);
	if (n)
	{
		print_node(n);
		printf("\n");
	} else
		printf("unset variable %s\n", var);
}

int
yyerror(const char *s1)
{
	fprintf(stderr, "%s\n", s1);
	return 0;
}

int
yywrap()
{
    int r = 1;
	return r;
}
