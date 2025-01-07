#!/bin/bash
# Run chess engine on all FENs

arg1=$1
shift

# Decide what computer to run on
#if [ $arg1 = "-h" ]; then
#    host=$1
#    shift
#    if [ $host != "localhost" ]; then
#        cmd="ssh -x $host $0 -h localhost"
#        for i in "$@" ; do
#            cmd="$cmd \"$i\""
#        done
#        exec $cmd
#    fi
#else
#    if [ $arg1 -lt 14 ]; then
#        host="zen4"
#    elif [ $arg1 -lt 22 ]; then
#        host="fractal"
#    elif [ $arg1 -lt 46 ]; then
#        host="c24"
#    elif [ $arg1 -lt 62 ]; then
#        host="x50"
#    else
#        echo "no host"
#        exit 0
#    fi
#    exec $0 -h $host "$@"
#fi

engine=./texel
nodes=100000

(
    echo uci
    echo setoption name Hash value 512
    echo isready

    for i in "$@" ; do
        echo position fen $i
        echo go nodes $nodes
    done

    echo quit
) | ./syncengine $engine | egrep '^bestmove |^info.* score ' | egrep -v 'lowerbound|upperbound' |
awk '
BEGIN {
    score = "x";
}
$1 == "info" {
    for (i = 1; i < NF-2; i++) {
        if ($i == "score") {
            if ($(i+1) == "cp") {
                score = $(i+2);
            } else if ($(i+1) == "mate") {
                if ($(i+2) > 0) {
                    score = 10000;
                } else {
                    score = -10000;
                }
            }
        }
    }
}
$1 == "bestmove" {
    if ($2 == "0000" || $2 == "(none)") {
        print -10000;
    } else {
        print score;
    }
    score = "x";
}'
