#pragma once
#ifndef moses_ConstraintModelFailureTable_h
#define moses_ConstraintModelFailureTable_h

#include "ConstraintSetApplication.h"

#include <boost/unordered_set.hpp>

namespace Moses {
namespace CM {

class FailureTable {
 public:
  void Add(const ConstraintSetApplication &);
  bool Contains(const ConstraintSetApplication &) const;
 private:
  typedef boost::unordered_set<ConstraintSetApplication,
                               ConstraintSetApplicationHasher,
                               ConstraintSetApplicationEqualityPred> Set;
  Set m_set;
};

}  // namespace CM
}  // namespace Moses

#endif
