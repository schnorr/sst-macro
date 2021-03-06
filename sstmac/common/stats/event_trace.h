/*
 *  This file is part of SST/macroscale:
 *               The macroscale architecture simulator from the SST suite.
 *  Copyright (c) 2009 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top
 *  SST/macroscale directory.
 */

#ifndef SSTMAC_COMMON_STATS_EVENT_TRACE_H_
#define SSTMAC_COMMON_STATS_EVENT_TRACE_H_

#include <sstmac/common/stats/stat_collector.h>
#include <sstmac/common/node_address.h>

namespace sstmac {

class event_trace :
  public stat_collector
{

 public:
  event_trace() :
    start_(-1), stop_(-1) {
  }

  virtual void
  simulation_finished(timestamp end);

  void
  collect(int key_typeid,
          const std::string& name,
          node_id addr,
          long threadid,
          int aid, int tid,
          long ticks_begin,
          long num_ticks);

  void
  reduce(stat_collector *coll);

  void
  dump_local_data();

  void
  dump_global_data();

  void
  global_reduce(parallel_runtime *rt);

  void
  init_factory_params(sprockit::sim_parameters *params);

  void
  clear();

  virtual
  ~event_trace() {
  }

  stat_collector*
  clone() const {
    return clone_me(-1);
  }

  event_trace*
  clone_me(int id) const {
    event_trace* cln = new event_trace;
    cln->set_id(id);
    clone_into(cln);
    return cln;
  }

 protected:
  void
  clone_into(event_trace *cln) const;

 protected:
  long start_;
  long stop_;

};

}

#endif /* EVENT_TRACE_H_ */

