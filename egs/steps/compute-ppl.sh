#!/bin/bash

set -e
set -o pipefail

source path.sh

if [ $# -lt 3 ]; then
  echo "Usage: $0 <fst> <word_syms> <testset> [disambig_sym]"
  exit 1
fi

fst=$1
word_syms=$2
testset=$3

disambig='#0'
if [ $# -gt 3 ]; then
  disambig=$4
fi

disambig_id=`grep "$disambig" "$word_syms" | awk '{print $2}'`

sym2int.pl --map-oov '<unk>' "$word_syms" < "$testset" \
  | sent-to-fst.py --disambig-symbol-id=$disambig_id \
  | compute-ppl "$fst"
