#include "../clutils.h"

/* ************** */
/* Aux. functions */
/* ************** */

cl_int errorL2Aux(cl_int code, const char* xtramsg, GError **err) {
	clu_if_error_create_error_return(code, err, "Big error in level %d function: %s", 2, xtramsg);
	return CL_SUCCESS;
}

cl_int errorL1Aux(cl_int code, GError **err) {
	cl_int status = errorL2Aux(code, "called by errorL1Aux", err);
	clu_if_error_return(status, err);
	return CL_SUCCESS;
}

/* ************** */
/* Test functions */
/* ************** */

static void errorOneLevelTest() {
	GError *err = NULL;

	cl_int status = errorL2Aux(CL_INVALID_VALUE, "called by errorOneLevelTest", &err);
	clu_if_error_goto(status, err, error);

	g_assert_not_reached();
	goto cleanup;

error:
	g_assert_error(err, CLU_UTILS_ERROR, CL_INVALID_VALUE);
	g_assert_cmpstr(err->message, ==, "Big error in level 2 function: called by errorOneLevelTest");
	g_error_free(err);

cleanup:
	return;
	
}

static void errorTwoLevelTest() {
	GError *err = NULL;

	cl_int status = errorL1Aux(CL_INVALID_HOST_PTR, &err);
	clu_if_error_goto(status, err, error);

	g_assert_not_reached();
	goto cleanup;

error:
	g_assert_error(err, CLU_UTILS_ERROR, CL_INVALID_HOST_PTR);
	g_assert_cmpstr(err->message, ==, "Big error in level 2 function: called by errorL1Aux");
	g_error_free(err);

cleanup:
	return;
}

static void errorNoneTest() {
	GError *err = NULL;

	cl_int status = errorL2Aux(CL_SUCCESS, "called by errorOneLevelTest", &err);
	clu_if_error_goto(status, err, error);

	goto cleanup;

error:
	g_assert_not_reached();
	g_error_free(err);

cleanup:
	g_assert_no_error(err);
	return;
}

static void errorNoVargsTest() {
	GError *err = NULL;

	clu_if_error_create_error_goto(CL_INVALID_DEVICE_TYPE, &err, error, "I have no additional arguments");

	g_assert_not_reached();
	goto cleanup;

error:
	g_assert_error(err, CLU_UTILS_ERROR, CL_INVALID_DEVICE_TYPE);
	g_assert_cmpstr(err->message, ==, "I have no additional arguments");
	g_error_free(err);

cleanup:
	return;
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/utils/error-onelevel", errorOneLevelTest);
	g_test_add_func("/utils/error-twolevel", errorTwoLevelTest);
	g_test_add_func("/utils/error-none", errorNoneTest);
	g_test_add_func("/utils/error-novargs", errorNoVargsTest);
	return g_test_run();
}
