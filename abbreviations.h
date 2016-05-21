/* $Id: abbreviations.h,v 1.4 2010/03/26 21:01:40 bediger Exp $ */

void setup_abbreviation_table(struct hashtable *h);

struct node *abbreviation_lookup(const char *id);
void         abbreviation_add(const char *id, struct node *expr);

/* add a string to the function-local symbols */
const char  *add_local_string(const char *orig_string);

void *current_hashtable(void);

/* these get called during epilog and prolog
 * of an interpreted-function-call. */
void push_call_stack(void);
void pop_call_stack(void);
