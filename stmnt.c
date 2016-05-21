/* $Id: stmnt.c,v 1.8 2010/04/07 12:33:09 bediger Exp $ */

#include <stdio.h>
#include <stdlib.h>

#include <node.h>
#include <stmnt.h>

static struct stmnt *free_stmnt_list = NULL;
static int allocated_stmnt_cnt = 0;

static struct context *free_context_list = NULL;
static int allocated_context_cnt = 0;

struct stmnt *
new_stmnt(struct node *stmnt_code)
{
	struct stmnt *st = NULL;

	
	if (free_stmnt_list)
	{
		st = free_stmnt_list;
		free_stmnt_list = free_stmnt_list->next;
	} else {
		st = malloc(sizeof(*st));
		++allocated_stmnt_cnt;
		st->sn = allocated_stmnt_cnt;
	}

	st->code = stmnt_code;
	st->next = NULL;

	return st;
}

void
free_stmnts(struct stmnt *stmnt_list)
{
	while (stmnt_list)
	{
		struct stmnt *tmp = stmnt_list->next;

		free_node(stmnt_list->code);
		stmnt_list->code = NULL;

		stmnt_list->next = free_stmnt_list;
		free_stmnt_list = stmnt_list;

		stmnt_list = tmp;
	}
}

void
free_all_stmnt(void)
{
	int freed_stmnt_cnt = 0;
	while (free_stmnt_list)
	{
		struct stmnt *tmp = free_stmnt_list->next;
		free(free_stmnt_list);
		free_stmnt_list = tmp;
		++freed_stmnt_cnt;
	}

	if (freed_stmnt_cnt != allocated_stmnt_cnt)
		printf("Allocated %d statements, but freed only %d\n",
			allocated_stmnt_cnt, freed_stmnt_cnt);
}

struct node *
execute_prog(struct stmnt *stmnt_list)
{
	struct stmnt *p;
	struct node *result = NULL;

	for (p = stmnt_list; p; p = p->next)
	{
		if (result) free_node(result);
		result = execute_code(p->code);
		if (result && result->return_now)
			break;
	}
	return result;
}
struct context *
new_context(void)
{
	struct context *ctxt = NULL;

	
	if (free_context_list)
	{
		ctxt = free_context_list;
		free_context_list = free_context_list->next;
	} else {
		ctxt = malloc(sizeof(*ctxt));
		++allocated_context_cnt;
	}

	ctxt->prgm = NULL;
	ctxt->prgm_tail = NULL;
	ctxt->next = NULL;

	return ctxt;
}

/* Free a single basic block (i.e. a struct context */
void
free_context(struct context *basic_block)
{
	if (basic_block)
	{
		free_stmnts(basic_block->prgm);
		basic_block->prgm = basic_block->prgm_tail = NULL;

		basic_block->next = free_context_list;
		free_context_list = basic_block;
	}
}

void
free_all_contexts(void)
{
	int freed_context_cnt = 0;

	while (free_context_list)
	{
		struct context *tmp = free_context_list->next;
		free(free_context_list);
		++freed_context_cnt;
		free_context_list = tmp;
	}

	if (freed_context_cnt != allocated_context_cnt)
	{
		printf("Allocated %d contexts, but freed only %d\n",
			allocated_context_cnt, freed_context_cnt);
	}
}

struct stmnt *
pop_context(struct context **stack_addr)
{
	struct context *curr = *stack_addr;
	struct stmnt *retval = NULL;
	if (curr)
	{
		retval = curr->prgm;
		*stack_addr = (*stack_addr)->next;
		curr->prgm = curr->prgm_tail = NULL;
		free_context(curr);
	}
	return retval;
}

void
push_context(struct context **address_of_stack)
{
	struct context *n = new_context();
	n->next = *address_of_stack;
	*address_of_stack = n;
}

void
add_stmnt(struct context *stack, struct stmnt *st)
{
	if (stack->prgm_tail)
	{
		stack->prgm_tail->next = st;
		stack->prgm_tail = st;
	} else {
		stack->prgm = st;
		stack->prgm_tail = st;
	}
}
