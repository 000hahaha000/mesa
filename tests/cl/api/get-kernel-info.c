/*
 * Copyright © 2012 Blaž Tomažič <blaz.tomazic@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * @file get-kernel-info.c
 *
 * Test API function:
 *
 *   cl_int clGetKernelInfo (cl_kernel kernel,
 *                           cl_kernel_info param_name,
 *                           size_t param_value_size,
 *                           void *param_value,
 *                           size_t *param_value_size_ret)
 */

#include "piglit-framework-cl-api.h"


PIGLIT_CL_API_TEST_CONFIG_BEGIN

	config.name = "clGetKernelInfo";
	config.version_min = 10;

	config.run_per_platform = true;
	config.create_context = true;

	config.program_source =  "kernel __attribute__((work_group_size_hint(1, 1, 1))) void dummy_kernel(int i) {}\n";

PIGLIT_CL_API_TEST_CONFIG_END


static bool
check_size(size_t expected_size, size_t actual_size, enum piglit_result *result) {
	if (expected_size != actual_size) {
		printf(": failed, expected and actual size differ. Expect %lu, got %lu",
		       expected_size, actual_size);
		piglit_merge_result(result, PIGLIT_FAIL);
		return false;
	}

	return true;
}

static bool
check_string(char* expected, char* actual_value, enum piglit_result *result) {
	if (strcmp(expected, actual_value)) {
		printf(": failed, expected and actual string differ. Expect '%s', got '%s'",
		       expected, actual_value);
		piglit_merge_result(result, PIGLIT_FAIL);
		return false;
	}

	return true;
}

static void
check_info(const struct piglit_cl_api_test_env* env,
	   cl_kernel_info kind, void* param_value, size_t param_value_size,
	   enum piglit_result *result) {
	switch (kind) {
		case CL_KERNEL_FUNCTION_NAME:
			if (check_size(strlen("dummy_kernel") + 1, param_value_size, result)) {
				check_string("dummy_kernel", param_value, result);
			}
			break;
		case CL_KERNEL_NUM_ARGS:
			if (check_size(sizeof(cl_uint), param_value_size, result)) {
				if (*(cl_uint*)param_value != 1) {
					printf(": failed, expected and actual value differ. Expect '%d', got '%d'",
					1, *(cl_uint*)param_value);
					piglit_merge_result(result, PIGLIT_FAIL);
				}
			}
			break;
		case CL_KERNEL_REFERENCE_COUNT:
			if (check_size(sizeof(cl_uint), param_value_size, result)) {
				if (*(cl_uint*)param_value < 1) {
					printf(": failed, expected and actual value differ. Expect >='%d', got '%d'",
					1, *(cl_uint*)param_value);
					piglit_merge_result(result, PIGLIT_FAIL);
				}
			}
			break;
		case CL_KERNEL_CONTEXT:
			if (check_size(sizeof(cl_context), param_value_size, result)) {
				if (*(cl_context*)param_value != env->context->cl_ctx) {
					printf(": failed, expected and actual context differ");
					piglit_merge_result(result, PIGLIT_FAIL);
				}
			}
			break;
		case CL_KERNEL_PROGRAM:
			if (check_size(sizeof(cl_program), param_value_size, result)) {
				if (*(cl_program*)param_value != env->program) {
					printf(": failed, expected and actual program differ");
					piglit_merge_result(result, PIGLIT_FAIL);
				}
			}
			break;
		case CL_KERNEL_ATTRIBUTES:
			if (check_size(strlen("work_group_size_hint(1,1,1)") + 1, param_value_size, result)) {
				check_string("work_group_size_hint(1,1,1)", param_value, result);
			}
			break;

		default:
			printf(": WARN unchecked value");
			piglit_merge_result(result, PIGLIT_WARN);
	}
}

enum piglit_result
piglit_cl_test(const int argc,
               const char** argv,
               const struct piglit_cl_api_test_config* config,
               const struct piglit_cl_api_test_env* env)
{
	enum piglit_result result = PIGLIT_PASS;

	int i;
	cl_int errNo;
	cl_kernel kernel;

	size_t param_value_size;
	void* param_value;

	int num_kernel_infos = PIGLIT_CL_ENUM_NUM(cl_kernel_info, env->version);
	const cl_kernel_info* kernel_infos = PIGLIT_CL_ENUM_ARRAY(cl_kernel_info);

	kernel = clCreateKernel(env->program,
	                        "dummy_kernel",
	                        &errNo);
	if(!piglit_cl_check_error(errNo, CL_SUCCESS)) {
		fprintf(stderr,
		        "Failed (error code: %s): Create kernel.\n",
		        piglit_cl_get_error_name(errNo));
		return PIGLIT_FAIL;
	}

	/*** Normal usage ***/
	for(i = 0; i < num_kernel_infos; i++) {
		printf("%s", piglit_cl_get_enum_name(kernel_infos[i]));

		errNo = clGetKernelInfo(kernel,
		                        kernel_infos[i],
		                        0,
		                        NULL,
		                        &param_value_size);
		if(!piglit_cl_check_error(errNo, CL_SUCCESS)) {
			fprintf(stderr,
			        "failed (error code: %s): Get size of %s.\n",
			        piglit_cl_get_error_name(errNo),
			        piglit_cl_get_enum_name(kernel_infos[i]));
			piglit_merge_result(&result, PIGLIT_FAIL);
			continue;
		}

		param_value = malloc(param_value_size);
		errNo = clGetKernelInfo(kernel,
		                        kernel_infos[i],
		                        param_value_size,
		                        param_value,
		                        NULL);
		if(!piglit_cl_check_error(errNo, CL_SUCCESS)) {
			fprintf(stderr,
			        "failed (error code: %s): Get value of %s.\n",
			        piglit_cl_get_error_name(errNo),
			        piglit_cl_get_enum_name(kernel_infos[i]));
			piglit_merge_result(&result, PIGLIT_FAIL);
		}

		check_info(env, kernel_infos[i], param_value, param_value_size, &result);

		printf("\n");
		free(param_value);
	}

	/*** Errors ***/

	/*
	 * CL_INVALID_VALUE if param_name is not one of the supported
	 * values or if size in bytes specified by param_value_size is
	 * less than size of return type and param_value is not a NULL
	 * value.
	 */
	errNo = clGetKernelInfo(kernel,
	                        CL_DEVICE_NAME,
	                        0,
	                        NULL,
	                        &param_value_size);
	if(!piglit_cl_check_error(errNo, CL_INVALID_VALUE)) {
		fprintf(stderr,
		        "Failed (error code: %s): Trigger CL_INVALID_VALUE if param_name is not one of the supported values.\n",
		        piglit_cl_get_error_name(errNo));
		piglit_merge_result(&result, PIGLIT_FAIL);
	}

	errNo = clGetKernelInfo(kernel,
	                        CL_KERNEL_FUNCTION_NAME,
	                        1,
	                        param_value,
	                        NULL);
	if(!piglit_cl_check_error(errNo, CL_INVALID_VALUE)) {
		fprintf(stderr,
		        "Failed (error code: %s): Trigger CL_INVALID_VALUE if size in bytes specified by param_value is less than size of return type and param_value is not a NULL value.\n",
		        piglit_cl_get_error_name(errNo));
		piglit_merge_result(&result, PIGLIT_FAIL);
	}
	
	/*
	 * CL_INVALID_KERNEL if kernel is not a valid kernel object.
	 */
	errNo = clGetKernelInfo(NULL,
	                        CL_KERNEL_FUNCTION_NAME,
	                        0,
	                        NULL,
	                        &param_value_size);
	if(!piglit_cl_check_error(errNo, CL_INVALID_KERNEL)) {
		fprintf(stderr,
		        "Failed (error code: %s): Trigger CL_INVALID_KERNEL if kernel is not a valid kernel object.\n",
		        piglit_cl_get_error_name(errNo));
		piglit_merge_result(&result, PIGLIT_FAIL);
	}

	clReleaseKernel(kernel);

	return result;
}
