/* $Id: stmnt.h,v 1.5 2010/04/07 12:33:09 bediger Exp $ */

struct context {
	struct stmnt *prgm;
	struct stmnt *prgm_tail;
	struct context *next; /* FIFO stack of contexts, and free list of contexts */
};

struct stmnt {
	int sn;
	struct node *code;
	struct stmnt *next;  /* linked list of struct stmnt in a basic block */
};

struct stmnt *new_stmnt(struct node *stmnt_code);
void free_stmnts(struct stmnt *stmnt_list);
void free_all_stmnt(void);
struct node *execute_prog(struct stmnt *stmnt_list);

struct context *new_context(void);
void free_context(struct context *basic_block);
void free_all_contexts(void);

struct stmnt *pop_context(struct context **stack_addr);
void push_context(struct context **address_of_stack);
void add_stmnt(struct context *stack, struct stmnt *st);
