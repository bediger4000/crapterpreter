/* $Id: abbreviations.c,v 1.5 2010/03/29 00:31:38 bediger Exp $ */


#include <stdio.h>
#include <stdlib.h>  /* malloc(), free() */

#include <node.h>
#include <hashtable.h>
#include <abbreviations.h>
#include <atom.h>

struct hashtable *abbr_table = NULL;

static struct hashtable *symbol_table_stack = NULL;

void *
current_hashtable(void)
{
	return (void *)symbol_table_stack;
}

void
setup_abbreviation_table(struct hashtable *h)
{
	abbr_table = h;
	symbol_table_stack = h;
}

struct node *
abbreviation_lookup(const char *id)
{
	struct node *r = NULL;
	struct hashtable *sp;
	void *p;

	for (sp = symbol_table_stack; sp; sp = sp->next)
	{
		p = data_lookup(sp, id);
		if (p)
		{
			r = (struct node *)p;
			break;
		}
	}

	return r;
}

void
abbreviation_add(const char *id, struct node *expr)
{
	struct hashnode *n = NULL;
	unsigned int hv;

	n = node_lookup(symbol_table_stack, id, &hv);
	if (n)
	{
		/* replace what already existed */
		free_node((struct node *)n->data);
		n->data = (void *)expr;
	} else {
		add_data(symbol_table_stack, id, (void *)expr);
	}
}

void
push_call_stack(void)
{
	struct hashtable *stack_frame = init_hashtable(28, 12);
	stack_frame->next = symbol_table_stack;
	symbol_table_stack = stack_frame;
}

void
pop_call_stack(void)
{
	struct hashtable *stack_frame = symbol_table_stack;
	symbol_table_stack = symbol_table_stack->next;
	free_hashtable(stack_frame);
	stack_frame = NULL;
}

/* Have a care: this keeps a *copy* of the argument string,
 * and that copy will get freed if/when the struct hashtable
 * pointed to by symbol_table_stack gets free'd
 * You shold probably only use this in the function-call
 * prolog code.
 */
const char *
add_local_string(const char *orig_string)
{
	const char *cpy_string = Atom_string(orig_string);
    return (const char *)add_data(symbol_table_stack, cpy_string, NULL);
}
