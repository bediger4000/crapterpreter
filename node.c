/* $Id: node.c,v 1.14 2010/04/07 12:33:09 bediger Exp $ */
#include <stdio.h>
#include <stdlib.h>  /* malloc, calloc, free */
#include <string.h>  /* strlen() */

#include <node.h>
#include <hashtable.h>
#include <atom.h>
#include <abbreviations.h>
#include <stmnt.h>

static struct node *free_node_list = NULL;
static int allocated_node_cnt = 0;

struct node *execute_operator(struct node *operator_node);
struct node *execute_value(struct node *value_node);
struct node *execute_function_call(struct node *function_call_node);
struct node *execute_control(struct node *control_node);
struct node *execute_logical_op(struct node *logical_op);
struct node *execute_comparison_op(struct node *comparison_op);

void print_value(struct node *value_node);
void print_operator(struct node *operator_node);
void print_function(struct node *function_node);
void print_call(struct node *function_call_node);
void print_control(struct node *control_node);

void print_identifier_list(struct node *n);

struct node *
new_node(void)
{
	struct node *r = NULL;
	if (free_node_list)
	{
		r = free_node_list;
		free_node_list = free_node_list->mgmt;
	} else {
		++allocated_node_cnt;
		r = malloc(sizeof(*r));
	}
	memset(r, 0, sizeof(*r));
	r->mgmt = NULL;
	r->typ = NT_READY;
	r->return_now = 0;
	return r;
}

struct node *
new_identifier(const char *id)
{
	struct node *n = new_node();
	n->typ = NT_VALUE;
	n->n.value.typ = VT_IDENTIFIER;
	n->n.value.v.identifier = id;
	return n;
}


void
free_node(struct node *tree)
{
	if (tree)
	{
		switch (tree->typ)
		{
		case NT_OPERATOR:
			if (OP_OUTPUT == tree->n.operator.op)
			{
				struct node *n = tree->n.operator.left;
				while (n)
				{
					struct node *tmp = n->mgmt;
					free_node(n);
					n = tmp;
				}
			} else {
				free_node(tree->n.operator.left);
				free_node(tree->n.operator.right);
			}
			break;
		case NT_CALL:
			free_args_list(tree->n.call.args);
			break;
		case NT_FUNCTION:
			free_stmnts(tree->n.function.code);
			free_args_list(tree->n.function.formal_args);
			break;
		case NT_CONTROL:
			free_node(tree->n.control.condition);
			free_stmnts(tree->n.control.true_block);
			free_stmnts(tree->n.control.false_block);
			break;
		case NT_FREE:
		case NT_READY:
		case NT_VALUE:
			break;
		}

		memset(tree, 0, sizeof(*tree));
		tree->typ = NT_FREE;
		tree->mgmt = free_node_list;
		free_node_list = tree;
	}
}

void
free_all_nodes(void)
{
	int freed_node_cnt = 0;

	while (free_node_list)
	{
		struct node *tmp = free_node_list->mgmt;
		free(free_node_list);
		free_node_list = tmp;
		++freed_node_cnt;
	}

	if (freed_node_cnt != allocated_node_cnt)
	{
		fprintf(stderr, "Allocated %d nodes, but only freed %d nodes\n",
			allocated_node_cnt, freed_node_cnt
		);
	}
}

/* Dispatch to a function that knows about a particular
 * node type (enum nodeType).
 */
struct node *
execute_code(struct node *code)
{
	struct node *(*f)(struct node *) = NULL;
	struct node *r = NULL;

	switch (code->typ)
	{
	case NT_FREE: case NT_READY:
		printf("execute NT_FREE or NT_READY node\n");
		break;
	case NT_FUNCTION:
		printf("Inappropriate execution of NT_FUNCTION type node\n");
		break;
	case NT_VALUE:
		f = execute_value;
		break;
	case NT_CALL:
		f = execute_function_call;
		break;
	case NT_OPERATOR:
		f = execute_operator;
		break;
	case NT_CONTROL:
		f = execute_control;
		break;
	}

	r = f(code);

	return r;
}

struct node *
execute_operator(struct node *op)
{
	struct node *n = NULL, *l = NULL, *r = NULL;
	struct node *retval = NULL;

	switch (op->n.operator.op)
	{
	case OP_OUTPUT:
		for (n = op->n.operator.left; n; n = n->mgmt)
		{
			l = execute_code(n);
			print_node(l);
			free_node(l);
		}
		printf("\n");
		break;
	case OP_ADD:
	case OP_SUB:
	case OP_MULT:
	case OP_DIV:
		l = execute_code(op->n.operator.left);
		r = execute_code(op->n.operator.right);
		n = new_node();
		n->typ = NT_VALUE;
		n->n.value.typ = VT_NUMBER;
		n->n.value.v.number = 
			op->n.operator.op == OP_ADD?
				l->n.value.v.number + r->n.value.v.number:
			op->n.operator.op == OP_SUB?
				l->n.value.v.number - r->n.value.v.number:
			op->n.operator.op == OP_MULT?
				l->n.value.v.number * r->n.value.v.number:
				l->n.value.v.number / r->n.value.v.number
		;
		retval = n;
		n = NULL;
		free_node(l);
		free_node(r);
		break;
	case OP_CONCAT: {
		char *buf;
		size_t l_str_len, new_str_len;
		l = execute_code(op->n.operator.left);
		r = execute_code(op->n.operator.right);
		l_str_len = strlen(l->n.value.v.string);
		new_str_len = strlen(r->n.value.v.string) + 1;
		new_str_len += l_str_len;

		buf = malloc(new_str_len);
		n = new_node();
		n->typ = NT_VALUE;
		n->n.value.typ = VT_STRING;

		strcpy(buf, l->n.value.v.string);
		strcpy(&buf[l_str_len], r->n.value.v.string);

		n->n.value.v.string = Atom_string(buf);
		free(buf);
		retval = n;
		n = NULL;
		free_node(l);
		free_node(r);
		}
	break;
	case OP_ASSIGN:  /* Assignment, not equality. */
		r = execute_code(op->n.operator.right);
		abbreviation_add(op->n.operator.left->n.value.v.identifier, r);
		break;
	case OP_RETURN:
		retval = execute_code(op->n.operator.left);
		retval->return_now = 1;
		break;
	case OP_NULL:
		break;
	case OP_AND: case OP_OR: case OP_NOT:
		retval = execute_logical_op(op);
		break;
	case OP_GREATER: case OP_LESSER: case OP_EQUALITY:
		retval = execute_comparison_op(op);
		break;
	}
	return retval;
}

/* This code could appear in execute_operator(), but it seemed neater
 * to break it out into its own function */
struct node *
execute_logical_op(struct node *logical_op)
{
	struct node *retval = NULL;
	struct node *l, *r = NULL;
	int lv, rv = 0;

	l = execute_code(logical_op->n.operator.left);
	lv = l->n.value.v.boolean;

	/* OP_NOT only has left value: work around this. */
	if (logical_op->n.operator.right)
		r = execute_code(logical_op->n.operator.right);
	if (r)
		rv = r->n.value.v.boolean;

	retval = new_node();
	retval->typ = NT_VALUE;
	retval->n.value.typ = VT_BOOLEAN;

	switch (logical_op->n.operator.op)
	{
	case OP_AND:
		retval->n.value.v.boolean = (lv && rv);
		break;
	case OP_OR:
		retval->n.value.v.boolean = (lv || rv);
		break;
	case OP_NOT:
		retval->n.value.v.boolean = !lv;
		break;
	default:
		break;
	}
	free_node(l);
	free_node(r);
	return retval;
}

/* This code could appear in execute_operator(), but it seemed neater
 * to break it out into its own function */
struct node *
execute_comparison_op(struct node *comparison_op)
{
	struct node *retval = NULL;
	struct node *l, *r = NULL;
	int lv, rv = 0;

	l = execute_code(comparison_op->n.operator.left);
	lv = l->n.value.v.number;

	r = execute_code(comparison_op->n.operator.right);
	rv = r->n.value.v.number;

	retval = new_node();
	retval->typ = NT_VALUE;
	retval->n.value.typ = VT_BOOLEAN;

	switch (comparison_op->n.operator.op)
	{
	case OP_GREATER:
		retval->n.value.v.boolean = (lv > rv);
		break;
	case OP_LESSER:
		retval->n.value.v.boolean = (lv < rv);
		break;
	case OP_EQUALITY:
		retval->n.value.v.boolean = (lv == rv);
		break;
	default:
		/* XXX - undefined! */
		break;
	}
	free_node(l);
	free_node(r);
	return retval;
}

struct node *
execute_value(struct node *value_node)
{
	struct node *n, *r = NULL;
	
	switch (value_node->n.value.typ)
	{
	case VT_NUMBER:
	case VT_STRING:
	case VT_BOOLEAN:
		r = new_node();
		r->n = value_node->n;
		r->typ = value_node->typ;
		break;
	case VT_IDENTIFIER:
		n = abbreviation_lookup(value_node->n.value.v.identifier);
		r = n? execute_code(n): new_identifier(value_node->n.value.v.identifier);
		break;
	}
	return r;
}

struct node *
execute_function_call(struct node *function_call_node)
{
	struct node *formal_args, *args;
	struct node *val = NULL;
	struct stmnt *curr;

	if (!function_call_node->n.function.code)
	{
		struct node *fn = abbreviation_lookup(function_call_node->n.function.func_name);
		function_call_node->n.call.function = fn;
	}

	/* Function prolog - setting formal arg values, etc */
	push_call_stack();

	formal_args = function_call_node->n.call.function->n.function.formal_args;
	args = function_call_node->n.call.args;

	while (formal_args)
	{
		const char *identifier = formal_args->n.value.v.identifier;
		struct node *local_value = NULL;

		if (args)
			local_value = execute_code(args);

		/* call-by-value? */
		identifier = add_local_string(identifier);
		abbreviation_add(identifier, local_value);

		formal_args = formal_args->mgmt;
		if (args)
			args = args->mgmt;
	}

	for (curr = function_call_node->n.call.function->n.function.code;
		curr; curr = curr->next)
	{
		if (val) free_node(val);
		val = execute_code(curr->code);
		/* last val "falls through" to return */
		if (val && val->return_now)
			break;
	}

	/* Function epilog goes here */
	pop_call_stack();

	return val;
}

struct node *
execute_control(struct node *control_node)
{
	struct node *r = NULL, *v = NULL;
	switch (control_node->n.control.typ)
	{
	case CT_IF:
		v = execute_code(control_node->n.control.condition);
		if (v->n.value.v.boolean == BOOL_TRUE)
			r = execute_prog(control_node->n.control.true_block);
		else {
			if (control_node->n.control.false_block)
				r = execute_prog(control_node->n.control.false_block);
		}
		free_node(v);
		break;
	case CT_WHILE:
		v = execute_code(control_node->n.control.condition);
		while (v->n.value.v.boolean == BOOL_TRUE)
		{
			free_node(v);
			if (r) free_node(r);
			r = execute_prog(control_node->n.control.true_block);
			if (r && r->return_now)
				break;
			v = execute_code(control_node->n.control.condition);
		}
		free_node(v);
		break;
	}
	return r;
}

void
print_node(struct node *node)
{
	switch (node->typ)
	{
	case NT_FREE:
	case NT_READY:
		break;
	case NT_CONTROL:
		print_control(node);
		break;
	case NT_OPERATOR:
		print_operator(node);
		break;
	case NT_VALUE:
		print_value(node);
		break;
	case NT_FUNCTION:
		print_function(node);
		break;
	case NT_CALL:
		print_call(node);
		break;
	}
}

void
print_value(struct node *value_node)
{
	switch (value_node->n.value.typ)
	{
	case VT_NUMBER:
		printf("%d", value_node->n.value.v.number);
		break;
	case VT_STRING:
		printf("%s", value_node->n.value.v.string);
		break;
	case VT_IDENTIFIER:
		printf("%s", value_node->n.value.v.identifier);
		break;
	case VT_BOOLEAN:
		printf("%s", value_node->n.value.v.boolean == BOOL_TRUE? "TRUE": "FALSE");
		break;
	}
}

void
print_operator(struct node *operator_node)
{
	struct node *n;
	switch (operator_node->n.operator.op)
	{
	case OP_NULL:
		break;
	case OP_OUTPUT:
		printf("-> ");
		print_node(n = operator_node->n.operator.left);
		for (n = operator_node->n.operator.left->mgmt; n; n = n->mgmt)
		{
			printf(", ");
			print_node(n);
		}
		printf(";\n");
		break;
	case OP_ASSIGN:
	case OP_ADD:
	case OP_SUB:
	case OP_CONCAT:
	case OP_MULT:
	case OP_DIV:
		print_node(operator_node->n.operator.left);
		printf(" %c ",
			operator_node->n.operator.op == OP_ADD? '+':
				operator_node->n.operator.op == OP_SUB? '-':
				operator_node->n.operator.op == OP_ASSIGN? '=':
				operator_node->n.operator.op == OP_CONCAT? '.':
				operator_node->n.operator.op == OP_MULT? '*': '/'
		);
		print_node(operator_node->n.operator.right);
		if (OP_ASSIGN == operator_node->n.operator.op)
			printf(";\n");
		break;
	case OP_GREATER: case OP_LESSER: case OP_EQUALITY:
		print_node(operator_node->n.operator.left);
		printf(" %s ",
			operator_node->n.operator.op == OP_GREATER? ">":
			operator_node->n.operator.op == OP_LESSER?  "<": "=="
		);
		print_node(operator_node->n.operator.right);
		break;
	case OP_AND: case OP_OR:
		print_node(operator_node->n.operator.left);
		printf(" %s ",
			operator_node->n.operator.op == OP_AND? "&&": "||"
		);
		print_node(operator_node->n.operator.right);
		break;
	case OP_NOT:
		printf("!");
		print_node(operator_node->n.operator.left);
		break;
	case OP_RETURN:
		printf("return ");
		print_node(operator_node->n.operator.left);
		printf(";\n");
		break;
	}
}

/* walk the "->mgmt" list of structs node, and print each
 * struct's value as an identifier. */
void
print_identifier_list(struct node *n)
{
	struct node *a;
	printf("%s", n->n.value.v.identifier);
	for (a = n->mgmt; a; a = a->mgmt)
		printf(", %s", a->n.value.v.identifier);
}

void
print_function(struct node *function_node)
{
	struct stmnt *st;

	printf("fun %s (", function_node->n.function.func_name);
	if (function_node->n.function.formal_args)
		print_identifier_list(function_node->n.function.formal_args);
	printf(") {\n");

	for (st = function_node->n.function.code; st; st = st->next)
		print_node(st->code);

	printf("}\n");
}

void
print_call(struct node *function_call_node)
{
	if (!function_call_node->n.function.code)
	{
		struct node *fn = abbreviation_lookup(function_call_node->n.function.func_name);
		function_call_node->n.call.function = fn;
	}

	printf("%s(", function_call_node->n.call.func_name);
	print_identifier_list(function_call_node->n.call.args);
	printf(")");
}

void
free_args_list(struct node *list)
{
	while (list)
	{
		struct node *tmp = list->mgmt;
		free_node(list);
		list = tmp;
	}
}
struct node *
new_operator(enum opNodeType op, struct node *left, struct node *right)
{
	struct node *n = new_node();
	n->typ = NT_OPERATOR;
	n->n.operator.op = op;
	n->n.operator.left = left;
	n->n.operator.right = right;
	return n;
}

void
print_control(struct node *control_node)
{
	struct stmnt *st;

	printf("%s (", control_node->n.control.typ == CT_IF? "if": "while");
	print_node(control_node->n.control.condition);
	printf(") {\n");

	if (control_node->n.control.true_block)
	{
		for (st = control_node->n.control.true_block; st; st = st->next)
			print_node(st->code);
	}

	if (control_node->n.control.false_block)
	{
		printf("} else {\n");
		for (st = control_node->n.control.false_block; st; st = st->next)
			print_node(st->code);
	}

	printf("}");
}
