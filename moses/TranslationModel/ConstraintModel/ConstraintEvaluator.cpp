#include "ConstraintEvaluator.h"

#include "FailureTable.h"
#include "Model.h"
#include "State.h"

#include "moses/ChartHypothesis.h"
#include "moses/StaticData.h"

#include <taco/constraint_evaluator.h>
#include <taco/option_table.h>

#include <cassert>

namespace Moses {
namespace CM {

ConstraintEvaluator::ConstraintEvaluator(const ConstraintModel &model)
    : m_model(model) {
}

std::auto_ptr<const ModelState> ConstraintEvaluator::Eval(
    const ChartHypothesis &hypo,
    bool &failure,
    std::vector<float> *failureCounts) const {
  typedef std::vector<const CSParamPair *> CSParamPairVec;

  failure = false;

  const TargetPhrase &targetPhrase = hypo.GetCurrTargetPhrase();
  const CSParamPairVec *constraintSets = targetPhrase.GetConstraintSets();

  if (!constraintSets) {
    return std::auto_ptr<const CM::ModelState>(m_model.EmptyModelState());
  }

  const taco::FeatureSelectionRule *featureSelectionRule =
    targetPhrase.GetFeatureSelectionRule();

  const std::vector<std::vector<taco::Interpretation> > &
    partialInterpretationVec = targetPhrase.GetPartialInterpretations(m_model);

  assert(partialInterpretationVec.size() == constraintSets->size());

  ConstraintSetApplication csa;

  std::auto_ptr<CM::ModelState> state;

  std::vector<std::vector<taco::Interpretation> >::const_iterator p =
    partialInterpretationVec.begin();
  CSParamPairVec::const_iterator q = constraintSets->begin();
  for (; p != partialInterpretationVec.end(); ++p, ++q) {
    const std::vector<taco::Interpretation> &partialInterpretations = *p;
    const taco::ConstraintSet &constraintSet = (*q)->first;
    const CM::SubmodelParams &params = (*q)->second;

    ConstructCSA(constraintSet, hypo, csa);

    const std::vector<taco::Interpretation> *finalInterpretations;

    failure = !EvalCS(hypo.GetCurrTargetPhrase(), csa, params.csTypeIndex,
                      partialInterpretations, finalInterpretations);
    if (failure) {
      if (m_model.HardConstraint()) {
        // Exit early -- there's no point evaluating the remaining constraints.
        return std::auto_ptr<const CM::ModelState>(m_model.EmptyModelState());
      }
      if (failureCounts) {
        int id = params.csTypeIndex;
        if (id != -1) {
          assert(id >= 0);
          ++(*failureCounts)[id];
        }
      }
      continue;
    }

    if (constraintSet.ContainsRoot()) {
      assert(featureSelectionRule);
      const taco::FeatureStructureSet *set =
          m_model.ProcessRootInterpretations(*finalInterpretations,
                                            *featureSelectionRule);
      if (set) {
        if (!state.get()) {
          state.reset(m_model.EmptyModelState());
        }
        state->sets[params.csTypeIndex] = *set;
        delete set;
      }
    }
  }

  if (state.get()) {
    return std::auto_ptr<const CM::ModelState>(state);
  } else {
    return std::auto_ptr<const CM::ModelState>(m_model.EmptyModelState());
  }
}

void ConstraintEvaluator::BuildOptionTable(const TargetPhrase &targetPhrase,
                                           const ConstraintSetApplication &csa,
                                           int csTypeIndex,
                                           taco::OptionTable &optionTable) const
{
  std::set<int> indices;
  csa.m_cs->GetIndices(indices);

  std::vector<const ChartHypothesis *>::const_iterator q = csa.m_hypos.begin();
  for (std::set<int>::const_iterator p = indices.begin();
       p != indices.end(); ++p) {
    if (*p == 0) {
      continue;
    }
    int rhsIndex = *p - 1;
    assert(rhsIndex >= 0);
    assert(rhsIndex < targetPhrase.GetSize());
    const Word &word = targetPhrase.GetWord(rhsIndex);
    if (!word.IsNonTerminal()) {
      continue;
    }
    const ChartHypothesis *prevHypo = *q++;
    const FFState *base = prevHypo->GetFFState(m_model.GetFeatureId());
    const CM::ModelState *prevState = dynamic_cast<const CM::ModelState*>(base);
    assert(prevState);
    assert(!prevState->sets[csTypeIndex].empty());
    optionTable.AddColumn(*p, prevState->sets[csTypeIndex].begin(),
                          prevState->sets[csTypeIndex].end());
  }
}

void ConstraintEvaluator::ConstructCSA(const taco::ConstraintSet &cs,
                                       const ChartHypothesis &hypo,
                                       ConstraintSetApplication &csa) const
{
  csa.m_cs = &cs;
  csa.m_words.clear();
  csa.m_hypos.clear();

  const TargetPhrase &targetPhrase = hypo.GetCurrTargetPhrase();
  const std::vector<const ChartHypothesis*> &prevHypos = hypo.GetPrevHypos();

  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    targetPhrase.GetAlignNonTerm().GetNonTermIndexMap();

  std::set<int> indices;
  cs.GetIndices(indices);

  for (std::set<int>::const_iterator p = indices.begin();
       p != indices.end(); ++p) {
    if (*p == 0) {
      continue;
    }
    int rhsIndex = *p - 1;
    assert(rhsIndex >= 0);
    assert(rhsIndex < targetPhrase.GetSize());
    const Word &word = targetPhrase.GetWord(rhsIndex);
    if (word.IsNonTerminal()) {
      std::size_t nonTermInd = nonTermIndexMap[rhsIndex];
      const ChartHypothesis *prevHypo = prevHypos[nonTermInd];
      csa.m_hypos.push_back(prevHypo);
    } else {
      const Factor *f = word.GetFactor(0);
      assert(f);
      csa.m_words.push_back(f);
    }
  }
}

bool ConstraintEvaluator::EvalCS(
    const TargetPhrase &targetPhrase,
    const ConstraintSetApplication &csa,
    int csTypeIndex,
    const std::vector<taco::Interpretation> &partialInterpretations,
    const std::vector<taco::Interpretation> *&finalInterpretations) const {

  if (m_failureTable.Contains(csa)) {
    return false;
  }

  m_resources.Clear();

  // Build an option table containing columns for previous hypotheses
  // only (i.e. no root column and no terminal columns).
  BuildOptionTable(targetPhrase, csa, csTypeIndex, m_resources.optionTable);

  if (m_resources.optionTable.IsEmpty()) {
    // Constraints have been fully evaluated already.
    finalInterpretations = &partialInterpretations;
  } else {
    m_resources.evaluator.Eval(partialInterpretations, m_resources.optionTable,
                               *csa.m_cs, m_resources.interpretations);
    finalInterpretations = &m_resources.interpretations;
  }

  bool failure = finalInterpretations->empty();

  if (failure) {
    m_failureTable.Add(csa);
  }

  return !failure;
}

}  // namespace CM
}  // namespace Moses
