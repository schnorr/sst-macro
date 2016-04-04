#ifndef sstmac_hardware_packet_h
#define sstmac_hardware_packet_h

#include <sstmac/common/sst_event.h>
#include <sstmac/common/messages/sst_message_fwd.h>
#include <sprockit/metadata_bits.h>
#include <sprockit/expandable_enum.h>

namespace sstmac {
namespace hw {

class packet :
  public event
{
 public:
  declare_expandable_enum(field);

 public:
  message*
  orig() const {
    return orig_;
  }

  virtual std::string
  to_string() const {
    return "packet";
  }

  bool
  is_tail() const {
    return orig_;
  }

  long
  byte_length() const {
    return num_bytes_;
  }

  uint64_t
  unique_id() const {
    return unique_id_;
  }

  void
  add_delay_us(double delay_us){
    cumulative_delay_us_ += delay_us;
  }

  double
  delay_us() const {
    return cumulative_delay_us_;
  }

  virtual void
  serialize_order(sprockit::serializer& ser);

  virtual bool
  is_packet() const {
    return true;
  }

  virtual node_id
  toaddr() const = 0;

  virtual node_id
  fromaddr() const = 0;

 protected:
  packet() : orig_(0) {}

  packet(
    message* orig,
    long num_bytes,
    long byte_offset);

 protected:
  message* orig_;

  long num_bytes_;

  long byte_offset_;

  uint64_t unique_id_;

  double cumulative_delay_us_;

  template <class T>
  T&
  get_field(field name){
    uint64_t* ptr = &fields_[name];
    return *reinterpret_cast<T*>(ptr);
  }

 private:
  std::map<field, uint64_t> fields_;

};

implement_enum_functions(packet::field)


}
}

#endif

