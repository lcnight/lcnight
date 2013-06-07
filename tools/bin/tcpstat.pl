#!/usr/bin/perl

# display tcp statistics at a regular interval
#
# Copyright (c) 2006 dean gaudet <dean@arctic.org>
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

use strict;
use warnings;

select(STDOUT); $| = 1;     # make unbuffered

my $delay;
if ($#ARGV == 0) {
    $delay = int($ARGV[0]);
    if ($delay < 1) {
        $delay = 1;
    }
}
else {
    $delay = 10;
}

# read the /proc/net/snmp headers
open(PROC, "</proc/net/snmp") || die "unable to open /proc/net/snmp for reading: $!\n";
my @tcp_values;
my %tcp_values;
while (<PROC>) {
    chomp;
    if (/^Tcp: /) {
        (undef, @tcp_values) = split;
        %tcp_values = map { $_ => 1 } @tcp_values;
        last;
    }
}

#my @display_values = qw/ActiveOpens PassiveOpens AttemptFails EstabResets CurrEstab InSegs OutSegs RetransSegs InErrs OutRsts/;
my @display_values = qw/ActiveOpens PassiveOpens InSegs InErrs OutSegs RetransSegs/;
my $width = 0;
foreach my $i (@display_values) {
    defined($tcp_values{$i}) or die "missing required tcp value $i in /proc/net/snmp\n";
}

my %prev_stats = map { $_ => 0 } @tcp_values;

foreach my $i (@display_values) {
    printf "%11s ", $i;
}
printf "\n";

my $suppress_line = 1;

while (1) {
    open(PROC, "</proc/net/snmp") || die "unable to open /proc/net/snmp for reading: $!\n";

    my %cur_stats;
    my $seen_header = 0;
    while (<PROC>) {
        chomp;
        if (!$seen_header && /^Tcp:/) {
            $seen_header = 1;
        }
        elsif (/^Tcp:/) {
            my (undef, @cur_values) = split;
            foreach my $j (0..$#tcp_values) {
                $cur_stats{$tcp_values[$j]} = $cur_values[$j];
            }
            last;
        }
    }
    close(PROC);

    unless ($suppress_line) {
        foreach my $i (@display_values) {
            printf "%11s ", $cur_stats{$i} - $prev_stats{$i};
        }
        print "\n";
    }
    $suppress_line = 0;
    
    %prev_stats = %cur_stats;

    sleep($delay);
}
