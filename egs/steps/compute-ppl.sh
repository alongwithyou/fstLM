#!/bin/bash

set -e
set -o pipefail

# Begin configuration section.
disambig_symbol='#0'
bos_symbol=''
eos_symbol=''
debug=0
# end configuration section.

echo "$0 $@"  # Print the command line for logging

source path.sh

. parse_options.sh

if [ $# -ne 3 ]; then
  echo "Usage: $0 <fst> <word_syms> <testset>"
  echo "Main options:"
  echo "  --disambig-symbol <symbol>     # default '#0'."
  echo "  --bos-symbol <symbol>          # default ''. Will generate bos arc for sentence FST, if specified."
  echo "  --eos-symbol <symbol>          # default ''. Will generate eos arc for sentence FST, if specified."
  echo "  --debug <debug>                # default 0. debug level for compute-ppl."
  exit 1
fi

fst=$1
word_syms=$2
testset=$3

disambig_id=`grep "$disambig_symbol" "$word_syms" | awk '{print $2}'`
bos_symbol_id=-1
if [ -n "$bos_symbol" ]; then
  bos_symbol_id=`grep "$bos_symbol" "$word_syms" | awk '{print $2}'`
fi
eos_symbol_id=-1
if [ -n "$eos_symbol" ]; then
  eos_symbol_id=`grep "$eos_symbol" "$word_syms" | awk '{print $2}'`
fi

sym2int.pl --map-oov '<unk>' "$word_syms" < "$testset" \
  | sent-to-fst.py --disambig-symbol-id=$disambig_id \
                   --bos-symbol-id=$bos_symbol_id \
                   --eos-symbol-id=$eos_symbol_id \
  | compute-ppl --debug=$debug "$fst"
