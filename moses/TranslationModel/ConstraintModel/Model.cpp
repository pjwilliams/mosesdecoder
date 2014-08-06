#include "Model.h"

#include "ConstraintEvaluator.h"
#include "State.h"

#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/FactorCollection.h"
#include "moses/StaticData.h"
#include "moses/UserMessage.h"

#include <taco/constraint_evaluator.h>
#include <taco/constraint_set_set.h>
#include <taco/option_table.h>
#include <taco/text-formats/constraint_parser.h>
#include <taco/text-formats/constraint_set_parser.h>
#include <taco/text-formats/constraint_table_parser.h>
#include <taco/text-formats/feature_structure_parser.h>
#include <taco/text-formats/lexicon_parser.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace Moses
{
namespace CM
{

ConstraintModel::ConstraintModel(const std::string &line)
    : StatefulFeatureFunction(line)
    , m_featureId(-1)
{
  std::cerr << "Creating constraint model..." << std::endl;

  ReadParameters();

  // Create a shared empty FeatureStructure for use in EmptyHypothesisState().
  {
    using namespace taco;
    FeatureStructureSpec spec;
    spec.content_pairs.insert(std::make_pair(FeaturePath(), kNullAtom));
    m_emptyFS.reset(new FeatureStructure(spec));
    assert(m_emptyFS->IsEmpty());
  }
}

void ConstraintModel::Load()
{
  // Load constraint tables.
  for (std::size_t i = 0; i < m_parameters.m_tableFiles.size(); ++i) {
    PrintUserTime(std::string("Start loading constraint table ") +
                    m_parameters.m_tableFiles[i]);
    // TODO Check for errors
    std::ifstream input(m_parameters.m_tableFiles[i].c_str());
    LoadConstraintTable(i, input);
    PrintUserTime("Finished loading constraint table");
  }

  // Load lexicon.
  for (std::size_t i = 0; i < m_parameters.m_lexiconFiles.size(); ++i) {
    const std::string &path = m_parameters.m_lexiconFiles[i];
    PrintUserTime(std::string("Start loading lexicon ") + path);
    // TODO Check for errors
    std::ifstream input(path.c_str());
    try {
      LoadLexicon(i, input);
    } catch (const util::Exception &e) {
      UTIL_THROW2("failed to load lexicon: `" << path << "': " << e.what());
    }
    PrintUserTime("Finished loading lexicon");
  }

  // Load feature selection table.
  {
    PrintUserTime(std::string("Start loading feature selection table ") +
                  m_parameters.m_featureSelectionFile);
    // TODO Check for errors
    std::ifstream input(m_parameters.m_featureSelectionFile.c_str());
    LoadFeatureSelectionTable(input);
    PrintUserTime("Finished loading feature selection table");
  }
}

void ConstraintModel::SetParameter(const std::string &key,
                                   const std::string &value)
{
  if (key == "feature-selection-file") {
    m_parameters.m_featureSelectionFile = value;
  } else if (key == "hard-constraint") {
    m_parameters.m_hardConstraint = Scan<bool>(value);
  } else if (key == "lexicon-files") {
    m_parameters.m_lexiconFiles = Tokenize(value);
  } else if (key == "table-files") {
    m_parameters.m_tableFiles = Tokenize(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* ConstraintModel::Evaluate(const Hypothesis &, const FFState *,
                                   ScoreComponentCollection *) const
{
  // This feature function cannot be used in the phrase-based decoder.
  assert(false);
  return 0;
}

FFState *ConstraintModel::EvaluateChart(
    const ChartHypothesis &hypo, int featureID,
    ScoreComponentCollection *accumulator) const
{
  if (HardConstraint()) {
    return 0;
  }
  bool failure;
  return EvaluateInternal(hypo, failure, accumulator);
}

const FFState *ConstraintModel::EmptyHypothesisState(const InputType &) const {
  return EmptyModelState();
}

FFState *ConstraintModel::EvaluateInternal(
  const ChartHypothesis &hypo,
  bool &failure,
  ScoreComponentCollection *scoreComponentCollection) const
{
  const ConstraintEvaluator &evaluator =
    hypo.GetManager().GetConstraintEvaluator();
  std::vector<float> numEvaluationFailures(GetNumScoreComponents());
  std::auto_ptr<const ModelState> state = evaluator.Eval(hypo, failure,
                                                         &numEvaluationFailures);
  std::auto_ptr<ModelState> state2(const_cast<ModelState *>(state.release()));
  scoreComponentCollection->PlusEquals(this, numEvaluationFailures);
  return state2.release();
}

ModelState *ConstraintModel::EmptyModelState() const {
  // The default CM state contains a single empty FS for each CS type.
  int numConstraintSetTypes = 1; // FIXME
  ModelState *state = new ModelState();
  state->sets.resize(numConstraintSetTypes);
  for (int i = 0; i < numConstraintSetTypes; ++i) {
    state->sets[i].insert(m_emptyFS);
  }
  return state;
}

const taco::FeatureStructureSet *
ConstraintModel::ProcessRootInterpretations(
    const std::vector<taco::Interpretation> &interpretations,
    const taco::FeatureSelectionRule &featureSelectionRule) const
{
  using namespace taco;

  const FeatureSelectionRule::RuleType ruleType = featureSelectionRule.type;

  if (ruleType == FeatureSelectionRule::Rule_Drop) {
    return 0;
  }

  FeatureStructureSet *fsSet = new FeatureStructureSet();
  for (std::vector<Interpretation>::const_iterator p = interpretations.begin();
       p != interpretations.end(); ++p) {
    boost::shared_ptr<const FeatureStructure> fs = p->GetFS(0);
    assert(fs);
    if (ruleType == FeatureSelectionRule::Rule_Select) {
      fs = fs->SelectiveClone(*(featureSelectionRule.selection_tree));
    }
    if (fs->IsEmpty() || fs->IsEffectivelyEmpty()) {
      // An empty FS trumps everything else.
      delete fsSet;
      return 0;
    }
    fsSet->insert(fs);
  }

  return fsSet;
}

void ConstraintModel::LoadLexicon(int i, std::istream &input)
{
  using namespace taco;

  if (i >= m_lexicons.size()) {
    m_lexicons.resize(i+1);
  }

  Lexicon<std::size_t> &lexicon = m_lexicons[i];
  assert(lexicon.IsEmpty());

  FactorCollection &factorCollection = FactorCollection::Instance();

  FeatureStructureParser fsParser(m_featureSet, m_valueSet);

  std::size_t lineNum = 1;

  try {
    LexiconParser end;
    for (LexiconParser parser(input); parser != end; ++parser) {
      const LexiconParser::Entry &entry = *parser;
      const Factor *f = factorCollection.AddFactor(Output, 0, entry.word);
      assert(f);
      size_t wordId = f->GetId();
      boost::shared_ptr<FeatureStructure> fs = fsParser.Parse(entry.fs);
      lexicon.Insert(wordId, fs);
      ++lineNum;
    }
  } catch (const taco::Exception &e) {
    UTIL_THROW2("line " << lineNum  << ": " << e.msg());
  }
}

void ConstraintModel::LoadConstraintTable(int i, std::istream &input)
{
  using namespace taco;

  if (i >= m_constraintTables.size()) {
    m_constraintTables.resize(i+1);
  }

  ConstraintTable &table = m_constraintTables[i];
  assert(table.empty());

  SubmodelParams params;
  params.csTypeIndex = i;

  // Constraint ID origin is 1, so insert an empty constraint set at index 0.
  // FIXME Sort this origin business out.  It's a constant source of errors.
  table.append(boost::shared_ptr<CSParamPair>());

  ConstraintSetParser csParser(m_featureSet, m_valueSet);

  ConstraintTableParser end;
  for (ConstraintTableParser parser(input); parser != end; ++parser) {
    const ConstraintTableParser::Entry &entry = *parser;
    // Check that the constraint ID is the same as the next table index.
    size_t id = std::atoi(entry.id.as_string().c_str());
    assert(id == table.size());
    boost::shared_ptr<ConstraintSet> cs = csParser.Parse(entry.constraint_set);
    boost::shared_ptr<CSParamPair> csp(new CSParamPair(*cs, params));
    table.append(csp);
  }
}

void ConstraintModel::LoadFeatureSelectionTable(std::istream &input)
{
  using namespace taco;

  assert(m_featureSelectionTable.empty());

  FeatureSelectionTableParser end;
  for (FeatureSelectionTableParser parser(input, m_featureSet);
       parser != end; ++parser) {
    const FeatureSelectionTableParser::Entry &entry = *parser;
    // Check that the feature selection ID is the same as the next table index.
    assert(entry.index == m_featureSelectionTable.size());
    m_featureSelectionTable.push_back(entry.rule);
  }
}

}  // namespace CM
}  // namespace Moses
