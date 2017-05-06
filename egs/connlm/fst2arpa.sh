#!/bin/bash

set -e
set -o pipefail

source path.sh

# see connLM/egs/tiny/run.sh 9 for generating data/g.txt.gz and data/words.txt
if [ ! -d data ]; then
  fst_dir=../../../../connLM/egs/tiny/exp/rnn+maxent/fst
  test_file=../../../../connLM/egs/tiny/data/test
  mkdir -p data
  ln -sf $test_file data/test.txt
  ln -sf $fst_dir/g.txt.gz data/g.txt.gz
  ln -sf $fst_dir/words.txt data/words.txt
fi

gunzip -c data/g.txt.gz | fstcompile --keep_state_numbering=true \
                                     --isymbols=data/words.txt \
                                     --osymbols=data/words.txt \
                        | connlmfst-to-arpa data/words.txt data/arpa.header \
                        | gzip -c > data/arpa.ngrams.gz

cat <(gzip -c data/arpa.header) data/arpa.ngrams.gz > data/arpa.gz

ngram -unk -order 4 -lm data/arpa.gz -ppl data/test.txt
