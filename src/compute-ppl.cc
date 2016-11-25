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
   This program loads in a fstLM and reads the sentences fsts from stdin,
   then computes the perplexity for each sentence fsts and prints them to
   the stdout.

*/

namespace fstlm {
using namespace fst;

class PPLComputer {
 public:
  PPLComputer(int argc,
               const char **argv):
      total_log_prob_(0.0),
      total_count_(0) {

    assert(argc >= 2);
    lm_ = VectorFst<StdArc>::Read(argv[1]);

    ProcessInput();
    ProduceOutput();
  }

  ~PPLComputer() {
    delete lm_;
  }
 private:

  void ProcessInput() {
    while (std::cin.peek(), !std::cin.eof()) {
      VectorFst<StdArc> *fst = ReadNextFst();
      ProcessCurrentFst(fst);
      delete fst;
    }
  }

  // this function reads next fst from stdin
  VectorFst<StdArc>* ReadNextFst() {
    fst::FstHeader hdr;
    if (!hdr.Read(std::cin, "stdin"))
      std::cerr << "Reading FST: error reading FST header";
    FstReadOptions ropts("<unspecified>", &hdr);
    VectorFst<StdArc> *fst = VectorFst<StdArc>::Read(std::cin, ropts);
    if (!fst)
      std::cerr << "Could not read sentenct fst";
    return fst;
  }

  void ProcessCurrentFst(VectorFst<StdArc> *fst) {
      VectorFst<StdArc> comp_fst;
      VectorFst<StdArc> ofst;

      Compose(*lm_, *fst, &comp_fst);

      ShortestPath(comp_fst, &ofst);

      ofst.Write("1.fst");
  }

  void ProduceOutput() {
    double ppl = exp(-total_log_prob_ / total_count_);
    std::cout << std::setprecision(10) << ppl << "\n";
    std::cerr << "compute-ppl: average log-prob per word was "
              << (total_log_prob_ / total_count_)
              << " (perplexity = "
              << exp(-total_log_prob_ / total_count_) << ") over "
              << total_count_ << " words.\n";
  }

  // lm_ is the fst for lm.
  VectorFst<StdArc> *lm_;

  double total_log_prob_;
  int64 total_count_;
};

}  // namespace fstlm

int main (int argc, const char **argv) {
  if (argc < 2) {
    std::cerr << "usage:\n"
              << "compute-ppl <lm_fst> \n"
              << "This program reads in a fst LM and compute the ppl for "
              << "sentence fsts from stdin, then prints the total perplexity "
              << "to stdout.\n"
              << "The sentence fsts are typically built by sent-to-fst.py.\n";
    exit(1);
  }

  // everything happens in the constructor of class PPLComputer.
  fstlm::PPLComputer(argc, argv);

  return 0;
}
