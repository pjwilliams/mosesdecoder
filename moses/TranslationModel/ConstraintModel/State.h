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

class FeatureStructureSetEqualityPred
{
 public:
  bool operator()(const FeatureStructureSet &x,
                  const FeatureStructureSet &y) const {
    if (x.size() != y.size()) {
      return false;
    }
    FeatureStructureEqualityPred fsPred;
    FeatureStructureSet::const_iterator p = x.begin();
    FeatureStructureSet::const_iterator q = y.begin();
    while (p != x.end()) {
      if (!fsPred(**p, **q)) {
        return false;
      }
    }
    return true;
  };
};

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
    const ModelState &cms = dynamic_cast<const ModelState &>(other);
    if (sets.size() != cms.sets.size()) {
      return false;
    }
    taco::FeatureStructureSetEqualityPred fsSetPred;
    std::vector<taco::FeatureStructureSet>::const_iterator p = sets.begin();
    std::vector<taco::FeatureStructureSet>::const_iterator q = cms.sets.begin();
    while (p != sets.end()) {
      if (!fsSetPred(*p, *q)) {
        return false;
      }
    }
    return true;
  };

  std::vector<taco::FeatureStructureSet> sets;
};

}  // namespace CM
}  // namespace Moses

#endif
