#!/usr/bin/env perl
use strict;
use warnings;

die "Usage: $0 <input> <varname>\n" unless @ARGV == 2;
my ($input, $varname) = @ARGV;

open(my $fh, "<:raw", $input) or die "open $input: $!";
my $data;
{ local $/; $data = <$fh>; }
close($fh);
my $len = length($data);

my $output = "$input.h";
open(my $out, ">", $output) or die "write $output: $!";
print $out "/* Auto-generated, do not edit */\n";
print $out "static const char ${varname}[$len] = {";
for my $i (0..$len-1) {
    if ($i % 12 == 0) {
        print $out "\n  " if $i > 0;
    } else {
        print $out " " if $i > 0;
    }
    printf $out "0x%02x", ord(substr($data, $i, 1));
    print $out "," if $i < $len - 1;
}
print $out "\n};\n";
print $out "static const unsigned int ${varname}_len = $len;\n";
close($out);
print "Generated $output ($len bytes)\n";
