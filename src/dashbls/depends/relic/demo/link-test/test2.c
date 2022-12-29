#include <relic.h>

void function2() {
	g1_t p;
	g2_t q;
	gt_t e;

	g1_null(p);
	g2_null(q);
	gt_null(e);

	core_init();

	RLC_TRY {
		core_init();
		pc_param_set_any();
		pc_param_print();

		g1_new(p);
		g2_new(q);
		gt_new(e);

		g1_rand(p);
		g2_rand(q);
		pc_map(e, p, q);

		gt_print(e);
		printf("Is pairing symmetric? %d\n", pc_map_is_type1() == 1);
		printf("Is pairing asymmetric? %d\n", pc_map_is_type3() == 1);
	} RLC_CATCH_ANY {
	} RLC_FINALLY {
		g1_free(p);
		g2_free(q);
		gt_free(e);
	}
	core_clean();
}
