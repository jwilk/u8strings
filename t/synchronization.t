#!/usr/bin/env perl

# Copyright © 2014-2018 Jakub Wilk <jwilk@jwilk.net>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the “Software”), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

no lib '.';

use strict;
use warnings;
use v5.10;

use FindBin ();
use Test::More tests => 6;

use IPC::Run ();

my $cmd = $ENV{U8STRINGS} // "$FindBin::Bin/../u8strings";

my ($stdout, $stderr);
my $stdin = "\xC2foobar\0";
my $cli = IPC::Run::start(
    [$cmd],
    '<', \$stdin,
    '>', \$stdout,
    '2>', \$stderr,
);
$cli->finish;
cmp_ok($cli->result, '==', 0, 'no error');
cmp_ok($stderr, 'eq', '', 'empty stderr');
cmp_ok($stdout, 'eq', "foobar\n", 'ASCII synchronization');

$stdin = "\xc2żółw\0";
$cli = IPC::Run::start(
    [$cmd],
    '<', \$stdin,
    '>', \$stdout,
    '2>', \$stderr,
);
$cli->finish;
cmp_ok($cli->result, '==', 0, 'no error');
cmp_ok($stderr, 'eq', '', 'empty stderr');
cmp_ok($stdout, 'eq', "żółw\n", 'non-ASCII synchronization');

# vim:ts=4 sts=4 sw=4 et
