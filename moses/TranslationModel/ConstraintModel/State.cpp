#include "State.h"

#include <algorithm>

namespace Moses
{
namespace CM
{

int ModelState::Compare(const FFState &other) const
{
  const ModelState &cms = dynamic_cast<const ModelState &>(other);

  assert(sets.size() == cms.sets.size());

  for (std::size_t i = 0; i < sets.size(); ++i) {
    const taco::FeatureStructureSet &a = sets[i];
    const taco::FeatureStructureSet &b = cms.sets[i];
    if (std::lexicographical_compare(
          a.begin(), a.end(), b.begin(), b.end(),
          taco::FeatureStructureSet::Orderer())) {
      return -1;
    } else if (std::lexicographical_compare(
          b.begin(), b.end(), a.begin(), a.end(),
          taco::FeatureStructureSet::Orderer())) {
      return 1;
    }
  }
  return 0;
}

}  // namespace CM
}  // namespace Moses
