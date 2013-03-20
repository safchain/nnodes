/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <stdio.h>

#include <types.h>

#include <mm.h>
#include <misc.h>
#include <log.h>

#include "ast.h"

/* prototypes */
static AST_NODE *_ast_mkbranch(AST_TREE *);
static AST_NODE *_ast_mksubtree(AST_TREE *);

inline uint32_t ast_asktree(AST_TREE *tree, void *context) {
	if (tree->begin)
		return tree->begin->func(tree, tree->begin->args, context);

	return -1;
}

static int _ast_increase_buffer(char **ptr, uint32_t *sz, uint32_t inc) {
	if ((*ptr = realloc(ptr, *sz + inc)) == NULL)
		return -1;
	*sz += inc;

	return 0;
}

static char *_ast_nextok(AST_TREE *tree) {
	uint32_t offset = 0;
	char c, q = '\0';

	if (*(tree->crstr) == '\0')
		return NULL;

	while ((c = *(tree->crstr++)) != '\0') {
		if (c != ' ' && c != '\t' && c != ',') {
			if (offset > tree->tksize - 1)
				if (_ast_increase_buffer(&tree->token, &tree->tksize, 10) == -1) {
					snprintf(tree->error, sizeof(tree->error),
							"memory allocation error");
					return NULL;
				}

			*(tree->token + offset++) = c;
			break;
		}
	}

	if (c == '(' || c == ')' || c == '!' || c == ',') {
		*(tree->token + offset) = '\0';
		return tree->crstr;
	}

	if (c == '\'' || c == '"')
		q = c;
	else if (c == '$') {
		c = *(tree->crstr);
		if (c == '{')
			q = '}';
	}

	while ((c = *(tree->crstr)) != '\0') {
		if (offset > tree->tksize - 1)
			if (_ast_increase_buffer(&tree->token, &tree->tksize, 10) == -1) {
				snprintf(tree->error, sizeof(tree->error),
						"memory allocation error");
				return NULL;
			}

		if (!q && (c == ' ' || c == '\t' || c == ',' || c == '(' || c == ')')) {
			*(tree->token + offset) = '\0';
			return tree->crstr;
		}

		*(tree->token + offset++) = c;
		tree->crstr++;

		if (q && q == c) {
			*(tree->token + offset) = '\0';
			return tree->crstr;
		}
	}
	*(tree->token + offset) = '\0';

	return tree->crstr;
}

static AST_VAR *_ast_get_var(AST_TREE *tree, char *name) {
	AST_VAR *var = tree->vars->vars;
	int len;

	while (var) {
		if (! (var->type == AST_TYPE_STRING || var->type == AST_TYPE_INTEGER || var->type == AST_TYPE_POINTER))
			break;

		/* skip { } */
		if (*name == '{') {
			len = strlen(var->name);

			if (strncmp(name + 1, var->name, len) == 0)
				return var;
		}
		else if (strcmp(name, var->name) == 0)
				return var;
		var = var->next;
	}

	return NULL;
}

static AST_CONST *_ast_get_const(AST_TREE *tree, char *name) {
	AST_CONST *cst = tree->consts->consts;
	int len;

	while (cst) {
		if (! (cst->type == AST_TYPE_STRING || cst->type == AST_TYPE_INTEGER || cst->type == AST_TYPE_POINTER))
			break;
		/* skip { } */
		if (*name == '{') {
			len = strlen(cst->name);

			if (strncmp(name + 1, cst->name, len) == 0)
				return cst;
		}
		else if (strcmp(name, cst->name) == 0)
				return cst;
		cst = cst->next;
	}

	return NULL;
}

static AST_OP *_ast_get_op(AST_TREE *tree, char *name) {
	AST_OP *op = tree->ops->ops;
	int len;

	while (op) {
		if (strcmp(name, op->name) == 0)
			return op;
		op = op->next;
	}

	return NULL;
}

static inline uint32_t _ast_or(AST_TREE *tree, AST_ARGS *args, void *context) {
	AST_NODE *left, *right;

	left = args->arg0->value.node;
	right = args->arg1->value.node;

	return (left->func(tree, left->args, context) || right->func(tree, right->args, context));
}

static inline uint32_t _ast_and(AST_TREE *tree, AST_ARGS *args, void *context) {
	AST_NODE *left, *right;

	left = args->arg0->value.node;
	right = args->arg1->value.node;

	return (left->func(tree, left->args, context) && right->func(tree, right->args, context));
}

static inline uint32_t _ast_op_true(AST_TREE *tree, AST_ARGS *args, void *context) {
	return 1;
}

static inline uint32_t _ast_op_false(AST_TREE *tree, AST_ARGS *args, void *context) {
	return 0;
}

static AST_NODE *_ast_mknode(AST_TREE *tree, uint32_t (*func) (AST_TREE *, AST_ARGS *, void *), AST_ARGS *args) {
	AST_NODE *node;

	if ((node = mm_pool_alloc0(tree->pool, sizeof (AST_NODE))) == NULL)
		return NULL;

	node->type = AST_TYPE_NODE;
	node->func = func;
	node->args = args;

	return node;
}

static int _ast_set_arg(AST_TREE *tree, int type, AST_ARG *arg) {
	AST_VAR *var;
	AST_CONST *cst;
	int value, len;
	char *end = NULL, *ptr;

	if (*tree->token == '$') {
		/* search var without starting $ */
		if ((var = _ast_get_var(tree, tree->token + 1)) != NULL) {

			if (var->type != type) {
				snprintf(tree->error, sizeof(tree->error),
						"syntax error, var type mismatch: %s", tree->token);
				return -1;
			}
			arg->type = AST_TYPE_VAR;

			arg->value.var = var;
		} else if ((cst = _ast_get_const(tree, tree->token + 1)) != NULL) {
			arg->type = cst->type;
			switch (cst->type) {
			case AST_TYPE_INTEGER:
				arg->value.integer = cst->cnst.integer;
				break;
			case AST_TYPE_STRING:
				arg->value.string = cst->cnst.string;
				break;
			case AST_TYPE_POINTER:
				arg->value.pointer = cst->cnst.pointer;
				break;
			default:
				snprintf(tree->error, sizeof(tree->error),
						"syntax error, unknow var/const: %s", tree->token);
				return -1;
			}
		}
		else {
			snprintf(tree->error, sizeof(tree->error),
					"syntax error, unknow var/const: %s", tree->token);
			return -1;
		}
	} else {
		switch (type) {
		case AST_TYPE_INTEGER:
			arg->type = AST_TYPE_INTEGER;

			value = strtol(tree->token, &end, 10);
			if ((errno == ERANGE && (value == LONG_MAX || value == LONG_MIN)) || *end != '\0') {
				snprintf(tree->error, sizeof(tree->error),
						"syntax error, bad argument: %s", tree->token);
				return -1;
			}
			arg->value.integer = value;

			break;
		case AST_TYPE_STRING:
			arg->type = AST_TYPE_STRING;

			len = strlen(tree->token);
			*(tree->token + len - 1) = '\0';

			if ((ptr = str_dup_pool_alloc(tree->pool, tree->token + 1)) == NULL) {
				snprintf(tree->error, sizeof(tree->error),
						"memory allocation error: %s", tree->token);
				return -1;
			}
			arg->value.string = ptr;

			break;
		default:
			snprintf(tree->error, sizeof(tree->error), "unknow type: %s",
					tree->token);
			return -1;
			break;
		}
	}

	return 0;
}

static AST_NODE *_ast_mkop_2(AST_TREE *tree, char type_1, char type_2, uint32_t (*func) (AST_TREE *, AST_ARGS *, void *)) {
	AST_ARGS *args;

	if ((args = (AST_ARGS *) mm_pool_alloc0(tree->pool, sizeof (AST_ARGS))) == NULL)
		return NULL;

	if (_ast_nextok(tree) == NULL || strcmp(tree->token, "(") != 0) {
		snprintf(tree->error, sizeof (tree->error),
				"syntax error");
		return NULL;
	}

	/* arg 0 */
	if (_ast_nextok(tree) == NULL || strcmp(tree->token, "(") == 0) {
		snprintf(tree->error, sizeof (tree->error),
				"syntax error, function does not support sub expression");
		return NULL;
	}

	if ((args->arg0 = (AST_ARG *) mm_pool_alloc0(tree->pool, sizeof (AST_ARG))) == NULL)
		return NULL;

	if (_ast_set_arg(tree, type_1, args->arg0) == -1)
		return NULL;

	/* arg 1 */
	if (_ast_nextok(tree) == NULL || strcmp(tree->token, "(") == 0) {
		snprintf(tree->error, sizeof (tree->error),
				"syntax error, function does not support sub expression");
		return NULL;
	}

	if ((args->arg1 = (AST_ARG *) mm_pool_alloc0(tree->pool, sizeof (AST_ARG))) == NULL)
		return NULL;

	if (_ast_set_arg(tree, type_2, args->arg1) == -1)
		return NULL;

	if (_ast_nextok(tree) == NULL || strcmp(tree->token, ")") != 0) {
		snprintf(tree->error, sizeof (tree->error),
				"syntax error");
		return NULL;
	}

	return _ast_mknode(tree, func, args);
}

static AST_NODE *_ast_mkop_1(AST_TREE *tree, char type_1, uint32_t (*func) (AST_TREE *, AST_ARGS *, void *)) {
	AST_ARGS *args;

	if ((args = (AST_ARGS *) mm_pool_alloc0(tree->pool, sizeof (AST_ARGS))) == NULL)
		return NULL;

	if (_ast_nextok(tree) == NULL || strcmp(tree->token, "(") != 0) {
		snprintf(tree->error, sizeof (tree->error),
				"syntax error");
		return NULL;
	}

	/* arg 0 */
	if (_ast_nextok(tree) == NULL || strcmp(tree->token, "(") == 0) {
		snprintf(tree->error, sizeof (tree->error),
				"syntax error, function does not support sub expression");
		return NULL;
	}

	if ((args->arg0 = (AST_ARG *) mm_pool_alloc0(tree->pool, sizeof (AST_ARG))) == NULL)
		return NULL;

	if (_ast_set_arg(tree, type_1, args->arg0) == -1)
		return NULL;

	if (_ast_nextok(tree) == NULL || strcmp(tree->token, ")") != 0) {
		snprintf(tree->error, sizeof (tree->error),
				"syntax error");
		return NULL;
	}

	return _ast_mknode(tree, func, args);
}

static inline uint32_t _ast_op_not(AST_TREE *tree, AST_ARGS *args, void *context) {
	AST_NODE *left;

	left = args->arg0->value.node;

	return !(left->func(tree, left->args, context));
}

static AST_NODE *_ast_mkop_not(AST_TREE *tree) {
	AST_NODE *node = NULL;
	AST_ARGS *args;

	if ((args = (AST_ARGS *) mm_pool_alloc0(tree->pool, sizeof (AST_ARGS))) == NULL)
		return NULL;

	if ((args->arg0 = (AST_ARG *) mm_pool_alloc0(tree->pool, sizeof (AST_ARG))) == NULL)
		return NULL;

	if ((node = _ast_mkbranch(tree)) == NULL)
		return NULL;

	args->arg0->type = AST_TYPE_NODE;
	args->arg0->value.node = node;

	return _ast_mknode(tree, _ast_op_not, args);
}

static inline uint32_t _ast_op_value(AST_TREE *tree, AST_ARGS *args, void *context) {
	AST_ARG *arg0 = args->arg0;

	if (arg0->type == AST_TYPE_INTEGER) {
		if (arg0->value.integer)
			return 1;
	} else if (arg0->type == AST_TYPE_STRING) {
		if (*(arg0->value.string) != '\0')
			return 1;
	}

	return 0;
}

static AST_NODE *_ast_mkop_value(AST_TREE *tree, int type) {
	AST_ARGS *args;

	if ((args = (AST_ARGS *) mm_pool_alloc0(tree->pool, sizeof (AST_ARGS))) == NULL)
		return NULL;

	/* arg 0 */
	if ((args->arg0 = (AST_ARG *) mm_pool_alloc0(tree->pool, sizeof (AST_ARG))) == NULL)
		return NULL;

	if (_ast_set_arg(tree, type, args->arg0) == -1)
		return NULL;

	return _ast_mknode(tree, _ast_op_value, args);
}

_AST_OP_II(_ast_op_mod, if (val0 % val1 == 0) return 1; else return 0;)
_AST_OP_II(_ast_op_equal, if (val0 == val1) return 1; else return 0;)
_AST_OP_SS(_ast_op_cmp, if (strcmp (val0, val1) == 0) return 1; else return 0;)
_AST_OP_SS(_ast_op_sub, if (strstr (val0, val1) != NULL) return 1; else return 0;)
_AST_OP_P(_ast_op_warn, printf("%s\n", val0); return 1; )

static AST_NODE *_ast_mkop(AST_TREE *tree) {
	AST_OP *op;

	if (strcasecmp(tree->token, "mod") == 0)
		return _ast_mkop_2(tree, AST_TYPE_INTEGER, AST_TYPE_INTEGER, _ast_op_mod);
	else if (strcasecmp(tree->token, "equal") == 0)
		return _ast_mkop_2(tree, AST_TYPE_INTEGER, AST_TYPE_INTEGER, _ast_op_equal);
	else if (strcasecmp(tree->token, "cmp") == 0)
		return _ast_mkop_2(tree, AST_TYPE_STRING, AST_TYPE_STRING, _ast_op_cmp);
	else if (strcasecmp(tree->token, "sub") == 0)
		return _ast_mkop_2(tree, AST_TYPE_STRING, AST_TYPE_STRING, _ast_op_sub);
	else if (strcasecmp(tree->token, "warn") == 0)
		return _ast_mkop_1(tree, AST_TYPE_POINTER, _ast_op_warn);
	else if (strcasecmp(tree->token, "not") == 0 || strcmp(tree->token, "!") == 0)
		return _ast_mkop_not(tree);
	else {
		if ((op = _ast_get_op(tree, tree->token))) {
			switch (op->type) {
			case AST_OP_TYPE_P:
				return _ast_mkop_1(tree, AST_TYPE_POINTER, op->func);
			case AST_OP_TYPE_II:
				return _ast_mkop_2(tree, AST_TYPE_INTEGER, AST_TYPE_INTEGER, op->func);
			case AST_OP_TYPE_SS:
				return _ast_mkop_2(tree, AST_TYPE_STRING, AST_TYPE_STRING, op->func);
			case AST_OP_TYPE_I:
				return _ast_mkop_1(tree, AST_TYPE_INTEGER, op->func);
			case AST_OP_TYPE_S:
				return _ast_mkop_1(tree, AST_TYPE_STRING, op->func);
			}
		}
		snprintf(tree->error, sizeof (tree->error),
				"syntax error, unknow operande, check near: %s", tree->token);
	}
	return NULL;
}

static AST_NODE *_ast_mkbranch(AST_TREE *tree) {
	AST_NODE *node = NULL;
	AST_ARGS *args;
	char *end = NULL;
	int value;

	if ((args = (AST_ARGS *) mm_pool_alloc0(tree->pool, sizeof (AST_ARGS))) == NULL)
		return NULL;

	if (_ast_nextok(tree) == NULL) {
		snprintf(tree->error, sizeof (tree->error), "syntax error, unknow reason");
		return NULL;
	}

	if (strcmp(tree->token, "(") == 0) {
		if ((node = _ast_mksubtree(tree)) == NULL)
		return NULL;
	} else if (strcmp(tree->token, ")") == 0)
		return NULL;
	else if (*tree->token == '\'' || *tree->token == '"') {
		if ((node = _ast_mkop_value(tree, AST_TYPE_STRING)) == NULL)
			return NULL;
	} else {
		value = strtol(tree->token, &end, 10);
		if ((errno == ERANGE && (value == LONG_MAX || value == LONG_MIN)) || *end != '\0') {
			if ((node = _ast_mkop(tree)) == NULL)
				return NULL;
		} else if ((node = _ast_mkop_value(tree, AST_TYPE_INTEGER)) == NULL)
			return NULL;
	}

	return node;
}

static AST_NODE *_ast_mksubtree(AST_TREE *tree) {
	AST_NODE *left = NULL, *right = NULL;
	AST_ARGS *args;

	if ((args = mm_pool_alloc0(tree->pool, sizeof (AST_ARGS))) == NULL)
		goto clean;
	if ((args->arg0 = mm_pool_alloc0(tree->pool, sizeof (AST_ARG))) == NULL)
		goto clean;
	if ((args->arg1 = mm_pool_alloc0(tree->pool, sizeof (AST_ARG))) == NULL)
		goto clean;

	if ((left = _ast_mkbranch(tree)) == NULL)
		goto clean;

	if (_ast_nextok(tree) == NULL)
		goto clean;

	if (strcasecmp(tree->token, "or") == 0) {
		if ((right = _ast_mksubtree(tree)) == NULL)
			goto clean;

		args->arg0->type = AST_TYPE_NODE;
		args->arg0->value.node = left;

		args->arg1->type = AST_TYPE_NODE;
		args->arg1->value.node = right;

		left = _ast_mknode(tree, _ast_or, args);
	} else if (strcasecmp(tree->token, "and") == 0) {
		if ((right = _ast_mksubtree(tree)) == NULL)
			goto clean;

		args->arg0->type = AST_TYPE_NODE;
		args->arg0->value.node = left;

		args->arg1->type = AST_TYPE_NODE;
		args->arg1->value.node = right;

		left = _ast_mknode(tree, _ast_and, args);
	} else if (strcmp(tree->token, ")") != 0) {
		snprintf(tree->error, sizeof (tree->error),
				"syntax error, check near: %s", tree->token);
	}

clean:
	return left;
}

AST_OP *ast_add_op (AST_OPS_LIST *list, char *name) {
	AST_OP *op;

	if ((op = calloc(1, sizeof(AST_OP))) == NULL)
		return NULL;
	strncpy(op->name, name, sizeof(op->name) - 1);

	if (list->ops) {
		list->last->next = op;
		list->last = op;
	}
	else {
		list->ops = op;
		list->last = op;
	}

	return op;
}

AST_VAR *ast_add_var (AST_VARS_LIST *list, char *name) {
	AST_VAR *var;

	if ((var = calloc(1, sizeof(AST_VAR))) == NULL)
		return NULL;
	strncpy(var->name, name, sizeof(var->name) - 1);

	if (list->vars) {
		list->last->next = var;
		list->last = var;
	}
	else {
		list->vars = var;
		list->last = var;
	}

	return var;
}

AST_CONST *ast_add_const (AST_CONSTS_LIST *list, char *name) {
	AST_CONST *conzt;

	if ((conzt = calloc(1, sizeof(AST_CONST))) == NULL)
		return NULL;
	strncpy(conzt->name, name, sizeof(conzt->name) - 1);

	if (list->consts) {
		list->last->next = conzt;
		list->last = conzt;
	}
	else {
		list->consts = conzt;
		list->last = conzt;
	}

	return conzt;
}

AST_TREE *ast_init (char *expr) {
	AST_TREE *tree;

	if ((tree = (AST_TREE *) calloc(1, sizeof(AST_TREE))) == NULL)
		return NULL;

	tree->tksize = 128;
	if ((tree->token = (char *) calloc(tree->tksize, sizeof(char))) == NULL)
		goto clean;

	if ((tree->pool = mm_pool_new()) == NULL)
		goto clean;

	if (expr) {
		tree->expr = expr;
		tree->crstr = expr;
	}

	return tree;

clean:
	if (tree->token)
		mm_free0((void *) &tree->token);

	return NULL;
}

int ast_compil(AST_TREE *tree, char *expr, AST_VARS_LIST *vars, AST_CONSTS_LIST *consts, AST_OPS_LIST *ops) {
	AST_NODE *node;

	if (expr) {
		tree->expr = expr;
		tree->crstr = expr;
	}
	if (tree->expr != NULL) {

		tree->vars = vars;
		tree->consts = consts;
		tree->ops = ops;

		if ((node = _ast_mksubtree(tree)) == NULL)
			return -1;
	} else
		node = _ast_mknode(tree, _ast_op_true, NULL);
	tree->begin = node;

	return 0;
}

void ast_free(AST_TREE *tree) {
	if (tree->token)
		free(tree->token);

	mm_pool_delete(tree->pool);

	free(tree);
}

