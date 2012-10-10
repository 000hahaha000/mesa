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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * @file blit-mismatched-samples.cpp
 *
 * This test verifies if glBlitFramebuffer() throws GL_INVALID_OPERATION with
 * non-matching samples in multisample framebuffer objects.
 *
 * We initialize two FBOs with different sample counts, do blitting operation
 * and then query the gl error.
 *
 * Author: Anuj Phogat <anuj.phogat@gmail.com>
 */

#include "common.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;

	config.window_width = 256;
	config.window_height = 256;
	config.window_visual = PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_RGBA;

PIGLIT_GL_TEST_CONFIG_END

const int pattern_width = 256; const int pattern_height = 256;
Fbo src_fbo, dst_fbo;

enum piglit_result
piglit_display()
{
	bool pass = true;

	/* Blit multisample-to-multisample with non-matching sample count */
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo.handle);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo.handle);
	glBlitFramebuffer(0, 0, pattern_width, pattern_height,
			  0, 0, pattern_width, pattern_height,
			  GL_COLOR_BUFFER_BIT, GL_NEAREST);

	/* Here GL_INVALID_OPERATION is an expected gl error */
	pass = piglit_check_gl_error(GL_INVALID_OPERATION) && pass;

	return (pass ? PIGLIT_PASS : PIGLIT_FAIL);
}

void
piglit_init(int argc, char **argv)
{
	GLint samples, max_samples;
	piglit_require_gl_version(30);

	/* OpenGL driver is supposed to round up the specified sample count to
	 * the next available sample count. So, this will create the FBO with
	 * minimum supported sample count.
	 */
	src_fbo.setup(FboConfig(1 /* sample count */,
				pattern_width,
				pattern_height));

	glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, src_fbo.handle);
	glGetIntegerv(GL_SAMPLES, &samples);

	/* Skip the test if samples = GL_MAX_SAMPLES */
	if (samples == max_samples) {
		printf("OpenGL driver seems to support only one possible"
		       " sample count\n");
		piglit_report_result(PIGLIT_SKIP);
	}

	/* Creating FBO with (samples + 1) ensures that we get a different
	 * value of sample count in dst_fbo.
	 */
	dst_fbo.setup(FboConfig(samples + 1, pattern_width, pattern_height));

	if (!piglit_check_gl_error(GL_NO_ERROR)) {
		printf("Error setting up frame buffer objects\n");
		piglit_report_result(PIGLIT_FAIL);
	}
}
