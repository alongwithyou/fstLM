#!/bin/bash

set -e
set -o pipefail

source path.sh

# data/g.txt and data/words.txt are generated from
#connlm-tofst --boost=0.005 --boost-power=0.5 --word-syms-file=exp/rnn/words.txt --output-unk=true exp/rnn/final.clm exp/rnn/g.txt

fstcompile data/g.txt | fstarcsort - data/g.fst

../steps/compute-ppl.sh --disambig-symbol '#phi' \
                        --bos-symbol '<s>' \
                        --eos-symbol '</s>' \
                        data/g.fst data/words.txt data/test.txt
