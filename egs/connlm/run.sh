#!/bin/bash

set -e
set -o pipefail

source path.sh

# data/g.txt and data/words.txt are generated from
#connlm-tofst --word-selection-method=majority --max-gram=4 --ws-arg=0.5 --wildcard-ws-arg=0.9 --word-syms-file=exp/rnn/fst/words.txt --output-unk=true exp/rnn/final.clm exp/rnn/fst/g.txt

fstcompile data/g.txt | fstarcsort - data/g.fst

../steps/compute-ppl.sh --disambig-symbol '#phi' \
                        --bos-symbol '<s>' \
                        --eos-symbol '</s>' \
                        data/g.fst data/words.txt data/test.txt

# PPL is 94.8192
