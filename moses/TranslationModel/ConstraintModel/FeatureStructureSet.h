#pragma once
#ifndef moses_ConstraintModelFeatureStructureSet_h
#define moses_ConstraintModelFeatureStructureSet_h

#include <taco/base/hash_combine.h>
#include <taco/base/utility.h>
#include <taco/feature_structure.h>

#include <set>

namespace Moses
{
namespace CM
{

// An ordered set of FeatureStructures, which are stored using shared_ptrs but
// ordered using their dereferenced values.  Note that FeatureStructureSet uses
// taco::BadFeatureStructureOrderer, which means that any structure sharing
// differences are ignored.
// TODO Would boost::flat_set be a better choice?  FeatureStructureSet is only
// used for storing FF state and once the state object has been produced, it
// can't be modified, only read.  flat_set's slower insertion might be
// outweighed by its faster iteration.  Or it may not make any real difference
// in practice...
typedef std::set<boost::shared_ptr<const taco::FeatureStructure>,
                 taco::DereferencingOrderer<
                    boost::shared_ptr<const taco::FeatureStructure>,
                    taco::BadFeatureStructureOrderer>
                > FeatureStructureSet;

// Hash function for FeatureStructureSet.
class FeatureStructureSetHash
{
 public:
  std::size_t operator()(const FeatureStructureSet &s) const {
    std::size_t seed = 0;
    taco::BadFeatureStructureHasher fsHash;
    for (FeatureStructureSet::const_iterator p = s.begin(); p != s.end(); ++p) {
      taco::hash_combine(seed, fsHash(**p));
    }
    return seed;
  }
};

// Equality predicate for FeatureStructureSet.
class FeatureStructureSetEqual
{
 public:
  bool operator()(const FeatureStructureSet &x,
                  const FeatureStructureSet &y) const {
    if (x.size() != y.size()) {
      return false;
    }
    taco::BadFeatureStructureEqualityPred fsEqual;
    FeatureStructureSet::const_iterator p = x.begin();
    FeatureStructureSet::const_iterator q = y.begin();
    while (p != x.end()) {
      if (!fsEqual(**p++, **q++)) {
        return false;
      }
    }
    return true;
  };
};

}  // namespace CM
}  // namespace Moses

#endif
