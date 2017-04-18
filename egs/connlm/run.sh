#!/bin/bash

set -e
set -o pipefail

source path.sh

# see connLM/egs/tiny/run.sh 9 for generating data/g.txt.gz and data/words.txt

gunzip -c data/g.txt.gz | fstcompile | fstarcsort - data/g.fst

../steps/compute-ppl.sh --disambig-symbol '#phi' \
                        --bos-symbol '<s>' \
                        --eos-symbol '</s>' \
                        data/g.fst data/words.txt data/test.txt

# PPL is 107.093
