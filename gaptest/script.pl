

run_tests(100, 16, 12, 18);

sub run_tests {
    my ($trials, $k, $graph_start, $graph_end) = @_;
    my @node_list = node_list_gen($trials, $graph_start, $graph_end);
    my $dir = 'temp';
    
    check_and_create_dir($dir);
    gen_graphs($graph_start, $graph_end, $k, $dir);

    print "\n\nBFS With Sync\n";
    run_bfs_for_many_graph($graph_start, $graph_end, $trials, 0, $k, $dir, @node_list);
    print "\n\nBFS Without Sync\n";
    run_bfs_for_many_graph($graph_start, $graph_end, $trials, 1, $k, $dir, @node_list);
    
    check_and_remove_dir($dir);
}

sub run_bfs_for_many_graph {
    my ($graph_size_start, $graph_size_end, $trials, $sync, $k, $dir, @node_list) = @_;

    for(my $i = $graph_size_start; $i <= $graph_size_end; $i++){
        my $graph_file = "$dir/g_$i.sg";
        my $node_list_sub_aref = $node_list[$i - $graph_size_start];
        my $line = run_bfs($graph_file, $trials, $sync, $node_list_sub_aref);
        print "$line\n";
    }
}

sub run_bfs {
    my ($graph_file, $trials, $sync, $node_list_sub_aref) = @_;

    my $add_if_sync = "";
    my @node_list_sub = @{$node_list_sub_aref};
    my $sum = 0;
    if($sync) { $add_if_sync = "_no_syn"; }
    for(my $trial = 0; $trial < $trials; $trial++){
        my $node = $node_list_sub[$trial];
        open(RUN, "\.\/bfs_no_BU$add_if_sync -f $graph_file -n 5 -r $node | grep \"Average Time\" | awk \'\{print \$3\}\'|");
        my @lines = <RUN>;
        my $line = $lines[0];
        chop($line);
        $sum = $sum + $line;
        close RUN;
    }
    $sum = $sum / $trials;
    return $sum;
}

#generates the list of random nodes to read from
sub node_list_gen {
    my ($trials, $graph_start, $graph_end) = @_;

    my @node_list;

    for(my $i = $graph_start; $i <= $graph_end; $i++){
        my $node_trial = node_list_gen_sub($trials, $i);
        push(@node_list, $node_trial);
    }

    return @node_list;
}

#Creates an array with <trials> number of random node values
sub node_list_gen_sub {
    my ($trials, $graph_size) = @_;
    
    my @node_sub_list;
    my $range_end = 2**$graph_size;

    for(my $trial = 1; $trial <= $trials; $trial++){
        my $rand_node = int(rand($range_end));
        push(@node_sub_list, $rand_node);
    }
    
    return \@node_sub_list;
}

#graph gen
sub gen_graphs {
    my ($graph_start, $graph_end, $k, $dir) = @_;

    for(my $i = $graph_start; $i <= $graph_end; $i++){
        gen_one_graph($i, $k, $dir);
    }
}



sub gen_one_graph {
    my ($graph_size, $k, $dir) = @_;
    system("./converter -g $graph_size -k $k -b $dir/g_$graph_size.sg");
}


sub check_and_create_dir {
    my ($dir) = @_;
    if(!-d $dir) {
        mkdir $dir;    
    }
}


sub check_and_remove_dir {
    my ($dir) = @_;
    if(!-d $dir) {
        system("rm -r $dir");    
    }
}

