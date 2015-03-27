#pragma once

#include "moses/ScoreComponentCollection.h"

namespace Moses
{

class TargetPhrase;

namespace Syntax
{

struct SLabel {
  float inputWeight;
  float score;
  ScoreComponentCollection scoreBreakdown;
  const TargetPhrase *translation;
};

}  // Syntax
}  // Moses
