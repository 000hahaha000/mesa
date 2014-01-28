#!/usr/bin/env python

# Copyright (c) 2013 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.


import os
import os.path as path
import unittest
import itertools
import tempfile

try:
    import simplejson as json
except ImportError:
    import json
import framework.summary as summary
from helpers import test_iterations, create_testresult, create_test


""" Status ordering from best to worst:

See ../summary.py.

"""

# These lists normally would reside within a class, however, these are meant to
# be fed to decorators, which donot have access to class variables.

# List of possible statuses.
# 'notrun' must be last or all kinds of things might break.
STATUSES = ['pass', 'fail', 'skip', 'warn', 'crash', 'dmesg-warn',
            'dmesg-fail', 'notrun']

# list of possible regressions
REGRESSIONS = [("pass", "warn"),
               ("pass", "dmesg-warn"),
               ("pass", "fail"),
               ("pass", "dmesg-fail"),
               ("pass", "crash"),
               ("dmesg-warn", "warn"),
               ("dmesg-warn", "dmesg-fail"),
               ("dmesg-warn", "fail"),
               ("dmesg-warn", "crash"),
               ("warn", "fail"),
               ("warn", "crash"),
               ("warn", "dmesg-fail"),
               ("dmesg-fail", "crash"),
               ("dmesg-fail", "fail"),
               ("fail", "crash"),
               ("skip", "crash"),
               ("skip", "fail"),
               ("skip", "dmesg-fail"),
               ("skip", "warn"),
               ("skip", "dmesg-warn"),
               ("notrun", "crash"),
               ("notrun", "fail"),
               ("notrun", "dmesg-fail"),
               ("notrun", "warn"),
               ("notrun", "dmesg-warn")]


# List of possible fixes
FIXES = [("crash", "fail"),
         ("crash", "dmesg-fail"),
         ("crash", "warn"),
         ("crash", "dmesg-warn"),
         ("crash", "pass"),
         ("crash", "skip"),
         ("crash", "notrun"),
         ("fail", "dmesg-fail"),
         ("fail", "warn"),
         ("fail", "dmesg-warn"),
         ("fail", "pass"),
         ("fail", "skip"),
         ("fail", "notrun"),
         ("dmesg-fail", "warn"),
         ("dmesg-fail", "dmesg-warn"),
         ("dmesg-fail", "pass"),
         ("dmesg-fail", "skip"),
         ("dmesg-fail", "notrun"),
         ("warn", "dmesg-warn"),
         ("warn", "pass"),
         ("warn", "skip"),
         ("warn", "notrun"),
         ("dmesg-warn", "pass"),
         ("dmesg-warn", "skip"),
         ("dmesg-warn", "notrun")]


# List of statuses that should be problems.
PROBLEMS = ["crash", "fail", "warn", "dmesg-warn", "dmesg-fail"]


class SummaryTestBase(unittest.TestCase):
    """ Abstract base class for testing the Summary module"""
    tmpdir = tempfile.mkdtemp("piglit")
    files = []

    @classmethod
    def tearDownClass(cls):
        """ Remove any folders which were created and are empty """
        for branch in cls.files:
            os.remove(branch)  # Delete the fie itself
            while branch != '/':
                branch = path.dirname(branch)
                try:
                    os.rmdir(branch)
                except OSError as exception:
                    # If there is an OSError the directory is not empty, so
                    # every additional attempt to call os.rmdir will fail.
                    if exception.errno == 39:
                        break

    @classmethod
    def _makedirs(cls, dirs):
        """ Create one more more folders, with some error checking
        
        This catches a "Directory Exists" error (OSError.errno == 17), and
        passes that; but any other error will be raised

        """
        try:
            os.makedirs(dirs)
        except OSError as exception:
            # os.mkdir (and by extension os.makedirs) quite annoyingly throws
            # an error.

            # if the directory exists if the error was not a "Directory Exists"
            # Error, go ahead and raise it.
            if exception.errno != 17:
                raise exception

            # you cannot pass in an if clause, since that would pass the if,
            # not the except
            pass

    def _generate_summary(self, files):
        return summary.Summary([path.join(self.tmpdir, i) for i in files])


class StatusTest(SummaryTestBase):
    """ Test status comparisons in the Summary module.

    This test creates summary objects with one or two results that it
    generates. These results have a single test in them forming known status
    pairs. These pairs are explicitly a fix, regression, or change. The code
    then creates a summary and ensures that the status has been added to the
    proper field, and not to any additional fields.

    """

    @classmethod
    def setUpClass(cls):
        """ Create all of the necissary files for running the test

        Create a set of temprorary result files, and add them to the files
        attribute. This attribute will be deleted by the tearDownClass() method
        after the tests have run

        """
        cls._makedirs(cls.tmpdir)

        # Create temporary files in the /tmp/piglit directory
        for result in STATUSES[:-1]:  # notrun cannot be generated here
            tmpfile = path.join(cls.tmpdir, result, "main")
            cls._makedirs(path.dirname(tmpfile))

            with open(tmpfile, 'w') as f:
                json.dump(create_testresult(name=result,
                                            tests=create_test("test", result)),
                          f, indent=4)

            # Add the file to the list of files to be removed in tearDownClass
            cls.files.append(tmpfile)

        # Create an additional file to test a Not Run status. This needs to be
        # generated separately since the test result should not include the
        # test that will have the not run status.
        tmpfile = path.join(cls.tmpdir, "notrun", "main")
        cls._makedirs(path.dirname(tmpfile))

        with open(tmpfile, 'w') as f:
            # If there are not tests buildDictionary fails
            json.dump(create_testresult(name="notrun", 
                                        tests=create_test("diff", "pass")),
                      f, indent=4)

        cls.files.append(tmpfile)

    @test_iterations(*REGRESSIONS)
    def test_is_regression(self, x, y):
        """ Only explicitly defined regressions should be treated as such

        Test that all combinations that are not explicitly a regression are not
        treated as regressions

        """
        result = self._generate_summary([x, y])
        self.assertEqual(
            len(result.tests['regressions']), 1,
            "{0} -> {1} is not a regression but should be".format(x, y))

    @test_iterations(*FIXES)
    def test_is_fix(self, x, y):
        """ Statuses that are explicitly defined as fixes should be fixes

        Test that ensures that all tests in the FIXES list are treated as fixes

        """
        result = self._generate_summary([x, y])
        self.assertEqual(
            len(result.tests['fixes']), 1,
            "{0} -> {1} is not a fix but should be".format(x, y))

    #XXX: is "notrun" -> not(notrun) a change?
    @test_iterations(*[i for i in itertools.product(STATUSES[:-1], STATUSES[:-1])
                       if i[0] != i[1]])
    def test_is_change(self, x, y):
        """ Check that all changes are treated as changes

        Given two statuses if status1 != status2 then that should be a change

        """
        result = self._generate_summary([x, y])
        self.assertEqual(
            len(result.tests['changes']), 1,
            "{0} -> {1} is a not a change but should be".format(x, y))

    @test_iterations(*PROBLEMS)
    def test_is_problem(self, x):
        """ Only statuses in the PROBLEMS list should be added to problems """
        result = self._generate_summary([x])
        self.assertEqual(
            len(result.tests['problems']), 1,
            "{0} is not a problem but should be".format(x))

    @test_iterations("skip")
    def test_is_skip(self, x):
        """ Ensure that skip is being added to the skip list """
        result = self._generate_summary([x])
        self.assertEqual(
            len(result.tests['skipped']), 1,
            "Skip is not being added to the list of skips")
