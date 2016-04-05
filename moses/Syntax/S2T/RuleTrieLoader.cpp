#include "RuleTrieLoader.h"

#include <sys/stat.h>
#include <cstdlib>

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <cmath>

#include "moses/FactorCollection.h"
#include "moses/Word.h"
#include "moses/Util.h"
#include "moses/Timer.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/Range.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/FactorCollection.h"
#include "moses/Syntax/RuleTableFF.h"
#include "moses/TranslationModel/ConstraintModel/Model.h"
#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/double-conversion/double-conversion.h"
#include "util/exception.hh"

#include "RuleTrie.h"
#include "moses/parameters/AllOptions.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

bool RuleTrieLoader::Load(Moses::AllOptions const& opts,
                          const std::vector<FactorType> &input,
                          const std::vector<FactorType> &output,
                          const std::string &inFile,
                          const RuleTableFF &ff,
                          RuleTrie &trie)
{
  typedef std::pair<unsigned int, unsigned int> IdPair;
  typedef std::set<IdPair> IdPairSet;
  typedef boost::unordered_map<
      IdPairSet,
      boost::shared_ptr<std::vector<const CM::CSParamPair *> >
    > IdPairSetMap;

  PrintUserTime(std::string("Start loading text phrase table. Moses format"));

  // const StaticData &staticData = StaticData::Instance();
  const StaticData &staticData = StaticData::Instance();
  const std::string& factorDelimiter = staticData.GetFactorDelimiter();
  const CM::ConstraintModel *constraintModel = staticData.GetConstraintModel();

  IdPairSetMap idPairSetMap;

  std::size_t count = 0;

  std::ostream *progress = NULL;
  IFVERBOSE(1) progress = &std::cerr;
  util::FilePiece in(inFile.c_str(), progress);

  // reused variables
  std::vector<float> scoreVector;
  StringPiece line;

  double_conversion::StringToDoubleConverter converter(double_conversion::StringToDoubleConverter::NO_FLAGS, NAN, NAN, "inf", "nan");

  while(true) {
    try {
      line = in.ReadLine();
    } catch (const util::EndOfFileException &e) {
      break;
    }

    util::TokenIter<util::MultiCharacter> pipes(line, "|||");
    StringPiece sourcePhraseString(*pipes);
    StringPiece targetPhraseString(*++pipes);
    StringPiece scoreString(*++pipes);

    StringPiece alignString;
    if (++pipes) {
      StringPiece temp(*pipes);
      alignString = temp;
    }

    bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == std::string::npos);
    if (isLHSEmpty && !opts.unk.word_deletion_enabled) { // staticData.IsWordDeletionEnabled()) {
      TRACE_ERR( ff.GetFilePath() << ":" << count << ": pt entry contains empty target, skipping\n");
      continue;
    }

    scoreVector.clear();
    for (util::TokenIter<util::AnyCharacter, true> s(scoreString, " \t"); s; ++s) {
      int processed;
      float score = converter.StringToFloat(s->data(), s->length(), &processed);
      UTIL_THROW_IF2(std::isnan(score), "Bad score " << *s << " on line " << count);
      scoreVector.push_back(FloorScore(TransformScore(score)));
    }
    const size_t numScoreComponents = ff.GetNumScoreComponents();
    if (scoreVector.size() != numScoreComponents) {
      UTIL_THROW2("Size of scoreVector != number (" << scoreVector.size() << "!="
                  << numScoreComponents << ") of score components on line " << count);
    }

    // parse source & find pt node

    // constituent labels
    Word *sourceLHS = NULL;
    Word *targetLHS;

    // create target phrase obj
    TargetPhrase *targetPhrase = new TargetPhrase(&ff);
    targetPhrase->CreateFromString(Output, output, targetPhraseString, &targetLHS);
    // source
    Phrase sourcePhrase;
    sourcePhrase.CreateFromString(Input, input, sourcePhraseString, &sourceLHS);

    // rest of target phrase
    targetPhrase->SetAlignmentInfo(alignString);
    targetPhrase->SetTargetLHS(targetLHS);

    ++pipes;  // skip over counts field.

    if (++pipes) {
      StringPiece sparseString(*pipes);
      targetPhrase->SetSparseScore(&ff, sparseString);
    }

    if (++pipes) {
      StringPiece propertiesString(*pipes);
      targetPhrase->SetProperties(propertiesString);
    }

    // Parse constraint model IDs and add constraint set pointers and feature
    // selection rule pointers to TargetPhrases.
    // TODO This assumes that ConstraintModel::Load() has already been called.
    // TODO Double-check that that's guaranteed to always be true.
    if (constraintModel && ++pipes) {
      const std::vector<CM::ConstraintTable> &constraintTables =
          constraintModel->GetConstraintTables();
      const taco::FeatureSelectionTable &featureSelectionTable =
          constraintModel->GetFeatureSelectionTable();
      std::vector<std::pair<unsigned int, unsigned int> > idPairVector;
      std::vector<std::string> tmpVector;
      // Add constraint set IDs to targetPhrase
      Tokenize<std::string>(tmpVector, pipes->as_string());
      idPairVector.resize(tmpVector.size());
      for (std::size_t i = 0; i < tmpVector.size(); ++i) {
        std::size_t pos = tmpVector[i].find(':');
        const char *s = tmpVector[i].c_str();
        idPairVector[i].first = std::atoi(s);
        idPairVector[i].second = std::atoi(s+pos+1);
      }
      bool hasConstraintSetIds = !idPairVector.empty();
      if (hasConstraintSetIds) {
        // TODO Using a std::set to order indices (and remove duplicates) is
        // overkill.  Use std::vector, std::sort, and std::unique instead.
        IdPairSet ids;
        ids.insert(idPairVector.begin(), idPairVector.end());
        IdPairSetMap::const_iterator p = idPairSetMap.find(ids);
        if (p != idPairSetMap.end()) {
          targetPhrase->SetConstraintSets(p->second);
        }
        else {
          boost::shared_ptr<std::vector<const CM::CSParamPair *> > constraintSets(
              new std::vector<const CM::CSParamPair *>);
          constraintSets->reserve(ids.size());
          for (IdPairSet::const_iterator q = ids.begin(); q != ids.end(); ++q) {
            const IdPair &id = *q;
            const CM::CSParamPair *pair =
                constraintTables[id.first][id.second].get();
            constraintSets->push_back(pair);
          }
          idPairSetMap[ids] = constraintSets;
          targetPhrase->SetConstraintSets(constraintSets);
        }
      }
      // Add feature selection rule ID to targetPhrase
      ++pipes;
      std::vector<unsigned int> idVector;
      Tokenize<unsigned int>(idVector, pipes->as_string());
      assert(idVector.size() == 0 || idVector.size() == 1);
      if (hasConstraintSetIds && idVector.size() == 1) {
        unsigned int index = idVector[0];
        targetPhrase->SetFeatureSelectionRule(featureSelectionTable[index]);
      }
    }

    targetPhrase->GetScoreBreakdown().Assign(&ff, scoreVector);
    targetPhrase->EvaluateInIsolation(sourcePhrase, ff.GetFeaturesToApply());

    TargetPhraseCollection::shared_ptr phraseColl
    = GetOrCreateTargetPhraseCollection(trie, sourcePhrase,
                                        *targetPhrase, sourceLHS);
    phraseColl->Add(targetPhrase);

    // not implemented correctly in memory pt. just delete it for now
    delete sourceLHS;

    count++;
  }

  // sort and prune each target phrase collection
  if (ff.GetTableLimit()) {
    SortAndPrune(trie, ff.GetTableLimit());
  }

  return true;
}

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
