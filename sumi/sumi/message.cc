#include <sumi/message.h>

namespace sumi {

const int message::ack_size = 16;
const int message::header_size = 64;

bool
message::is_nic_ack() const
{
  switch(payload_type_)
  {
  case rdma_put_ack:
  case rdma_get_ack:
  case eager_payload_ack:
    return true;
  default:
    return false;
  }
}

message*
message::clone_ack() const
{
  message* ack = clone_msg();
  switch (payload_type()){
    case message::rdma_get:
      ack->set_payload_type(message::rdma_get_ack);
      break;
    case message::rdma_put:
      ack->set_payload_type(message::rdma_put_ack);
      break;
    default:
      spkt_throw_printf(sprockit::value_error,
        "message::clone_ack: invalid payload type %s",
        message::tostr(payload_type()));
  }
  return ack;
}

void
message::reverse()
{
  int tmp = sender_;
  sender_ = recver_;
  recver_ = tmp;
}

message*
message::clone() const
{
  message* cln = new message;
  clone_into(cln);
  return cln;
}

std::string
message::to_string() const
{
  return sprockit::printf("message %s %s %d->%d",
            tostr(payload_type_), tostr(class_),
            sender_, recver_);
}

#define enumcase(x) case x: return #x
const char*
message::tostr(payload_type_t ty)
{
  switch(ty) {
    enumcase(header);
    enumcase(eager_payload);
    enumcase(eager_payload_ack);
    enumcase(rdma_put);
    enumcase(rdma_get);
    enumcase(rdma_put_ack);
    enumcase(rdma_get_ack);
    enumcase(rdma_get_nack);
    enumcase(nvram_get);
    enumcase(failure);
    enumcase(none);
    enumcase(software_ack);
  }
  spkt_throw_printf(sprockit::value_error,
    "message::tostr: invalid payload type %d", ty);
}

const char*
message::tostr(class_t ty)
{
  switch(ty)
  {
    enumcase(pt2pt);
    enumcase(unexpected);
    enumcase(collective);
    enumcase(collective_done);
    enumcase(ping);
    enumcase(terminate);
    enumcase(no_class);
    enumcase(fake);
  }
  spkt_throw_printf(sprockit::value_error,
    "message::tostr: invalid message type %d", ty);
}

void
message::clone_into(message* cln) const
{
  cln->payload_type_ = payload_type_;
  cln->class_ = class_;
  cln->sender_ = sender_;
  cln->recver_ = recver_;
}

void
message::buffer_send(public_buffer& buf, long num_bytes)
{
  void* new_buf = new char[num_bytes];
  void* old_buf = buf.ptr;
  ::memcpy(new_buf, old_buf, num_bytes);
  buf.ptr = new_buf;
}

void
message::move_local_to_remote()
{
  if (local_buffer_.ptr){ //might be null
    ::memcpy(remote_buffer_.ptr, local_buffer_.ptr, num_bytes_);
    delete[] (char*) local_buffer_.ptr;
    local_buffer_.ptr = 0;
  }
}

void
message::move_remote_to_local()
{
  if (remote_buffer_.ptr){
    ::memcpy(local_buffer_.ptr, remote_buffer_.ptr, num_bytes_);
    delete[] (char*) remote_buffer_.ptr;
    remote_buffer_.ptr = 0;
  }
}

void
message::buffer_send()
{
  //nothing to do
  if (local_buffer_.ptr == nullptr)
    return;

  switch(payload_type_){
    case rdma_get:
      buffer_send(remote_buffer_, num_bytes_);
      break;
    case rdma_put:
      buffer_send(local_buffer_, num_bytes_);
      break;
    case eager_payload:
      buffer_send(local_buffer_, num_bytes_);
      break;
    default:
      //do nothing
      break;
  }
}

void
message::serialize_order(sumi::serializer &ser)
{
  ser & sender_;
  ser & recver_;
  ser & class_;
  ser & payload_type_;
  ser & num_bytes_;
  ser & transaction_id_;
  ser & needs_send_ack_;
  ser & needs_recv_ack_;
  ser & local_buffer_;
  ser & remote_buffer_;
  switch (payload_type_)
  {
    case rdma_get:
      if (remote_buffer_.ptr){
        sumi::array(remote_buffer_.ptr, num_bytes_);
      }
      break;
    case rdma_put:
    case eager_payload:
      if (local_buffer_.ptr){
        sumi::array(local_buffer_.ptr, num_bytes_);
      }
      break;
    default:
      break;
  }
}

}

