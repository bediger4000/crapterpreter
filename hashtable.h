/* $Id: hashtable.h,v 1.2 2010/03/19 15:14:02 bediger Exp $ */

struct hashnode {
	struct hashnode *next, *prev;  /* hash chain */
	unsigned int value;            /* hash value and chain split count */
	int nodes_in_chain;
	char *string;
	size_t string_length;
	void *data;
};

/* array of hash chains consists of the "head" nodes in the chains.
 * I doubled up use of two of the hashnode struct elements for per-chain
 * counts of events of interests.  This may not be too smart. */

struct hashtable {
	int p; /* next bucket to split */
	int maxp; /* upper bound on p during this expansion */

	int currentsize;
	int allocated;

	int maxload;     /* ave lines per chain for a rehash */

	int node_cnt;    /* number of nodes in hashtable */

	int rehash_cnt;  /* number of rehashes - one chain at a time */
	
	struct hashnode **buckets;    /* array of hash chains */
	struct hashnode **sentinels;  /* array of hash chains */

	int flags;

	struct hashtable *next;
};

/* number of buckets has to be a power of 2 for this to work */
#define MOD(x,y)        ((x) & ((y)-1))

const char *add_string(struct hashtable *h, const char *key);
void *add_data(struct hashtable *h, const char *key, void *data);
void *data_lookup(struct hashtable *h, const char *key_string);
struct hashnode *node_lookup(
	struct hashtable *h, const char *string_to_lookup, unsigned int *hashval
);

struct hashtable *init_hashtable(int initial_buckets, int maxload);
void free_hashtable(struct hashtable *h);
