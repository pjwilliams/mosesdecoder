#pragma once
#ifndef moses_ConstraintModelState_h
#define moses_ConstraintModelState_h

#include "moses/FF/FFState.h"

#include <taco/feature_structure_set.h>

#include <vector>

namespace taco
{

std::size_t hash_value(const FeatureStructure &x) {
  taco::FeatureStructureHasher hasher;
  return hasher(x);
};

std::size_t hash_value(const FeatureStructureSet &s) {
  std::size_t seed = 0;
  for (FeatureStructureSet::const_iterator p = s.begin(); p != s.end(); ++p) {
    boost::hash_combine(seed, **p);
  }
  return seed;
}

}

namespace Moses
{
namespace CM
{

class ModelState : public FFState
{
 public:
  virtual std::size_t hash() const {
    return boost::hash_range(sets.begin(), sets.end());
  };

  virtual bool operator==(const FFState& other) const {
    return sets == other.sets;
  };

  std::vector<taco::FeatureStructureSet> sets;
};

}  // namespace CM
}  // namespace Moses

#endif
