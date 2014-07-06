#pragma once
#ifndef moses_ConstraintModelSubmodelParams_h
#define moses_ConstraintModelSubmodelParams_h

namespace Moses
{
namespace CM
{

struct SubmodelParams
{
 public:
  SubmodelParams() : csTypeIndex(-1) {}
  int csTypeIndex;
};

}  // namespace CM
}  // namespace Moses

#endif
