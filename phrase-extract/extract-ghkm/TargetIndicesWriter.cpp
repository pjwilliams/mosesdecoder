/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh
 
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

#include "TargetIndicesWriter.h"

#include "AlignmentGraph.h"
#include "Node.h"
#include "Subgraph.h"

#include <cassert>
#include <ostream>
#include <map>
#include <sstream>
#include <vector>

namespace Moses {
namespace GHKM {

void TargetIndicesWriter::IndexGraph(const AlignmentGraph &graph,
                                     size_t graphId)
{
  m_graphId = graphId;
  m_nodeIndices.clear();
  size_t nextIndex = 0;
  Index(*(graph.GetRoot()), nextIndex);
}

void TargetIndicesWriter::Index(const Node &root, size_t &nextIndex)
{
  const std::vector<Node*> &children = root.GetChildren();
  for (std::vector<Node*>::const_iterator p = children.begin();
       p != children.end(); ++p) {
    const Node &child = **p;
    if (child.GetType() != SOURCE) {
      Index(child, nextIndex);
    }
  }
  m_nodeIndices[&root] = nextIndex++;
}

void TargetIndicesWriter::Write(const Subgraph &fragment)
{
  // Graph ID
  m_out << m_graphId << " |||";

  // RHS indices
  std::vector<const Node *> targetLeaves;
  fragment.GetTargetLeaves(targetLeaves);
  for (std::vector<const Node *>::const_iterator p(targetLeaves.begin());
       p != targetLeaves.end(); ++p) {
    const Node &leaf = **p;
    if (leaf.GetSpan().empty()) {
      std::vector<const Node *> targetNodes;
      leaf.GetTargetNodes(targetNodes);
      for (std::vector<const Node *>::const_iterator q(targetNodes.begin());
           q != targetNodes.end(); ++q) {
        m_out << " " << m_nodeIndices[*q];
      }
    } else if (leaf.GetType() == SOURCE) {
      // Do nothing
    } else {
      m_out << " " << m_nodeIndices[&leaf];
    }
  }

  // LHS index
  m_out << " " << m_nodeIndices[fragment.GetRoot()] << std::endl;
}

}  // namespace GHKM
}  // namespace Moses
