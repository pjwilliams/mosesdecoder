#pragma once
#ifndef moses_ConstraintModelState_h
#define moses_ConstraintModelState_h

#include "FeatureStructureSet.h"

#include "moses/FF/FFState.h"

#include "taco/base/hash_combine.h"

#include <vector>

namespace Moses
{
namespace CM
{

class ModelState : public FFState
{
 public:
  virtual std::size_t hash() const {
    FeatureStructureSetHash setHash;
    std::size_t seed = 0;
    for (std::vector<FeatureStructureSet>::const_iterator p = sets.begin();
         p != sets.end(); ++p) {
      taco::hash_combine(seed, setHash(*p));
    }
    return seed;
  };

  virtual bool operator==(const FFState& other) const {
    const ModelState &cms = dynamic_cast<const ModelState &>(other);
    if (sets.size() != cms.sets.size()) {
      return false;
    }
    FeatureStructureSetEqual fsSetEqual;
    std::vector<FeatureStructureSet>::const_iterator p = sets.begin();
    std::vector<FeatureStructureSet>::const_iterator q = cms.sets.begin();
    while (p != sets.end()) {
      if (!fsSetEqual(*p++, *q++)) {
        return false;
      }
    }
    return true;
  };

  std::vector<FeatureStructureSet> sets;
};

}  // namespace CM
}  // namespace Moses

#endif
