#ifndef SSTMAC_HARDWARE_NETWORK_ROUTING_ROUTER_MINAD_ROUTER_H
#define SSTMAC_HARDWARE_NETWORK_ROUTING_ROUTER_MINAD_ROUTER_H

#include <sstmac/hardware/router/minimal_routing.h>

namespace sstmac {
namespace hw {

class minimal_adaptive_router :
  public minimal_router
{

 public:
  minimal_adaptive_router() :
    minimal_router(routing::minimal_adaptive){}

  virtual std::string
  to_string() const {
    return "min_ad";
  }

  virtual void
  route(packet* pkt);

  virtual void
  finalize_init();

};


}
}


#endif // MINAD_ROUTER_H

