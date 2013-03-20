/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include "config.h"
#include <check.h>

#include <mm.h>
#include <misc.h>
#include <rdb.h>
#include <nnodes.h>
#include <ast.h>

static RDB *rdb = NULL;

/*
 * Core test suite
 */
START_TEST(load_conf) {
	fail_unless((rdb = rdb_read("db.xml")) != NULL, "load config file");
}
END_TEST

START_TEST(simple_a_resolv) {
	char req_data[] = /* 26 */
	{0x86,0xF3,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x77,0x77
	,0x77,0x04,0x74,0x65,0x73,0x74,0x00,0x00,0x01,0x00,0x01};
	char res_data[] = /* 50 */
	{0x86,0xF3,0x85,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x03,0x77,0x77
	,0x77,0x04,0x74,0x65,0x73,0x74,0x00,0x00,0x01,0x00,0x01,0x03,0x77,0x77,0x77
	,0x04,0x74,0x65,0x73,0x74,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x01,0x00
	,0x04,0x01,0x02,0x03,0x04};
	LOOKUP *lookup;
	char buffer[BUFSIZ];

	memset (buffer, 0, BUFSIZ);
	memcpy(buffer, req_data, sizeof(req_data));

	lookup = nnodes_lookup_init(rdb);
	nnodes_lookup(lookup, buffer, buffer, BUFSIZ);

	fail_unless(memcmp(buffer, res_data, sizeof(res_data)) == 0, "Simple A resolution");
}
END_TEST

START_TEST(multiple_a_resolv) {
	char req_data[] = /* 26 */
	{0xC8,0x7D,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x63,0x64
	,0x6E,0x04,0x74,0x65,0x73,0x74,0x00,0x00,0x01,0x00,0x01};
	char buffer[BUFSIZ];
	LOOKUP *lookup;
	int i;

	lookup = nnodes_lookup_init(rdb);
	for (i = 0; i != 1000000; i++) {
		memset (buffer, 0, BUFSIZ);
		memcpy(buffer, req_data, sizeof(req_data));

		nnodes_lookup(lookup, buffer, buffer, BUFSIZ);
	}

	fail_unless(1 == 1, "Multiple A resolution");
}
END_TEST


inline char *get_var_texte() {
	return "toto";
}

START_TEST(ast_simple) {
	AST_TREE *tree;
	AST_VAR vars[3] = {
			{ "$timestamp", AST_TYPE_INTEGER, { get_var_timestamp } },
			{ "$texte", AST_TYPE_STRING, { get_var_texte } },
			{ "", 0, { NULL } } };
	char str[255] = "cmp(\"toto\", $texte) and equal(1,1) and mod($timestamp,2)";
	int i = 0;

	tree = ast_mktree(str, vars);
	if (tree->begin) {
		printf("%d, %d...\n", ast_asktree(tree), 34 % 3);
	}

	for (i = 0; i != 10000000; i++) {
		ast_asktree(tree);
	}

	printf("%s\n", tree->error);

	ast_free(tree);

	fail_unless(1 == 1, "Ast simple");
}


Suite *nnodes_suite(void) {
	Suite *s = suite_create("nnodes");
	TCase *tc = tcase_create("core");

	tcase_add_test(tc, load_conf);
	tcase_add_test(tc, simple_a_resolv);
	tcase_add_test(tc, multiple_a_resolv);
	tcase_add_test(tc, ast_simple);

	suite_add_tcase(s, tc);

	return s;
}

int main () {
	int number_failed;
	Suite *s = nnodes_suite ();
	SRunner *sr = srunner_create(s);
	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? 0 : 255;
}
