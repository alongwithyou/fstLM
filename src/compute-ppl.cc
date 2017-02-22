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
#include <fst/script/compile.h>

/**
   This program loads in a fstLM and reads the sentences FSTs from stdin,
   then computes the perplexity for sentence FSTs and prints it to the stdout.

*/

DEFINE_int32(debug, 0, "debug level");

namespace fstlm {
using namespace fst;

class PPLComputer {
 public:
  PPLComputer(int argc, char **argv):
      total_log_prob_(0.0),
      total_count_(0), cur_sent_id_(0),
      invalid_count_(0), num_invalid_sents_(0) {

    assert(argc >= 2);
    lm_ = VectorFst<StdArc>::Read(argv[1]);
    if (!lm_) {
      std::cerr << "compute-ppl: Reading LM FST error from"
          << argv[1] << '\n';
      exit(1);
    }
    ArcSort(lm_, ILabelCompare<StdArc>());

    ProcessInput();
    ProduceOutput();
  }

  ~PPLComputer() {
    delete lm_;
  }
 private:

  void ProcessInput() {
    while (std::cin.peek(), !std::cin.eof()) {
      string line;
      getline(std::cin, line);
      char format = line[0];
      int num_words = atoi(line.substr(1).c_str());

      VectorFst<StdArc> *fst = ReadNextFst(format);
      if (!fst) {
          std::cerr << "compute-ppl: Sent " << cur_sent_id_
              << ": Error read sentence FST\n";
          exit(1);
      }
      double p = ProcessCurrentFst(*fst);
      delete fst;

      cur_sent_id_++;

      if (p != 0.0) {
        total_count_ += num_words;
      } else {
        num_invalid_sents_ ++;
        invalid_count_ += num_words;
      }
    }
  }

  // this function reads next fst from stdin
  VectorFst<StdArc>* ReadNextFst(char format) {
    string content;  // The actual content of the input stream's next FST.
    if (format == 't') {
      content.clear();
      string line;
      while (getline(std::cin, line)) {
        if (line == "</FST>") {
            break;
        }
        content.append(line);
        content.append("\n");
      }

      std::istringstream is(content);
      std::ostringstream oss;
      oss << cur_sent_id_ << ".fst";
      FstCompiler<StdArc> fstcompiler(is, oss.str(), NULL,
                               NULL, NULL, false, false,
                               false, false, false);

      VectorFst<StdArc> *fst = fstcompiler.Fst().Copy();
      return fst;
    } else if (format == 'b') {
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
    } else {
      std::cerr << "compute-ppl: Sent " << cur_sent_id_
          << ": Unknown file format\n";
      exit(1);
    }
  }

  double ProcessCurrentFst(VectorFst<StdArc> &fst) {
      VectorFst<StdArc> comp_fst;
      VectorFst<StdArc> ofst;

      Compose(fst, *lm_, &comp_fst);

      ShortestPath(comp_fst, &ofst);

      double log_prob = GetProb(ofst);

      total_log_prob_ += log_prob;

#ifdef _FSTLM_DEBUG_
      std::ostringstream oss;
      oss << cur_sent_id_ << ".fst";
      fst.Write(oss.str());
      oss.clear();
      oss.str("");
      oss << cur_sent_id_ << ".comp.fst";
      comp_fst.Write(oss.str());
      oss.clear();
      oss.str("");
      oss << cur_sent_id_ << ".final.fst";
      ofst.Write(oss.str());
#endif

      return log_prob;
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

    if (FLAGS_debug >= 1) {
      std::cout << "Sent: " << cur_sent_id_ << "\n";
    }

    Weight weight = Weight::One();

    int32 num_words = 0;
    StateId st = initial_state;
    while (fst.Final(st) == Weight::Zero()) {
      assert(fst.NumArcs(st) == 1);
      for (ArcIterator<StdFst> aiter(fst, st); !aiter.Done(); aiter.Next()) {
        const StdArc &arc = aiter.Value();
        weight = Times(weight, arc.weight);
        if (FLAGS_debug >= 2) {
            std::cout << arc.ilabel << "\t" << arc.weight.Value() << "\n";
        }
        st = arc.nextstate;
      }
      num_words++;
    }
    if (FLAGS_debug >= 2 && fst.Final(st).Value() != 0.0) {
      std::cout << "Final: " << fst.Final(st).Value() << "\n";
    }

    if (FLAGS_debug >= 1) {
      std::cout << "Total: " << num_words << "\t" << weight.Value() << "\n";
    }

    return weight.Value();
  }

  void ProduceOutput() {
    double ppl = exp(total_log_prob_ / total_count_);
    std::cout << std::setprecision(10) << ppl << "\n";
    if (num_invalid_sents_ > 0) {
        std::cerr << "compute-ppl: There are " << num_invalid_sents_
            << " invalid sentences (with " << invalid_count_ << " words).\n";
    }
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
  int64 invalid_count_;
  int64 num_invalid_sents_;
};

}  // namespace fstlm

int main (int argc, char **argv) {
  const char *usage = "Usage: compute-ppl <lm_fst> < <sent_fsts>\n"
              "This program reads in a fst LM and compute the ppl for "
              "sentence FSTs from stdin, then prints the total perplexity "
              "to stdout.\n"
              "The sentence FSTs are typically built by sent-to-fst.py.\n";
  SET_FLAGS(usage, &argc, &argv, true);

  if (argc < 2) {
    ShowUsage();
    exit(1);
  }

  // everything happens in the constructor of class PPLComputer.
  fstlm::PPLComputer(argc, argv);

  return 0;
}
