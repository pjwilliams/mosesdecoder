#pragma once
#ifndef moses_ConstraintModelConstraintEvaluator_h
#define moses_ConstraintModelConstraintEvaluator_h

#include "ConstraintSetApplication.h"
#include "FailureTable.h"
#include "State.h"

#include <taco/constraint_evaluator.h>
#include <taco/constraint_set.h>
#include <taco/interpretation.h>
#include <taco/option_table.h>

#include <boost/shared_ptr.hpp>

#include <set>
#include <vector>

namespace Moses {

class ChartHypothesis;
class TargetPhrase;

namespace CM {

class ConstraintModel;

class ConstraintEvaluator {
 public:
  ConstraintEvaluator(const ConstraintModel &);

  std::auto_ptr<const ModelState> Eval(const ChartHypothesis &, bool &,
                                       std::vector<float> *) const;

  const FailureTable &GetFailureTable() const { return m_failureTable; }

 private:
  struct EvalResources {
    void Clear() {
      interpretations.clear();
      optionTable.Clear();
    }
    taco::ConstraintEvaluator evaluator;
    std::vector<taco::Interpretation> interpretations;
    taco::OptionTable optionTable;
  };

  void ConstructCSA(const taco::ConstraintSet &, const ChartHypothesis &,
                    ConstraintSetApplication &) const;

  void BuildOptionTable(const TargetPhrase &, const ConstraintSetApplication &,
                        int, taco::OptionTable &) const;

  bool EvalCS(const TargetPhrase &, const ConstraintSetApplication &,
              int, const std::vector<taco::Interpretation> &,
              const std::vector<taco::Interpretation> *&) const;

  const ConstraintModel &m_model;
  mutable FailureTable m_failureTable;
  mutable EvalResources m_resources;
};

}  // namespace CM
}  // namespace Moses

#endif
