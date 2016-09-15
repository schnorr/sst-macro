#include <sstmac/hardware/simple/simple_nic.h>
#include <sstmac/hardware/network/network_message.h>
#include <sstmac/hardware/node/node.h>
#include <sstmac/common/event_handler.h>
#include <sprockit/util.h>
#include <sprockit/sim_parameters.h>
#include <sstmac/hardware/interconnect/interconnect.h>

#if !SSTMAC_INTEGRATED_SST_CORE

namespace sstmac {
namespace hw {

SpktRegister("simple", nic, simple_nic,
            "implements a nic that models messages via a simple latency/bandwidth injection delay");

simple_nic::simple_nic(sprockit::sim_parameters* params, node* parent) :
  next_free_(0),
  nic(params, parent)
{
  sprockit::sim_parameters* inj_params = params->get_namespace("injection");
  double inj_bw = inj_params->get_bandwidth_param("bandwidth");
  inj_bw_inverse_ = 1.0/inj_bw;
  inj_lat_ = inj_params->get_time_param("latency");
}

void
simple_nic::do_send(network_message* msg)
{
  long num_bytes = msg->byte_length();
  timestamp now_ = now();
  timestamp start_send = now_ > next_free_ ? now_ : next_free_;
  timestamp time_to_inject = inj_lat_ + timestamp(inj_bw_inverse_ * num_bytes);
  //leave the injection latency term to the interconnect
  send_to_injector(start_send, msg);

  next_free_ = start_send + time_to_inject;
  if (msg->needs_ack()) {
    //do whatever you need to do so that this msg decouples all pointers
    network_message* acker = msg->clone_injection_ack();
    send_to_node(next_free_, acker);
  }
}

void
simple_nic::handle(event *ev)
{
  spkt_throw_printf(sprockit::value_error,
    "simple_nic::handle: should never be called - should be routed directly to MTL handler");
}

void
simple_nic::connect(
  sprockit::sim_parameters* params,
  int src_outport,
  int dst_inport,
  connection_type_t ty,
  connectable* mod)
{
}

void
simple_nic::send_to_injector(timestamp t, network_message* msg)
{
  interconn_->immediate_send(parent(), msg, t);
}

void
simple_nic::send_to_node(timestamp t, network_message* msg){
  schedule(t, parent_, msg);
}

}
}

#endif
