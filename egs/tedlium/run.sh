#!/bin/bash

set -e
set -o pipefail

source path.sh

num_dev_sentences=15000
# num_cantab_TEDLIUM_sentences=250000
vocab_size=100000
disambig='#0'

if [ ! -d cantab-TEDLIUM ]; then
    echo "Downloading \"http://cantabresearch.com/cantab-TEDLIUM.tar.bz2\". "
    wget --no-verbose --output-document=- http://cantabresearch.com/cantab-TEDLIUM.tar.bz2 | bzcat | tar --extract --file=- || exit 1
    gzip cantab-TEDLIUM/cantab-TEDLIUM-pruned.lm3
fi
lm=cantab-TEDLIUM/cantab-TEDLIUM-pruned.lm3.gz

if [ ! -d data ]; then
  mkdir data
fi

head -n $num_dev_sentences < cantab-TEDLIUM/cantab-TEDLIUM.txt \
  | sed 's/ <\/s>//g'  > data/dev.txt

gunzip -c $lm | arpa2fst --disambig-symbol=$disambig \
                         --write-symbol-table=data/words.txt - data/G.fst
disambig_id=`grep "$disambig" data/words.txt | awk '{print $2}'`

sym2int.pl --map-oov '<unk>' data/words.txt < data/dev.txt \
  | sent-to-fst.py --disambig-symbol-id=$disambig_id \
  | compute-ppl data/G.fst

# With 15000 dev sentences
# srilm perplexity:
# ngram -lm cantab-TEDLIUM/cantab-TEDLIUM-pruned.lm3.gz -ppl data/dev.txt
# ppl= 132.944 ppl1= 172.832

# fstlm perplexity:
# ppl=131.065
# it is slightly better than srilm, since sometimes there could be a
# better path through backoff arc.
