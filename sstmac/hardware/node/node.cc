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

#include <sstmac/software/libraries/unblock_event.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/hardware/node/node.h>
#include <sstmac/hardware/nic/nic.h>
#include <sstmac/hardware/memory/memory_model.h>
#include <sstmac/hardware/processor/processor.h>
#include <sstmac/hardware/interconnect/interconnect.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/app.h>
#include <sstmac/software/launch/app_launch.h>
#include <sstmac/software/launch/launcher.h>
#include <sstmac/software/launch/launch_event.h>
#include <sstmac/software/launch/job_launcher.h>
#include <sstmac/common/event_callback.h>
#include <sstmac/common/runtime.h>
#include <sprockit/keyword_registration.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/util.h>
#include <sprockit/output.h>

#if SSTMAC_INTEGRATED_SST_CORE
#include <sstmac/sst_core/integrated_component.h>
#include <sstmac/sst_core/connectable_wrapper.h>
#endif

ImplementFactory(sstmac::hw::node);
RegisterDebugSlot(node)
RegisterNamespaces("os", "memory", "proc");

namespace sstmac {
namespace hw {

std::list<sw::app_launch*> node::launchers_;

using namespace sstmac::sw;

node::node(sprockit::sim_parameters* params,
  uint64_t id, event_manager* mgr)
  : connectable_component(params, id,
  event_loc_id(node_id(params->get_int_param("id"))),
  mgr,
  new_handler(this, &node::handle)),
  params_(params)
{
  my_addr_ = event_location().convert_to_node_id();
  next_outgoing_id_.set_src_node(my_addr_);

  sprockit::sim_parameters* nic_params = params->get_namespace("nic");
  nic_params->add_param_override("id", int(my_addr_));
  nic_ = nic_factory::get_param("model", nic_params, this);

  sprockit::sim_parameters* mem_params = params->get_optional_namespace("memory");
  mem_model_ = memory_model_factory::get_optional_param("model", "logP", mem_params, this);

  sprockit::sim_parameters* proc_params = params->get_optional_namespace("proc");
  proc_ = processor_factory::get_optional_param("processor", "instruction",
          proc_params,
          mem_model_, this);

  nsocket_ = params->get_optional_int_param("nsockets", 1);

  sprockit::sim_parameters* os_params = params->get_optional_namespace("os");
  os_ = new sw::operating_system(os_params, this);

  app_launcher_ = new app_launcher(os_);
  job_launcher_ = job_launcher::static_job_launcher(params, mgr);
}

link_handler*
node::ack_handler(int port) const
{
  return nic_->ack_handler(port);
}

link_handler*
node::payload_handler(int port) const
{
  return nic_->payload_handler(port);
}

void
node::setup()
{
  schedule_launches();
#if SSTMAC_INTEGRATED_SST_CORE
  event_scheduler::setup();
#endif
}

void
node::init(unsigned int phase)
{
#if SSTMAC_INTEGRATED_SST_CORE
  event_scheduler::init(phase);
#endif
  if (phase == 0){
    build_launchers(params_);
  }
}

node::~node()
{
  if (os_) delete os_;
  if (mem_model_) delete mem_model_;
  if (proc_) delete proc_;
  //JJW 03/09/2015 - node does not own NIC
  //if (nic_) delete nic_;
}

void
node::connect_output(sprockit::sim_parameters* params,
  int src_outport, int dst_inport,
  event_handler* mod)
{
  //forward connection to nic
  nic_->connect_output(params, src_outport, dst_inport, mod);
}

void
node::connect_input(sprockit::sim_parameters* params,
  int src_outport, int dst_inport,
  event_handler* mod)
{
  //forward connection to nic
  nic_->connect_input(params, src_outport, dst_inport, mod);
}

void
node::execute(ami::SERVICE_FUNC func, event* data)
{
  spkt_throw(sprockit::unimplemented_error,
             "node does not implement asynchronous services - choose new node model");
}

void
node::build_launchers(sprockit::sim_parameters* params)
{
  if (!launchers_.empty()) return;

  bool keep_going = true;
  int aid = 1;
  while (keep_going || aid < 10){
    app_launch* appman = app_launch::static_app_launch(aid, params);
    if (appman){
      launchers_.push_back(appman);
      keep_going = true;
    } else {
      keep_going = false;
    }
    ++aid;
  }
}

std::string
node::to_string() const
{
  return sprockit::printf("node(%d)", int(my_addr_));
}

void
node::job_launch(app_launch* appman)
{
  job_launcher_->handle_new_launch_request(appman, this);
}

void
node::schedule_launches()
{
  for (app_launch* appman : launchers_){
    schedule(appman->time(), new_callback(this, &node::job_launch, appman));
  }
}

void
node::handle(event* ev)
{
  if (failed()){
    //do nothing - I failed
  } else {
    node_debug("forwarding event %s to OS",
               sprockit::to_string(ev).c_str());
    os_->handle_event(ev);
  }
}

void
node::fail_stop()
{
  fail();
  nic_->fail();
  cancel_all_messages();
}

void
node::send_to_nic(network_message* netmsg)
{
  node_debug("sending to %d", int(netmsg->toaddr()));
  netmsg->set_net_id(allocate_unique_id());
  netmsg->put_on_wire();

  if (netmsg->toaddr() == my_addr_){
    nic_->intranode_send(netmsg);
  } else {
    nic_->internode_send(netmsg);
  }
}

}
} // end of namespace sstmac


