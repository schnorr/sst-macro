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

#ifndef SSTMAC_BACKENDS_NATIVE_COMPONENTS_NIC_NETWORKINTERFACE_H_INCLUDED
#define SSTMAC_BACKENDS_NATIVE_COMPONENTS_NIC_NETWORKINTERFACE_H_INCLUDED

#include <sstmac/common/timestamp.h>
#include <sstmac/hardware/node/node_fwd.h>
#include <sstmac/hardware/common/failable.h>
#include <sstmac/hardware/common/connection.h>
#include <sstmac/hardware/common/packet_fwd.h>
#include <sstmac/hardware/network/network_message_fwd.h>
#include <sstmac/hardware/interconnect/interconnect_fwd.h>
#include <sstmac/common/stats/stat_spyplot_fwd.h>
#include <sstmac/common/stats/stat_histogram_fwd.h>
#include <sstmac/common/stats/stat_local_int_fwd.h>
#include <sstmac/common/stats/stat_global_int_fwd.h>
#include <sstmac/common/messages/sst_message_fwd.h>

#include <sprockit/debug.h>
#include <sprockit/factories/factory.h>

DeclareDebugSlot(nic);

#define nic_debug(...) \
  debug_printf(sprockit::dbg::nic, "NIC on node %d: %s", \
    int(addr()), sprockit::printf(__VA_ARGS__).c_str())

namespace sstmac {
namespace hw {

/**
 * A networkinterface is a delegate between a node and a server module.
 * This object helps ornament network operations with information about
 * the process (ppid) involved.
 */
class nic :
  public sprockit::factory_type,
  public failable,
  public connectable_subcomponent
{
#if SSTMAC_INTEGRATED_SST_CORE
 public:
  void handle_event(SST::Event* ev);
#endif
 public:
  virtual std::string
  to_string() const = 0;

  virtual ~nic();

  /**
   * Initialize all member variables from the parameters object
   * @param params
   */
  virtual void
  init_factory_params(sprockit::sim_parameters* params);

  /**
   * @return A unique ID for the NIC positions. Opaque typedef to an int.
   */
  node_id
  addr() const {
    return my_addr_;
  }

  /**
   * Set an event handler wrapper encapsulation the parent computational unit.
   * @param nd The compute node attached to the NIC
   */
  virtual void
  set_node(node* nd){
    parent_ = nd;
  }

  /**
   * Delete all static variables associated with this class.
   * This should be registered with the runtime system via need_delete_statics
   */
  static void
  delete_statics();

  /**
   * A final initialization function called for the object after all parameters have been read.
   * Guarantees initialization of all subclass variables.
   */
  virtual void
  finalize_init();

  virtual void
  handle(event *ev) = 0;

  void
  mtl_handle(event* ev);

  event_handler*
  mtl_handler() const {
    return mtl_handler_;
  }

  /**
    Perform the set of operations standard to all NICs.
    This then passes control off to a model-specific #do_send
    function to actually carry out the send
    @param payload The network message to send
  */
  void
  internode_send(network_message* payload);

  /**
    Perform the set of operations standard to all NICs
    for transfers within a node. This function is model-independent,
    unlike #internode_send which must pass control to #do_send.
   * @param payload
   */
  void
  intranode_send(network_message* payload);

  /**
   * @return The injection latency for moving a packet from the NIC to the
   *          first network router (or netlink block, etc)
   */
  virtual timestamp
  injection_latency() const = 0;

  virtual double
  injection_bandwidth() const = 0;

  virtual void
  set_event_parent(event_scheduler* m);


 protected:
  nic(sprockit::factory_type* interconn);

  /**
    Start the message sending and inject it into the network
    This performs all model-specific work
    @param payload The network message to send
  */
  virtual void
  do_send(network_message* payload) = 0;

  void
  send_to_node(network_message* netmsg);

  bool
  negligible_size(int bytes) const {
    return bytes <= negligible_size_;
  }

  /**
   The NIC can either receive an entire message (bypass the byte-transfer layer)
   or it can receive packets.  If an incoming message is a full message (not a packet),
   it gets routed here. Unlike #recv_chunk, this has a default implementation and does not throw.
   @param chunk
   */
  void
  recv_message(message* msg);

 protected:
  node_id my_addr_;

  int negligible_size_;

  interconnect* interconn_;
  node* parent_;

 private:
  stat_spyplot* spy_num_messages_;
  stat_spyplot* spy_bytes_;
  stat_histogram* hist_msg_size_;
  stat_local_int* local_bytes_sent_;
  stat_global_int* global_bytes_sent_;
  event_handler* mtl_handler_;

 private:
  /**
   For messages requiring an NIC ACK to signal that the message
   has injected into the interconnect.  Create an ack and
   send it up to the parent node.
   */
  void
  ack_send(network_message* payload);

  void
  send_to_interconn(network_message* netmsg);

  void record_message(network_message* msg);

};

DeclareFactory1InitParam(nic, sprockit::factory_type*/*interconnect*/);

}
} // end of namespace sstmac.

#endif

