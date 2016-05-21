/* $Id: atom.c,v 1.1.1.1 2010/03/12 03:22:38 bediger Exp $ */
#include <string.h>
#include <hashtable.h>
#include <atom.h>


static struct hashtable *atom_table = NULL;

void
setup_atom_table(struct hashtable *h)
{
	atom_table = h;
}

const char *
Atom_string(const char *str)
{
	return add_string(atom_table, str);
}
