/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __AST_H
#define __AST_H

#include <types.h>

typedef struct ast_arg_s AST_ARG;
typedef struct ast_args_s AST_ARGS;
typedef struct ast_node_s AST_NODE;
typedef struct ast_tree_s AST_TREE;

typedef struct ast_var_s AST_VAR;
typedef struct ast_const_s AST_CONST;
typedef struct ast_op_s AST_OP;

typedef struct ast_vars_list_s AST_VARS_LIST;
typedef struct ast_consts_list_s AST_CONSTS_LIST;
typedef struct ast_ops_list_s AST_OPS_LIST;

enum AST_TYPE {
	AST_TYPE_NODE = 1,
	AST_TYPE_VAR,
	AST_TYPE_INTEGER,
	AST_TYPE_STRING,
	AST_TYPE_POINTER
};

enum AST_OP_TYPE {
	AST_OP_TYPE_II = 1,
	AST_OP_TYPE_SS,
	AST_OP_TYPE_S,
	AST_OP_TYPE_I,
	AST_OP_TYPE_P
};

struct ast_var_s {
	char name[255];

	char type;
	union func_u {
		uint32_t (*integer)(void *);
		char *(*string)(void *);
	} func;

	struct ast_var_s *next;
};

struct ast_vars_list_s {
	struct ast_var_s *vars;
	struct ast_var_s *last;
};

struct ast_const_s {
	char name[255];

	char type;
	union cnst_u {
		uint32_t integer;
		char *string;
		void *pointer;
	} cnst;

	struct ast_const_s *next;
};
struct ast_consts_list_s {
	struct ast_const_s *consts;
	struct ast_const_s *last;
};

struct ast_arg_s {
	char type;
	union value_u {
		AST_NODE *node;
		AST_VAR *var;
		uint32_t integer;
		char *string;
		void *pointer;
	} value;
};

struct ast_args_s {
	AST_ARG *arg0;
	AST_ARG *arg1;
};

struct ast_ops_list_s {
	struct ast_op_s *ops;
	struct ast_op_s *last;
};

struct ast_tree_s {
	char *name;
	char *expr;

	char *crstr;

	uint32_t tksize;
	char *token;

	MM_POOL *pool;
	AST_NODE *begin;

	AST_VARS_LIST *vars;
	AST_CONSTS_LIST *consts;
	AST_OPS_LIST *ops;

	char error[255];
};

struct ast_op_s {
	char name[255];

	char type;
	char preproc;	/* preprocessus op or not */

	uint32_t (*func) (AST_TREE *, AST_ARGS *, void *);

	struct ast_op_s *next;
};

struct ast_node_s {
	char type;
	uint32_t (*func)(AST_TREE *, AST_ARGS *, void *);
	AST_ARGS *args;
};

/* op with two integer as args */
#define _AST_OP_II(FUNC, OP) \
uint32_t FUNC(AST_TREE *tree, AST_ARGS *args, void *context) { \
	AST_ARG *arg0 = args->arg0; \
	AST_ARG *arg1 = args->arg1; \
	int val0 = 3, val1 = 2; \
\
	if (arg0->type == AST_TYPE_INTEGER) \
		val0 = arg0->value.integer; \
	else if (arg0->type == AST_TYPE_VAR && arg0->value.var->type \
			== AST_TYPE_INTEGER) \
		val0 = arg0->value.var->func.integer(context); \
	else \
		snprintf(tree->error, sizeof (tree->error), \
				"syntax error, bad argument, unknow arg0 type: %d", \
				arg0->type); \
\
	if (arg1->type == AST_TYPE_INTEGER) \
		val1 = arg1->value.integer; \
	else if (arg1->type == AST_TYPE_VAR && arg1->value.var->type \
			== AST_TYPE_INTEGER) \
		val1 = arg1->value.var->func.integer(context); \
	else \
		snprintf(tree->error, sizeof (tree->error), \
				"syntax error, bad argument, unknow arg0 type: %d", \
				arg0->type); \
\
	OP \
}

/* op with one integer as args */
#define _AST_OP_I(FUNC, OP) \
uint32_t FUNC(AST_TREE *tree, AST_ARGS *args, void *context) { \
	AST_ARG *arg0 = args->arg0; \
	AST_ARG *arg1 = args->arg1; \
	int val0 = 3, val1 = 2; \
\
	if (arg0->type == AST_TYPE_INTEGER) \
		val0 = arg0->value.integer; \
	else if (arg0->type == AST_TYPE_VAR && arg0->value.var->type \
			== AST_TYPE_INTEGER) \
		val0 = arg0->value.var->func.integer(context); \
	else \
		snprintf(tree->error, sizeof (tree->error), \
				"syntax error, bad argument, unknow arg0 type: %d", \
				arg0->type); \
\
	OP \
}

/* op with one string as args */
#define _AST_OP_S(FUNC, OP) \
uint32_t FUNC(AST_TREE *tree, AST_ARGS *args, void *context) { \
	AST_ARG *arg0 = args->arg0; \
	AST_ARG *arg1 = args->arg1; \
	char *val0 = NULL, *val1 = NULL; \
\
	if (arg0->type == AST_TYPE_STRING) \
		val0 = arg0->value.string; \
	else if (arg0->type == AST_TYPE_VAR && arg0->value.var->type \
			== AST_TYPE_STRING) \
		val0 = arg0->value.var->func.string(context); \
	else \
		snprintf(tree->error, sizeof (tree->error), \
				"syntax error, bad argument, unknow arg0 type: %d", \
				arg0->type); \
\
	OP \
}

/* op with two string as args */
#define _AST_OP_SS(FUNC, OP) \
uint32_t FUNC(AST_TREE *tree, AST_ARGS *args, void *context) { \
	AST_ARG *arg0 = args->arg0; \
	AST_ARG *arg1 = args->arg1; \
	char *val0 = NULL, *val1 = NULL; \
\
	if (arg0->type == AST_TYPE_STRING) \
		val0 = arg0->value.string; \
	else if (arg0->type == AST_TYPE_VAR && arg0->value.var->type \
			== AST_TYPE_STRING) \
		val0 = arg0->value.var->func.string(context); \
	else \
		snprintf(tree->error, sizeof (tree->error), \
				"syntax error, bad argument, unknow arg0 type: %d", \
				arg0->type); \
\
	if (arg1->type == AST_TYPE_STRING) \
		val1 = arg1->value.string; \
	else if (arg1->type == AST_TYPE_VAR && arg1->value.var->type \
			== AST_TYPE_STRING) \
		val1 = arg1->value.var->func.string(context); \
	else \
		snprintf(tree->error, sizeof (tree->error),\
				"syntax error, bad argument, unknow arg0 type: %d", \
				arg0->type); \
\
	OP \
}

#define _AST_OP_P(FUNC, OP) \
uint32_t FUNC(AST_TREE *tree, AST_ARGS *args, void *context) { \
	AST_ARG *arg0 = args->arg0; \
	AST_ARG *arg1 = args->arg1; \
	char *val0 = NULL, *val1 = NULL; \
\
	if (arg0->type == AST_TYPE_POINTER) \
		val0 = arg0->value.pointer; \
	else \
		snprintf(tree->error, sizeof (tree->error),\
				"syntax error, bad argument, unknow arg0 type: %d", \
				arg0->type); \
\
	OP \
}

/* prototypes */
AST_TREE *ast_init (char *);
int ast_compil(AST_TREE *, char *, AST_VARS_LIST *, AST_CONSTS_LIST *, AST_OPS_LIST *);
void ast_free(AST_TREE *);
uint32_t ast_asktree(AST_TREE *, void *);
AST_VAR *ast_add_var (AST_VARS_LIST *, char *);
AST_CONST *ast_add_const (AST_CONSTS_LIST *, char *);
AST_OP *ast_add_op (AST_OPS_LIST *, char *);


#endif
