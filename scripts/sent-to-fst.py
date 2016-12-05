#!/usr/bin/env python

# we're using python 3.x style print but want it to work in python 2.x,
from __future__ import print_function
import argparse
import sys
import subprocess

parser = argparse.ArgumentParser(description="This script converts a set of "
                                 "sentences into a set of FSTs. Each line in "
                                 "input represents a sentence and one FST "
                                 "will be genrated for each sentence. "
                                 "The input lines should be converted into "
                                 "integer format, typically through sym2int.pl"
                                 "Example usage: "
                                 "  sent-to-fst.py --disambig-symbol-id=1 < data/dev.int > data/dev.fsts",
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)

parser.add_argument("--disambig-symbol-id", type=int, default=None,
                    help="id for disambig symbol. If specified, generate "
                         "the FST state for disambig symbol.")
parser.add_argument("--bos-symbol-id", type=int, default=None,
                    help="id for bos symbol. If specified, generate the FST "
                         "state for BOS.")
parser.add_argument("--eos-symbol-id", type=int, default=None,
                    help="id for eos symbol. If specified, generate the FST "
                         "state for EOS.")
parser.add_argument("--compile-fst", type=str, choices=['true', 'false'],
                    default='false',
                    help="whether compile output fst to openfst format.")

# echo command line to stderr for logging.
print(' '.join(sys.argv), file=sys.stderr)

args = parser.parse_args()

if args.disambig_symbol_id is not None and args.disambig_symbol_id <= 0:
  print("--disambig-symbol-id must be a positive integer.", file=sys.stderr)
  sys.exit(1)

if args.bos_symbol_id is not None and args.bos_symbol_id <= 0:
  print("--bos-symbol-id must be a positive integer.", file=sys.stderr)
  sys.exit(1)

if args.eos_symbol_id is not None and args.eos_symbol_id <= 0:
  print("--eos-symbol-id must be a positive integer.", file=sys.stderr)
  sys.exit(1)

def build_fst_txt(words):
  words = map(int, line.split())
  num_words = len(words) + 1 # for EOS
  if args.disambig_symbol_id is not None:
    assert args.disambig_symbol_id not in words

  if args.bos_symbol_id is not None:
    words = [args.bos_symbol_id] + words
  if args.eos_symbol_id is not None:
    words = words + [args.eos_symbol_id]

  fst_txt = ''
  n_states = 0
  for word in words:
    fst_txt += "%d %d %d %d\n" % (n_states, n_states + 1, word, word)
    if args.disambig_symbol_id is not None:
      fst_txt += "%d %d %d %d\n" % (n_states, n_states,
          args.disambig_symbol_id, args.disambig_symbol_id)
    n_states += 1
  if args.disambig_symbol_id is not None:
    fst_txt += "%d %d %d %d\n" % (n_states, n_states,
        args.disambig_symbol_id, args.disambig_symbol_id)
  fst_txt += "%d\n" % n_states
  return fst_txt, num_words

n_fsts = 0
for line in sys.stdin:
  if line.strip() == '':
    continue

  fst_txt, num_words = build_fst_txt(line)

  if args.compile_fst == 'true':
    command = ['fstcompile']
    p = subprocess.Popen(command, stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    print("b%d" % num_words)
    print(p.communicate(input=fst_txt)[0], end='')
  else:
    print("t%d" % num_words)
    print(fst_txt, end='')
    print("</FST>")

  n_fsts += 1

print("Success to convert %d FSTs." % n_fsts, file=sys.stderr)
