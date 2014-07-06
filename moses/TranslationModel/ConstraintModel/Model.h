#pragma once
#ifndef moses_ConstraintModelModel_h
#define moses_ConstraintModelModel_h

#include "State.h"
#include "Table.h"
#include "CSParamPair.h"

#include "moses/FactorCollection.h"
#include "moses/FF/StatefulFeatureFunction.h"

#include <taco/base/vocabulary.h>
#include <taco/constraint_set.h>
#include <taco/feature_selection_table.h>
#include <taco/feature_structure.h>
#include <taco/feature_structure_set.h>
#include <taco/interpretation.h>
#include <taco/lexicon.h>
#include <taco/text-formats/feature_structure_writer.h>

#include <boost/shared_ptr.hpp>

#include <cassert>
#include <istream>
#include <set>
#include <string>
#include <vector>
#include <utility>

namespace taco
{
class ConstraintSetSet;
class OptionTable;
}

namespace Moses
{
namespace CM
{

class ConstraintModel : public StatefulFeatureFunction
{
 public:
  ConstraintModel(const std::string &);

  // Override FeatureFunction function.
  bool IsUseable(const FactorMask &) const { return true; }

  // Override FeatureFunction function.
  void Evaluate(const Phrase &, const TargetPhrase &,
                ScoreComponentCollection &,
                ScoreComponentCollection &) const {}

  // Override FeatureFunction function.
  void Evaluate(const InputType &, const InputPath &, const TargetPhrase &,
                const StackVec *, ScoreComponentCollection &,
                ScoreComponentCollection *) const {}

  // Override FeatureFunction function.
  void Load();

  // Override FeatureFunction function.
  void SetParameter(const std::string &, const std::string &);

  // Override StatefulFeatureFunction function.
  FFState* Evaluate(const Hypothesis &, const FFState *,
                    ScoreComponentCollection *) const;

  // Override StatefulFeatureFunction function.
  FFState *EvaluateChart(const ChartHypothesis &, int,
                         ScoreComponentCollection *) const;

  // Override StatefulFeatureFunction function.
  const FFState *EmptyHypothesisState(const InputType &) const;

  // Return index of object in StatefulFeatureFunction::m_statefulFFs vector.
  int GetFeatureId() const { return m_featureId; }
  void SetFeatureId(int i) { m_featureId = i; }

  //std::string GetScoreProducerWeightShortName(unsigned) const { return "cm"; }

  FFState *EvaluateInternal(const ChartHypothesis &,
                            bool &,
                            ScoreComponentCollection *) const;

  taco::Vocabulary &GetFeatureSet() { return m_featureSet; }
  const taco::Vocabulary &GetFeatureSet() const { return m_featureSet; }

  taco::Vocabulary &GetValueSet() { return m_valueSet; }
  const taco::Vocabulary &GetValueSet() const { return m_valueSet; }

  const std::vector<ConstraintTable> &GetConstraintTables() const {
    return m_constraintTables;
  }

  const std::vector<taco::Lexicon<std::size_t> > &GetLexicons() const {
    return m_lexicons;
  }

  const taco::FeatureSelectionTable &GetFeatureSelectionTable() const {
    return m_featureSelectionTable;
  }

  ModelState *EmptyModelState() const;

  void LoadLexicon(int, std::istream &);
  void LoadConstraintTable(int, std::istream &);
  void LoadFeatureSelectionTable(std::istream &);

  bool HardConstraint() const { return m_parameters.m_hardConstraint; }

  const taco::FeatureStructureSet *ProcessRootInterpretations(
      const std::vector<taco::Interpretation> &,
      const taco::FeatureSelectionRule &) const;

 private:
  struct Parameters {
    Parameters() : m_hardConstraint(false) {}
    bool m_hardConstraint;
    std::string m_featureSelectionFile;
    std::vector<std::string> m_lexiconFiles;
    std::vector<std::string> m_tableFiles;
  };

  Parameters m_parameters;
  std::vector<int> m_scoreIndices;
  taco::Vocabulary m_featureSet;
  taco::Vocabulary m_valueSet;
  std::vector<taco::Lexicon<std::size_t> > m_lexicons;
  std::vector<ConstraintTable> m_constraintTables;
  taco::FeatureSelectionTable m_featureSelectionTable;
  boost::shared_ptr<taco::FeatureStructure> m_emptyFS;
  boost::shared_ptr<const ModelState> m_defaultStateValue;
  int m_featureId;
};

}  // namespace CM
}  // namespace Moses

#endif
