#include "FailureTable.h"

namespace Moses {
namespace CM {

void FailureTable::Add(const ConstraintSetApplication &csa) {
  m_set.insert(csa);
}

bool FailureTable::Contains(const ConstraintSetApplication &csa) const {
  return m_set.find(csa) != m_set.end();
}

}  // namespace CM
}  // namespace Moses
