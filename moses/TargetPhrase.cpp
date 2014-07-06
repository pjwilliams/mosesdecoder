// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <algorithm>
#include <stdlib.h>
#include <taco/constraint_evaluator.h>
#include <taco/option_table.h>
#include "util/exception.hh"
#include "util/tokenize_piece.hh"

#include "TargetPhrase.h"
#include "GenerationDictionary.h"
#include "LM/Base.h"
#include "StaticData.h"
#include "ScoreComponentCollection.h"
#include "Util.h"
#include "AlignmentInfoCollection.h"
#include "InputPath.h"
#include "moses/TranslationModel/PhraseDictionary.h"

using namespace std;

namespace Moses
{

TargetPhrase::GuardedPartialFSVec::GuardedPartialFSVec()
  : m_vec(NULL)
{
}

TargetPhrase::GuardedPartialFSVec::GuardedPartialFSVec(
    const GuardedPartialFSVec &)
  : m_vec(NULL)
{
}

TargetPhrase::GuardedPartialFSVec & TargetPhrase::GuardedPartialFSVec::operator=(
    const TargetPhrase::GuardedPartialFSVec &other)
{
  boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
  if (m_vec) {
    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(lock);
    delete m_vec;
    m_vec = NULL;
  }
}

TargetPhrase::TargetPhrase( std::string out_string)
  :Phrase(0)
  , m_fullScore(0.0)
  , m_futureScore(0.0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
  , m_ruleSource(NULL)
{

  //ACAT
  const StaticData &staticData = StaticData::Instance();
  CreateFromString(Output, staticData.GetInputFactorOrder(), out_string, 
		   // staticData.GetFactorDelimiter(), // eliminated [UG]
		   NULL);
}

TargetPhrase::TargetPhrase()
  :Phrase()
  , m_fullScore(0.0)
  , m_futureScore(0.0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
  , m_ruleSource(NULL)
{
}

TargetPhrase::TargetPhrase(const Phrase &phrase)
  : Phrase(phrase)
  , m_fullScore(0.0)
  , m_futureScore(0.0)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  , m_lhsTarget(NULL)
  , m_ruleSource(NULL)
{
}

TargetPhrase::TargetPhrase(const TargetPhrase &copy)
  : Phrase(copy)
  , m_fullScore(copy.m_fullScore)
  , m_futureScore(copy.m_futureScore)
  , m_scoreBreakdown(copy.m_scoreBreakdown)
  , m_alignTerm(copy.m_alignTerm)
  , m_alignNonTerm(copy.m_alignNonTerm)
{
  if (copy.m_lhsTarget) {
    m_lhsTarget = new Word(*copy.m_lhsTarget);
  } else {
    m_lhsTarget = NULL;
  }

  if (copy.m_ruleSource) {
    m_ruleSource = new Phrase(*copy.m_ruleSource);
  } else {
    m_ruleSource = NULL;
  }
}

TargetPhrase::~TargetPhrase()
{
  //cerr << "m_lhsTarget=" << m_lhsTarget << endl;

  delete m_lhsTarget;
  delete m_ruleSource;
}

#ifdef HAVE_PROTOBUF
void TargetPhrase::WriteToRulePB(hgmert::Rule* pb) const
{
  pb->add_trg_words("[X,1]");
  for (size_t pos = 0 ; pos < GetSize() ; pos++)
    pb->add_trg_words(GetWord(pos)[0]->GetString());
}
#endif

void TargetPhrase::Evaluate(const Phrase &source)
{
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  Evaluate(source, ffs);
}

void TargetPhrase::Evaluate(const Phrase &source, const std::vector<FeatureFunction*> &ffs)
{
  if (ffs.size()) {
    const StaticData &staticData = StaticData::Instance();
    ScoreComponentCollection futureScoreBreakdown;
    for (size_t i = 0; i < ffs.size(); ++i) {
      const FeatureFunction &ff = *ffs[i];
      if (! staticData.IsFeatureFunctionIgnored( ff )) {
        ff.Evaluate(source, *this, m_scoreBreakdown, futureScoreBreakdown);
      }
    }

    float weightedScore = m_scoreBreakdown.GetWeightedScore();
    m_futureScore += futureScoreBreakdown.GetWeightedScore();
    m_fullScore = weightedScore + m_futureScore;

  }
}

void TargetPhrase::Evaluate(const InputType &input, const InputPath &inputPath)
{
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  const StaticData &staticData = StaticData::Instance();
  ScoreComponentCollection futureScoreBreakdown;
  for (size_t i = 0; i < ffs.size(); ++i) {
    const FeatureFunction &ff = *ffs[i];
    if (! staticData.IsFeatureFunctionIgnored( ff )) {
      ff.Evaluate(input, inputPath, *this, NULL, m_scoreBreakdown, &futureScoreBreakdown);
    }
  }
  float weightedScore = m_scoreBreakdown.GetWeightedScore();
  m_futureScore += futureScoreBreakdown.GetWeightedScore();
  m_fullScore = weightedScore + m_futureScore;
}

void TargetPhrase::SetXMLScore(float score)
{
  const FeatureFunction* prod = PhraseDictionary::GetColl()[0];
  size_t numScores = prod->GetNumScoreComponents();
  vector <float> scoreVector(numScores,score/numScores);

  m_scoreBreakdown.Assign(prod, scoreVector);
}

void TargetPhrase::SetAlignmentInfo(const StringPiece &alignString)
{
  AlignmentInfo::CollType alignTerm, alignNonTerm;
  for (util::TokenIter<util::AnyCharacter, true> token(alignString, util::AnyCharacter(" \t")); token; ++token) {
    util::TokenIter<util::SingleCharacter, false> dash(*token, util::SingleCharacter('-'));

    char *endptr;
    size_t sourcePos = strtoul(dash->data(), &endptr, 10);
    UTIL_THROW_IF(endptr != dash->data() + dash->size(), util::ErrnoException, "Error parsing alignment" << *dash);
    ++dash;
    size_t targetPos = strtoul(dash->data(), &endptr, 10);
    UTIL_THROW_IF(endptr != dash->data() + dash->size(), util::ErrnoException, "Error parsing alignment" << *dash);
    UTIL_THROW_IF2(++dash, "Extra gunk in alignment " << *token);


    if (GetWord(targetPos).IsNonTerminal()) {
      alignNonTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    } else {
      alignTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    }
  }
  SetAlignTerm(alignTerm);
  SetAlignNonTerm(alignNonTerm);

}

void TargetPhrase::SetAlignTerm(const AlignmentInfo::CollType &coll)
{
  const AlignmentInfo *alignmentInfo = AlignmentInfoCollection::Instance().Add(coll);
  m_alignTerm = alignmentInfo;

}

void TargetPhrase::SetAlignNonTerm(const AlignmentInfo::CollType &coll)
{
  const AlignmentInfo *alignmentInfo = AlignmentInfoCollection::Instance().Add(coll);
  m_alignNonTerm = alignmentInfo;
}

void TargetPhrase::SetSparseScore(const FeatureFunction* translationScoreProducer, const StringPiece &sparseString)
{
  m_scoreBreakdown.Assign(translationScoreProducer, sparseString.as_string());
}

void TargetPhrase::Merge(const TargetPhrase &copy, const std::vector<FactorType>& factorVec)
{
  Phrase::MergeFactors(copy, factorVec);
  m_scoreBreakdown.Merge(copy.GetScoreBreakdown());
  m_futureScore += copy.m_futureScore;
  m_fullScore += copy.m_fullScore;
}

void TargetPhrase::SetProperties(const StringPiece &str)
{
  if (str.size() == 0) {
    return;
  }

  vector<string> toks;
  TokenizeMultiCharSeparator(toks, str.as_string(), "{{");
  for (size_t i = 0; i < toks.size(); ++i) {
    string &tok = toks[i];
    if (tok.empty()) {
      continue;
    }
    size_t endPos = tok.rfind("}");

    tok = tok.substr(0, endPos - 1);

    vector<string> keyValue = TokenizeFirstOnly(tok, " ");
    UTIL_THROW_IF2(keyValue.size() != 2,
    		"Incorrect format of property: " << str);
    SetProperty(keyValue[0], keyValue[1]);
  }
}

void TargetPhrase::SetProperty(const std::string &key, const std::string &value) 
{
  const StaticData &staticData = StaticData::Instance();
  const PhrasePropertyFactory& phrasePropertyFactory = staticData.GetPhrasePropertyFactory();
  m_properties[key] = phrasePropertyFactory.ProduceProperty(key,value);
}

const PhraseProperty *TargetPhrase::GetProperty(const std::string &key) const
{
  std::map<std::string, boost::shared_ptr<PhraseProperty> >::const_iterator iter;
  iter = m_properties.find(key);
  if (iter != m_properties.end()) {
    const boost::shared_ptr<PhraseProperty> &pp = iter->second;
    return pp.get();
  }
  return NULL;
}

void TargetPhrase::SetRuleSource(const Phrase &ruleSource) const
{
  if (m_ruleSource == NULL) {
    m_ruleSource = new Phrase(ruleSource);
  }
}

void TargetPhrase::GeneratePartialInterpretations(
    const CM::ConstraintModel &constraintModel,
    PartialFSVec &partialFSVec) const
{
  using namespace taco;

  typedef std::vector<const CM::CSParamPair *> CSParamPairVec;

  // Should only be called once.
  assert(partialFSVec.empty());

  const CSParamPairVec *constraintSets = GetConstraintSets();
  if (!constraintSets || constraintSets->empty()) {
    return;
  }

  partialFSVec.resize(constraintSets->size());

  ConstraintEvaluator evaluator;

  for (size_t i = 0; i < constraintSets->size(); ++i) {
    const CM::CSParamPair *pair = (*constraintSets)[i];
    const ConstraintSet &constraintSet = pair->first;
    const CM::SubmodelParams &params = pair->second;
    std::set<int> indices;
    constraintSet.GetIndices(indices);
    OptionTable optionTable;
    const Lexicon<std::size_t> &lexicon =
        constraintModel.GetLexicons()[params.csTypeIndex];
    BuildOptionTable(lexicon, indices, optionTable);
    evaluator.Eval(optionTable, constraintSet, partialFSVec[i]);
  }
}

void TargetPhrase::BuildOptionTable(const taco::Lexicon<std::size_t> &lexicon,
                                    const std::set<int> &indices,
                                    taco::OptionTable &optionTable) const
{
  using namespace taco;

  for (std::set<int>::const_iterator p = indices.begin();
       p != indices.end(); ++p) {
    if (*p == 0) { // LHS
      optionTable.AddWildcardColumn(0);
      continue;
    }
    int rhsIndex = *p - 1;
    assert(rhsIndex >= 0);
    assert(rhsIndex < GetSize());
    const Word &word = GetWord(rhsIndex);
    if (word.IsNonTerminal()) {
      optionTable.AddWildcardColumn(*p);
      continue;
    }
    const Factor *f = word.GetFactor(0);
    assert(f);
    size_t wordId = f->GetId();
    const Lexicon<std::size_t>::MappedType *fsVec = lexicon.Lookup(wordId);
    if (fsVec) {
      assert(!fsVec->empty());
      optionTable.AddColumn(*p, fsVec->begin(), fsVec->end());
    } else {
      // This should only happen for unknown source words that are untranslated.
      // The lexicon should contain at least one entry for every word that
      // occurs as a terminal in the main grammar.
      // TODO Emit a warning every time this happens?
      optionTable.AddWildcardColumn(*p);
    }
  }
}

void swap(TargetPhrase &first, TargetPhrase &second)
{
  first.SwapWords(second);
  std::swap(first.m_fullScore, second.m_fullScore);
  std::swap(first.m_futureScore, second.m_futureScore);
  swap(first.m_scoreBreakdown, second.m_scoreBreakdown);
  std::swap(first.m_alignTerm, second.m_alignTerm);
  std::swap(first.m_alignNonTerm, second.m_alignNonTerm);
  std::swap(first.m_lhsTarget, second.m_lhsTarget);
}

TO_STRING_BODY(TargetPhrase);

std::ostream& operator<<(std::ostream& os, const TargetPhrase& tp)
{
  if (tp.m_lhsTarget) {
    os << *tp.m_lhsTarget<< " -> ";
  }
  os << static_cast<const Phrase&>(tp) << ":" << flush;
  os << tp.GetAlignNonTerm() << flush;
  os << ": c=" << tp.m_fullScore << flush;
  os << " " << tp.m_scoreBreakdown << flush;
  
  const Phrase *sourcePhrase = tp.GetRuleSource();
  if (sourcePhrase) {
    os << " sourcePhrase=" << *sourcePhrase << flush;
  }

  if (tp.m_properties.size()) {
	os << " properties: " << flush;

	TargetPhrase::Properties::const_iterator iter;
	for (iter = tp.m_properties.begin(); iter != tp.m_properties.end(); ++iter) {
		const string &key = iter->first;
		const PhraseProperty *prop = iter->second.get();
		assert(prop);

		os << key << "=" << *prop << " ";
	}
  }

  return os;
}

}

