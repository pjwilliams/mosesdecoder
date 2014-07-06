#pragma once
#ifndef moses_ConstraintTable_h
#define moses_ConstraintTable_h

#include "CSParamPair.h"

#include <boost/shared_ptr.hpp>

#include <vector>

namespace Moses
{
namespace CM
{

class ConstraintTable
{
 private:
  typedef std::vector<boost::shared_ptr<CSParamPair> > Table;

 public:
  typedef Table::iterator iterator;
  typedef Table::const_iterator const_iterator;

  iterator begin() { return m_table.begin(); }
  iterator end() { return m_table.end(); }

  const_iterator begin() const { return m_table.begin(); }
  const_iterator end() const { return m_table.end(); }

  size_t size() const { return m_table.size(); }
  bool empty() const { return m_table.empty(); }

  void append(boost::shared_ptr<CSParamPair> p) { m_table.push_back(p); }

  const boost::shared_ptr<CSParamPair> &operator[](size_t i) const {
    return m_table[i];
  }

 private:
  Table m_table;
};

}  // namespace CM
}  // namespace Moses

#endif
