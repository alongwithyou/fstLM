/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Wang Jian
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <math.h>
#include <stdlib.h>

#include <fst/fstlib.h>

/**
   This program loads in a fstLM in connLMFST format, and converts
   it into ARPA format.

   This is simply the C++ version of connLM/egs/utils/fst/fst2arpa.py, since
   the Python version eats up too much memory for large FST.

*/

DEFINE_int32(debug, 0, "debug level");
DEFINE_string(bos, "<s>", "BOS symbol");
DEFINE_string(phi, "#phi", "Bow symbol");
DEFINE_int32(bos_id, 2, "BOS state id");
DEFINE_int32(wildcard_id, 3, "Wildcard state id");

namespace fstlm {
using namespace fst;

class ARPAConverter {
  typedef StdArc::StateId StateId;
  typedef StdArc::Weight Weight;

 public:
  ARPAConverter(int argc, char **argv):
      order_(0.0) {
    assert(argc >= 3);
    std::cerr << "connlmfst-to-arpa: Loading FST...\n";
    lm_ = VectorFst<StdArc>::Read(std::cin, FstReadOptions("standard input"));
    if (!lm_) {
      std::cerr << "connlmfst-to-arpa: Reading LM FST error from stdin\n";
      exit(1);
    }
    ArcSort(lm_, ILabelCompare<StdArc>());

    fst::SymbolTableTextOptions opts;
    word_syms_ = SymbolTable::ReadText(argv[1], opts);
    if (!word_syms_) exit(1);

    OutputNgrams();
    OutputHeader(argv[2]);
  }

  ~ARPAConverter() {
    delete lm_;
    delete word_syms_;
  }
 private:

  string Weight2Str(float w) {
    static const float LN_10 = log(10);
    static const string ARPA_INF = string("-99");
    if (isinf(w)) {
      return ARPA_INF;
    } else {
      std::ostringstream oss;
      oss << (-w / LN_10);
      return oss.str();
    }
  }

  bool IsBackoffArc(const StdArc &arc) {
    return (arc.ilabel == word_syms_->Find(FLAGS_phi));
  }

  float GetBow(StateId st) {
    size_t n = lm_->NumArcs(st);
    assert (n > 0);
    ArcIterator<StdFst> aiter(*lm_, st);
    aiter.Seek(n - 1);
    const StdArc &arc = aiter.Value();
    assert(IsBackoffArc(arc));
    return arc.weight.Value();
  }

  string Ngram(vector<int32> syms) {
    std::ostringstream oss;
    for(size_t i = 0; i < syms.size(); ++i) {
      if(i != 0) {
        oss << " ";
      }
      oss << word_syms_->Find(syms[i]);
    }

    return oss.str();
  }

    void OutputNgrams() {
      vector<vector<int32>> ssyms(lm_->NumStates());
      ssyms[FLAGS_bos_id].push_back(word_syms_->Find(FLAGS_bos));

      StateId bos_rng[2] = {-1, -1};
      StateId wildcard_rng[2] = {FLAGS_wildcard_id, FLAGS_wildcard_id + 1};

      while (bos_rng[0] < bos_rng[1] || wildcard_rng[0] < wildcard_rng[1]) {
        ++order_;
        std::cerr << "connlmfst-to-arpa: Printing " << order_ << "-gram...\n";
        int64 cnt = 0;
        std::cout << "\n\\" << order_ << "-grams:\n";
        if (order_ == 1) {
          std::cout << Weight2Str(INFINITY) << "\t" << FLAGS_bos << "\t"
            << Weight2Str(GetBow(FLAGS_bos_id)) << "\n";
          ++cnt;
        }

        StateId rng[2] = {bos_rng[0], bos_rng[1]};
        StateId end = bos_rng[1] - 1;
        bos_rng[0] = INT_MAX;
        bos_rng[1] = -1;
        for (StateId st = rng[0]; st < rng[1]; st++) {
          for (ArcIterator<StdFst> aiter(*lm_, st); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            if (IsBackoffArc(arc)) {
              continue;
            }
            vector<int32> syms;
            syms.reserve(ssyms[st].size() + 1);
            syms.insert(syms.begin(), ssyms[st].begin(), ssyms[st].end());
            syms.push_back(arc.ilabel);

            if (arc.nextstate > end) { // non-highest order and not </s>
              if (arc.nextstate < bos_rng[0]) {
                bos_rng[0] = arc.nextstate;
              }
              if (arc.nextstate + 1> bos_rng[1]) {
                bos_rng[1] = arc.nextstate + 1;
              }

              std::cout << Weight2Str(arc.weight.Value()) << "\t"
                << Ngram(syms) << "\t"
                << Weight2Str(GetBow(arc.nextstate)) << "\n";

              ssyms[arc.nextstate].swap(syms);
            } else {
              std::cout << Weight2Str(arc.weight.Value()) << "\t"
                << Ngram(syms) << "\n";
            }
            ++cnt;
          }
        }

        rng[0] = wildcard_rng[0];
        rng[1] = wildcard_rng[1];
        end = wildcard_rng[1] - 1;
        wildcard_rng[0] = INT_MAX;
        wildcard_rng[1] = -1;
        for (StateId st = rng[0]; st < rng[1]; st++) {
          for (ArcIterator<StdFst> aiter(*lm_, st); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            if (IsBackoffArc(arc)) {
              continue;
            }
            vector<int32> syms;
            syms.reserve(ssyms[st].size() + 1);
            syms.insert(syms.end(), ssyms[st].begin(), ssyms[st].end());
            syms.push_back(arc.ilabel);

            if (arc.nextstate > end) { // non-highest order and not </s>
              if (arc.nextstate < wildcard_rng[0]) {
                wildcard_rng[0] = arc.nextstate;
              }
              if (arc.nextstate + 1> wildcard_rng[1]) {
                wildcard_rng[1] = arc.nextstate + 1;
              }

              std::cout << Weight2Str(arc.weight.Value()) << "\t"
                << Ngram(syms) << "\t"
                << Weight2Str(GetBow(arc.nextstate)) << "\n";

              ssyms[arc.nextstate].swap(syms);
            } else {
              std::cout << Weight2Str(arc.weight.Value()) << "\t"
                << Ngram(syms) << "\n";
            }
            ++cnt;
          }
        }

        ngram_counts_.push_back(cnt);
        std::cerr << "connlmfst-to-arpa: " << cnt << " Done.\n";

        if (order_ == 1) {
          bos_rng[0] = FLAGS_bos_id;
          bos_rng[1] = FLAGS_bos_id + 1;
        }
      }

      std::cout << "\n\\end\\\n";
    }

    void OutputHeader(const char* header_fname) {
      std::ofstream header_file;

      header_file.open(header_fname);

      header_file << "\\data\\" << "\n";
      for (int32 o = 0; o < order_; o++) {
        header_file << "ngram " << (o + 1) << "=" << ngram_counts_[o]<< "\n";
      }
      header_file.close();
    }

    // lm_ is the fst for lm.
    VectorFst<StdArc> *lm_;
    SymbolTable *word_syms_;

    int32 order_;
    vector<int64> ngram_counts_;
  };

}  // namespace fstlm

int main (int argc, char **argv) {
  const char *usage = "Usage: connlmfst-to-ppl <words_syms> <header_name> < <connlm_fst> > <arpa>\n"
              "This program loads in a fstLM in connLMFST format from stdin,"
              "and converts it into ARPA format to stdout.\n";
  SET_FLAGS(usage, &argc, &argv, true);

  if (argc < 3) {
    ShowUsage();
    exit(1);
  }

  // everything happens in the constructor of class ARPAConverter.
  fstlm::ARPAConverter(argc, argv);

  return 0;
}
