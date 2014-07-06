#pragma once
#ifndef moses_ConstraintModelCSParamPair_h
#define moses_ConstraintModelCSParamPair_h

#include "SubmodelParams.h"

#include <taco/constraint_set.h>

namespace Moses
{
namespace CM
{

typedef std::pair<taco::ConstraintSet, SubmodelParams> CSParamPair;

}  // namespace CM
}  // namespace Moses

#endif
