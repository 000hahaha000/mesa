/*
 * Copyright © 2012 Intel Corporation
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

/** @file getactiveuniformblockname.c
 *
 * From the GL_ARB_uniform_buffer_object spec:
 *
 *     "An active uniform block's name string can be queried from its
 *      uniform block index by calling
 *
 *          void GetActiveUniformBlockName(uint program,
 *                                         uint uniformBlockIndex,
 *                                         sizei bufSize,
 *                                         sizei* length,
 *                                         char* uniformBlockName);
 *
 *      The string name of the uniform block identified by
 *      <uniformBlockIndex> is returned into <uniformBlockName>. The
 *      name is null-terminated. The actual number of characters
 *      written into <uniformBlockName>, excluding the null
 *      terminator, is returned in <length>.  If <length> is NULL, no
 *      length is returned.
 *
 *      <bufSize> contains the maximum number of characters (including
 *      the null terminator) that will be written back to
 *      <uniformBlockName>.
 *
 *      If an error occurs, nothing will be written to
 *      <uniformBlockName> or <length>.
 *
 *      ...
 *
 *      The error INVALID_VALUE is generated by GetActiveUniformsiv,
 *      GetActiveUniformName, GetActiveUniformBlockiv,
 *      GetActiveUniformBlockName, and UniformBlockBinding if
 *      <program> is not a value generated by GL.
 *
 *      The error INVALID_VALUE is generated by GetActiveUniformName
 *      and GetActiveUniformBlockName if <bufSize> is less than zero.
 *
 *      The error INVALID_VALUE is generated by
 *      GetActiveUniformBlockiv, GetActiveUniformBlockName, and
 *      UniformBlockBinding if <uniformBlockIndex> is greater than or
 *      equal to ACTIVE_UNIFORM_BLOCKS."
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;
	config.window_visual = PIGLIT_GL_VISUAL_RGBA | PIGLIT_GL_VISUAL_DOUBLE;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

void
piglit_init(int argc, char **argv)
{
	int i;
	GLuint prog;
	const char *source =
		"#extension GL_ARB_uniform_buffer_object : enable\n"
		"uniform a { float u1; };\n"
		"uniform bbb { float u2; };\n"
		"uniform cc { float u3; };\n"
		"void main() {\n"
		"	gl_FragColor = vec4(u1 + u2 + u3);\n"
		"}\n";
	int blocks;
	bool pass = true;
	const char *names[3] = {"a", "bbb", "cc"};
	bool found[3] = {false, false, false};
	char no_write;
	char fill_char = 0xd0;

	piglit_require_extension("GL_ARB_uniform_buffer_object");

	prog = piglit_build_simple_program(NULL, source);

	glGetProgramiv(prog, GL_ACTIVE_UNIFORM_BLOCKS, &blocks);
	assert(blocks == 3);

	for (i = 0; i < blocks; i++) {
		GLint written_strlen = 0;
		GLint namelen = 9999;
		char name[1000];
		int name_index;

		/* This is the size including null terminator. */
		glGetActiveUniformBlockiv(prog, i, GL_UNIFORM_BLOCK_NAME_LENGTH,
					  &namelen);

		memset(name, 0xd0, sizeof(name));
		glGetActiveUniformBlockName(prog, i, sizeof(name),
					    &written_strlen, name);
		if (written_strlen >= sizeof(name) - 1) {
			fprintf(stderr,
				"return strlen %d, longer than the buffer size\n",
				written_strlen);
			pass = false;
			continue;
		} else if (name[written_strlen] != 0) {
			fprintf(stderr, "return name[%d] was %d, expected 0\n",
				written_strlen, name[written_strlen]);
			pass = false;
			continue;
		} else if (strlen(name) != written_strlen) {
			fprintf(stderr, "return strlen was %d, but \"%s\" "
				"has strlen %d\n", written_strlen, name,
				(int)strlen(name));
			pass = false;
			continue;
		}

		for (name_index = 0; name_index < ARRAY_SIZE(names); name_index++) {
			if (strcmp(names[name_index], name) == 0) {
				if (found[name_index]) {
					fprintf(stderr,
						"Uniform block \"%s\" "
						"returned twice.\n", name);
					pass = false;
				}
				found[name_index] = true;
				break;
			}
		}
		if (name_index == ARRAY_SIZE(names)) {
			fprintf(stderr,
				"block \"%s\" is not a known block name\n", name);
			pass = false;
			continue;
		}

		if (namelen != written_strlen + 1) {
			fprintf(stderr,
				"block \"%s\" had "
				"GL_UNIFORM_BLOCK_NAME_LENGTH %d, expected %d\n",
				name, namelen, written_strlen + 1);
			pass = false;
			continue;
		}

		/* Test for overflow by writing to a bufSize equal to
		 * strlen and checking if a null terminator or
		 * something landed past that.
		 */
		memset(name, fill_char, sizeof(name));
		glGetActiveUniformBlockName(prog, i, written_strlen,
					    NULL, name);
		if (name[written_strlen] != fill_char) {
			fprintf(stderr, "glGetActiveUniformName overflowed: "
				"name[%d] = 0x%02x instead of 0x%02x\n",
				written_strlen, name[written_strlen],
				fill_char);
			pass = false;
		}
	}

	if (!piglit_khr_no_error) {
		no_write = fill_char;
		glGetActiveUniformBlockName(0xd0d0, 0, 1, NULL, &no_write);
		pass = piglit_check_gl_error(GL_INVALID_VALUE) && pass;
		if (no_write != fill_char)
			pass = false;

		no_write = fill_char;
		glGetActiveUniformBlockName(prog, 0, -1, NULL, &no_write);
		pass = piglit_check_gl_error(GL_INVALID_VALUE) && pass;
		if (no_write != fill_char)
			pass = false;

		no_write = fill_char;
		glGetActiveUniformBlockName(prog, blocks, 1, NULL, &no_write);
		pass = piglit_check_gl_error(GL_INVALID_VALUE) && pass;
		if (no_write != fill_char)
			pass = false;
	}

	glDeleteProgram(prog);

	piglit_report_result(pass ? PIGLIT_PASS : PIGLIT_FAIL);
}

enum piglit_result piglit_display(void)
{
	/* UNREACHED */
	return PIGLIT_FAIL;
}
