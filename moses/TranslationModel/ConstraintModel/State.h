#pragma once
#ifndef moses_ConstraintModelState_h
#define moses_ConstraintModelState_h

#include "moses/FF/FFState.h"

#include <taco/feature_structure_set.h>

#include <vector>

namespace Moses
{
namespace CM
{

class ModelState : public FFState
{
 public:
  virtual int Compare(const FFState &) const;
  std::vector<taco::FeatureStructureSet> sets;
};

}  // namespace CM
}  // namespace Moses

#endif
