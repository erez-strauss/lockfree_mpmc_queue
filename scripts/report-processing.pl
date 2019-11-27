#!/usr/bin/perl

use strict;
use warnings;

my $infilename = "reports/q-bw-report.20191112-180914.txt";
$infilename = $ARGV[0] if (0 < @ARGV);

sub commify($) { my $x = reverse $_[0]; $x=~s/(\d\d\d)(?=\d)(?!\d*\.)/$1,/g ; return scalar reverse $x; }

sub load_file($) {
	my $infilename = shift;
	my @data;
	my $infile;
	open($infile, "<$infilename") || die("failed to open input file");

	while(<$infile>){
	       chomp;
	       my @fields=split(/\s\s*/);
	       next if !/^Q BW:/;
	       #printf("@fields\n");
	       # Q BW: data size: 8 index size: 8 capacity: 64 producers: 8 consumers: 4 for: 200ms mpmc_queue<tt> push: 609585 pop: 609585 tsc: 443832686 tsc/op: 728 push/pop per sec: 3034125

	       my ($datasz,$indexsz,$capacity,$prod,$cons,$qname,$bw)=(@fields)[4,7,9,11,13,16,28];
	       # printf("$bw $qname $prod $cons @fields\n");
	       my $rec = {
		       # 'zline    ' => $_,
		       'data_sz'    => $datasz,
		       'index_sz'   => $indexsz,
		       'queue_name' => $qname,
		       'capacity'   => $capacity,
		       'producers'  => $prod,
		       'consumers'  => $cons,
		       'bandwidth'  => $bw
	       };
	       @data = ( @data , $rec );
	}
	@data = sort ( { $b->{'bandwidth'} <=> $a->{'bandwidth'} } @data);
	return @data;
}

my @data = load_file($infilename);

sub info_per_data($) {
	
	my $data_size = shift;

	print ("report for data size: $data_size\n");

	my @ddata = grep ( { $_->{data_sz} == $data_size } @data );

	my $mx;
	for my $r ( grep { ($_->{producers} == 1 && $_->{consumers} == 1) } @ddata ) {
		print ("fastest 1-to-1: ");
		print (" $_: ", commify $$r{$_}) for (( # 'producers', 'consumers',
			       	'data_sz', 'index_sz', 'queue_name', 'capacity', 'bandwidth' ));
		print ("\n");
		$mx = $r->{bandwidth};
		last;
	}
	for my $r ( grep { ($_->{producers} == 2 && $_->{consumers} == 2) } @ddata ) {
		print("fastest 2-to-2: ");
		print (" $_: ", commify $$r{$_}) for (( # 'producers', 'consumers',
			       	'data_sz', 'index_sz', 'queue_name', 'capacity', 'bandwidth' ));
		# print ("\n");
		printf(" (%.2f)\n", 100.0* $r->{bandwidth} / $mx);
		# $mx = $r->{bandwidth};
		last;
	}
	for my $r ( grep { ($_->{producers} == 1 && $_->{consumers} == 2) } @ddata ) {
		print("fastest 1-to-2: ");
		print (" $_: ", commify $$r{$_}) for (( # 'producers', 'consumers',
			       	'data_sz', 'index_sz', 'queue_name', 'capacity', 'bandwidth' ));
		# print ("\n");
		printf(" (%.2f)\n", 100.0* $r->{bandwidth} / $mx);
		# $mx = $r->{bandwidth};
		last;
	}
	for my $r ( grep { ($_->{producers} == 2 && $_->{consumers} == 1) } @ddata ) {
		print("fastest 2-to-1: ");
		print (" $_: ", commify $$r{$_}) for (( # 'producers', 'consumers',
			       	'data_sz', 'index_sz', 'queue_name', 'capacity', 'bandwidth' ));
		# print ("\n");
		printf(" (%.2f)\n", 100.0* $r->{bandwidth} / $mx);
		# $mx = $r->{bandwidth};
		last;
	}
	for my $r ( grep { ($_->{producers} == 1 && $_->{consumers} == 1 && $_->{queue_name} =~ /boost/) } @ddata ) {
		print("boostq  1-to-1: ");
		print (" $_: ", commify $$r{$_}) for (( # 'producers', 'consumers',
			       	'data_sz', 'index_sz', 'queue_name', 'capacity', 'bandwidth' ));
		printf(" (%.2f)\n", 100.0* $r->{bandwidth} / $mx);
		last;
	}
	for my $r ( grep { ($_->{producers} == 2 && $_->{consumers} == 2 && $_->{queue_name} =~ /boost/) } @ddata ) {
		print("boostq  2-to-2: ");
		print (" $_: ", commify $$r{$_}) for (( # 'producers', 'consumers',
			       	'data_sz', 'index_sz', 'queue_name', 'capacity', 'bandwidth' ));
		printf(" (%.2f)\n", 100.0* $r->{bandwidth} / $mx);
		last;
	}
}
info_per_data(4);
info_per_data(8);
info_per_data(12);

