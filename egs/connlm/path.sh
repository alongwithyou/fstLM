export KALDI_ROOT=`pwd`/../../../kaldi
export PATH=$KALDI_ROOT/src/lmbin:$KALDI_ROOT/tools/openfst/bin:$PATH
export PATH=$KALDI_ROOT/egs/wsj/s5/utils/:$PATH
export LD_LIBRARY_PATH=$KALDI_ROOT/tools/openfst/lib:$LD_LIBRARY_PATH

export FSTLM_ROOT=`pwd`/../..
export PATH=$FSTLM_ROOT/src:$FSTLM_ROOT/scripts:$PATH

