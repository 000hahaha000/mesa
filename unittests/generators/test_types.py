# encoding=utf-8
# Copyright © 2016, 2019 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""Tests from generated_tests/modules/types.py"""

import itertools

import pytest

# pylint can't figure out the sys.path manipulation.
from generated_tests.modules import types  # pylint: disable=import-error,wrong-import-order


def test_container_is_type_assert():
    """modules.types.Container: Only accept one of is_scalar or
    is_vector or is_matrix"""
    for s, v, m in itertools.product([True, False], repeat=3):
        # Don't test the valid case
        if [s, v, m].count(True) == 0:
            continue

        with pytest.raises(AssertionError):
            types.Container('foo', is_scalar=s, is_vector=v, is_matrix=m,
                            contains=types.FLOAT)


def test_matrix_is_square():
    """modules.types.Matrix.square: works for square matricies"""
    for mat in [types.MAT2, types.DMAT3X3]:
        assert mat.square is True


def test_matrix_is_not_square():
    """modules.types.Matrix.square: works for non-square matricies"""
    assert types.MAT2X4.square is False


def test_type_int_float_assert():
    """modules.types.Type: only integer or floating can be passed."""
    with pytest.raises(AssertionError):
        types.Type('foo', integer=True, floating=True, size=32)


def test_type_float_signed_assert():
    """modules.types.Type: floating types must be signed."""
    with pytest.raises(AssertionError):
        types.Type('foo', floating=True, signed=False, size=32)
