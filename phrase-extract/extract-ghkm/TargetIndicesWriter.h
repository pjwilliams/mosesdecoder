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

#pragma once
#ifndef EXTRACT_GHKM_TARGET_INDICES_WRITER_H_
#define EXTRACT_GHKM_TARGET_INDICES_WRITER_H_

#include <boost/unordered_map.hpp>

#include <ostream>

namespace Moses {
namespace GHKM {

class AlignmentGraph;
class Node;
class Subgraph;

// Writes rules as sequences of indices corresponding to the input alignment
// graph.  For example, the rule
//
// 905662 ||| 26 27 49 51 53
//
// comes from the alignment graph with ID 905662.  The target LHS is the node
// with index 53 and the target RHS is the sequence of nodes with indices 26,
// 27, 49, and 51.
//
// Indices are assigned to an alignment graph's target tree nodes by visiting
// them in depth first order and allocating IDs contiguously from 0.
//
// IndexGraph() should be called to assign IDs for a graph before calling
// Write() once for each rule fragment.
//
// FIXME Make this less clunky.  Maybe pass the graph, subgraph, and index
// to Write and do the indexing transparently.
class TargetIndicesWriter
{
 public:
  TargetIndicesWriter(std::ostream &out) : m_out(out) {}

  void IndexGraph(const AlignmentGraph &, size_t);

  void Write(const Subgraph &);

 private:
  // Disallow copying
  TargetIndicesWriter(const TargetIndicesWriter &);
  TargetIndicesWriter &operator=(const TargetIndicesWriter &);

  void Index(const Node &, size_t &);

  std::ostream &m_out;
  size_t m_graphId;
  boost::unordered_map<const Node *, size_t> m_nodeIndices;
};

}  // namespace GHKM
}  // namespace Moses

#endif
