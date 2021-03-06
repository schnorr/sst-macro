#include "patinkin.h"

namespace sstmac {
namespace tutorial {

SpktRegister("patinkin", actor, mandy_patinkin,
            "He's on one of those shows now... NCIS? CSI?");

void
mandy_patinkin::init_factory_params(sprockit::sim_parameters* params)
{
  sword_hand_ = params->get_param("sword_hand");

  if (sword_hand_ == "left") {
    spkt_throw(sprockit::value_error,
              "I am not left handed!");
  }
  else if (sword_hand_ != "right") {
    spkt_throw_printf(sprockit::value_error,
                     "Invalid hand specified: %s",
                     sword_hand_.c_str());
  }

  actor::init_factory_params(params);
}

void
mandy_patinkin::act()
{
  std::cout <<
            "Hello. My name is Inigo Montoya. You killed my father. Prepare to die!"
            << std::endl;
}


}
}


