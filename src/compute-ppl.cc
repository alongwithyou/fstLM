/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Wang Jian
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
#include <vector>
#include <map>
#include <math.h>
#include <stdlib.h>

#include <fst/fstlib.h>


/**
   This program loads in a fstLM and reads the sentences FSTs from stdin,
   then computes the perplexity for sentence FSTs and prints it to the stdout.

*/

namespace fstlm {
using namespace fst;

class PPLComputer {
 public:
  PPLComputer(int argc,
               const char **argv):
      total_log_prob_(0.0),
      total_count_(0), cur_sent_id_(0) {

    assert(argc >= 2);
    lm_ = VectorFst<StdArc>::Read(argv[1]);
    if (!lm_) {
      std::cerr << "compute-ppl: Reading LM FST error from" << argv[1] << '\n';
      exit(1);
    }

    ProcessInput();
    ProduceOutput();
  }

  ~PPLComputer() {
    delete lm_;
  }
 private:

  void ProcessInput() {
    while (std::cin.peek(), !std::cin.eof()) {
      int32 num_words;
      std::cin >> num_words;

      VectorFst<StdArc> *fst = ReadNextFst();
      ProcessCurrentFst(*fst);
      delete fst;

      cur_sent_id_++;
      total_count_ += num_words;
    }
  }

  // this function reads next fst from stdin
  VectorFst<StdArc>* ReadNextFst() {
    fst::FstHeader hdr;
    if (!hdr.Read(std::cin, "stdin")) {
      std::cerr << "compute-ppl: Sent " << cur_sent_id_
          << ": Error reading sentence FST header\n";
      exit(1);
    }
    FstReadOptions ropts("<unspecified>", &hdr);
    VectorFst<StdArc> *fst = VectorFst<StdArc>::Read(std::cin, ropts);
    if (!fst) {
      std::cerr << "compute-ppl: Sent " << cur_sent_id_
          << ": Could not read sentenct fst\n";
      exit(1);
    }
    return fst;
  }

  void ProcessCurrentFst(VectorFst<StdArc> &fst) {
      VectorFst<StdArc> comp_fst;
      VectorFst<StdArc> ofst;

      Compose(*lm_, fst, &comp_fst);

      ShortestPath(comp_fst, &ofst);

      double log_prob = GetProb(ofst);

      total_log_prob_ += log_prob;

#ifdef _FSTLM_DEBUG_
      std::ostringstream oss;
      oss << cur_sent_id_ << ".comp.fst";
      comp_fst.Write(oss.str());
      oss.str("");
      oss << cur_sent_id_ << ".final.fst";
      ofst.Write(oss.str());
#endif
  }

  double GetProb(VectorFst<StdArc> &fst) {
    typedef StdArc::StateId StateId;
    typedef StdArc::Weight Weight;

    StateId initial_state = fst.Start();
    if (initial_state == kNoStateId) {
      std::cerr << "compute-ppl: Sent " << cur_sent_id_
          << ": empty FST after composition\n";
      return 0.0;
    }

    Weight weight = Weight::One();

    for (StateIterator<StdFst> siter(fst); !siter.Done(); siter.Next()) {
      StateId st = siter.Value();
      for (ArcIterator<StdFst> aiter(fst, st); !aiter.Done(); aiter.Next()) {
        const StdArc &arc = aiter.Value();
        weight = Times(weight, arc.weight);
      }
      if (fst.Final(st) != Weight::Zero()) {
          weight = Times(weight, fst.Final(st));
      }
    }

    return weight.Value();
  }

  void ProduceOutput() {
    double ppl = exp(total_log_prob_ / total_count_);
    std::cout << std::setprecision(10) << ppl << "\n";
    std::cerr << "compute-ppl: average log-prob per word was "
              << (total_log_prob_ / total_count_)
              << " (perplexity = " << ppl << ") over "
              << total_count_ << " words.\n";
  }

  // lm_ is the fst for lm.
  VectorFst<StdArc> *lm_;

  double total_log_prob_;
  int64 total_count_;
  int64 cur_sent_id_;
};

}  // namespace fstlm

int main (int argc, const char **argv) {
  if (argc < 2) {
    std::cerr << "usage:\n"
              << "compute-ppl <lm_fst> < <sent_fsts>\n"
              << "This program reads in a fst LM and compute the ppl for "
              << "sentence FSTs from stdin, then prints the total perplexity "
              << "to stdout.\n"
              << "The sentence FSTs are typically built by sent-to-fst.py.\n";
    exit(1);
  }

  // everything happens in the constructor of class PPLComputer.
  fstlm::PPLComputer(argc, argv);

  return 0;
}
