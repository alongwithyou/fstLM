#!/bin/bash

set -e
set -o pipefail

source path.sh

# see connLM/egs/tiny/run.sh 9 for generating data/g.txt.gz and data/words.txt

gunzip -c data/g.txt.gz | fstcompile --keep_state_numbering=true \
                                     --isymbols=data/words.txt \
                                     --osymbols=data/words.txt \
                        | connlmfst-to-arpa data/words.txt data/arpa.header \
                        | gzip -c > data/arpa.gz
