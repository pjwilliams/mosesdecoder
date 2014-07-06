#pragma once
#ifndef moses_ConstraintModelConstraintSetApplication_h
#define moses_ConstraintModelConstraintSetApplication_h

#include <taco/constraint_set.h>

#include <boost/functional/hash.hpp>

#include <vector>

namespace Moses {

class Factor;
class ChartHypothesis;

namespace CM {

struct ConstraintSetApplication {
  const taco::ConstraintSet *m_cs;
  std::vector<const Factor *> m_words;
  std::vector<const ChartHypothesis *> m_hypos;
};

struct ConstraintSetApplicationHasher {
 public:
  std::size_t operator()(const ConstraintSetApplication &csa) const {
    std::size_t seed = 0;
    boost::hash_combine(seed, csa.m_cs);
    boost::hash_combine(seed, csa.m_words);
    boost::hash_combine(seed, csa.m_hypos);
    return seed;
  }
};

struct ConstraintSetApplicationEqualityPred {
 public:
  bool operator()(const ConstraintSetApplication &a,
                  const ConstraintSetApplication &b) const {
    return a.m_cs == b.m_cs &&
           a.m_words == b.m_words &&
           a.m_hypos == b.m_hypos;
  }
};

}  // namespace CM
}  // namespace Moses

#endif
