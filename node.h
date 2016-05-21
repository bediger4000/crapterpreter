/* $Id: node.h,v 1.9 2010/04/07 12:33:09 bediger Exp $ */

enum nodeType { NT_FREE, NT_READY, NT_OPERATOR, NT_VALUE, NT_FUNCTION, NT_CALL, NT_CONTROL};
enum valueType { VT_NUMBER, VT_STRING, VT_IDENTIFIER, VT_BOOLEAN }; 
enum opNodeType { OP_NULL, OP_ASSIGN, OP_ADD, OP_SUB, OP_CONCAT, OP_RETURN, OP_OUTPUT,
	OP_MULT, OP_DIV,
	OP_AND, OP_OR, OP_NOT,
	OP_EQUALITY, OP_GREATER, OP_LESSER
	 };
enum controlType { CT_IF, CT_WHILE };

enum booleanType {BOOL_FALSE = 0, BOOL_TRUE = 1};

struct node {
	struct node *mgmt;
	enum nodeType typ;
	int return_now;
	union {
		struct {
			enum opNodeType op;
			struct node *left;
			struct node *right;
		} operator;

		struct {
			enum valueType typ;
			union {
				enum booleanType boolean;
				int number;
				const char *string;
				const char *identifier;
			} v;
		} value;
		struct {
			enum controlType typ;
			struct node *condition;
			struct stmnt *true_block;
			struct stmnt *false_block;
		} control;
		struct {
			const char *func_name;
			struct node *formal_args;
			struct node *args;
			struct stmnt *code;
		} function;
		struct {
			const char *func_name;
			struct node *args;
			struct node *function;
		} call;
	} n;
};

struct node *new_node(void);
struct node *new_identifier(const char *id);
struct node *new_operator(enum opNodeType op, struct node *left, struct node *right);
void free_node(struct node *tree);
void free_args_list(struct node *list_head);
void free_all_nodes(void);
struct node *execute_code(struct node *code);
void print_node(struct node *node);
