
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

#include <sprockit/spkt_config.h>
#include <sprockit/spkt_string.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/basic_string_tokenizer.h>
#include <sprockit/units.h>
#include <sprockit/driver_util.h>
#include <sprockit/errors.h>
#include <sprockit/keyword_registration.h>
#include <sprockit/regexp.h>
#include <sprockit/fileio.h>
#include <sprockit/output.h>
#include <cstring>

RegisterDebugSlot(params,
    "print all the details of the initial reading parameters from the input file"
    ", all usage of parameters within the application"
    ", and also any internal overriding or automatic parameter generation");
RegisterDebugSlot(read_params,
  "print all the details of reading and using paramters within the application");
RegisterDebugSlot(write_params,
  "print all the details of writing or overriding parameters within the application"
  "  - this includes the initial reading of parameters from the input file");

namespace sprockit {

double 
get_quantity_with_units(const char* value, const char* key)
{
  bool error;
  double t = get_timestamp(value, error);
  if (!error) return t;
  double bw = get_bandwidth(value, error);
  if (!error) return bw;
  double freq = get_frequency(value, error);
  if (!error) return freq;
  double bytes = byte_length(value, error);
  if (!error) return bytes;


  const char* begin = value;
  char* end = const_cast<char*>(begin);
  double ret = ::strtod(begin, &end);

  while (*end==' ') ++end;
  int size = (int)((size_t)end - (size_t)begin);
  if (begin == end || size != ::strlen(value)) {
    spkt_throw_printf(input_error,
        "sim_parameters::get_quantity: param %s with value %s is not formatted as a double with units (Hz,GB/s,ns,KB)",
        key, value);
  }
  return ret;
}

sim_parameters* sim_parameters::empty_ns_params_ = new sim_parameters;

double
get_freq_from_str(const char* val, const char* key)
{
  bool errorflag = false;
  double ret = get_frequency(val, errorflag);
  if (errorflag) {
    spkt_abort_printf("improperly formatted frequency (%s) for parameter %s", val, key);
  }
  return ret;
}

long
get_byte_length_from_str(const char* val, const char* key)
{
  bool errorflag = false;
  double ret = byte_length(val, errorflag);
  if (errorflag) {
    spkt_abort_printf("improperly formatted byte length (%s) for parameter %s", val, key);
  }
  return ret;
}

double
get_time_from_str(const char* val, const char* key)
{
  bool errorflag = false;
  double ret = get_timestamp(val, errorflag);
  if (errorflag) {
    spkt_abort_printf("improperly formatted time (%s) for parameter %s", val, key);
  }
  return ret;
}

double
get_bandwidth_from_str(const char* val, const char* key)
{
  bool errorflag = false;
  double ret = get_bandwidth(val, errorflag);
  if (errorflag) {
    spkt_abort_printf("improperly formatted bandwidth (%s) for parameter %s", val, key);
  }
  return ret;
}

param_assign::operator int() const
{
  return get_quantity_with_units(param_.c_str(), key_.c_str());
}

param_assign::operator double() const
{
  return get_quantity_with_units(param_.c_str(), key_.c_str());
}

void
param_assign::operator=(int x)
{
  param_ = sprockit::printf("%d", x); 
}

void
param_assign::operator=(double x)
{
  param_ = sprockit::printf("%f", x); 
}

double
param_assign::getBandwidth() const 
{
  return get_bandwidth_from_str(param_.c_str(), key_.c_str()); 
}

double
param_assign::getFrequency() const 
{
  return get_freq_from_str(param_.c_str(), key_.c_str()); 
}

long
param_assign::getByteLength() const
{
  return get_byte_length_from_str(param_.c_str(), key_.c_str());
}

double
param_assign::getTime() const 
{
  return get_time_from_str(param_.c_str(), key_.c_str());
}

const std::string&
param_assign::set(const char* str)
{
  param_ = str;
  return param_;
}

const std::string&
param_assign::set(const std::string& str)
{
  param_ = str;
  return param_;
}

const std::string&
param_assign::setValue(double x, const char* units)
{
  param_ = sprockit::printf("%f%s", x, units);
  return param_;
}

const std::string&
param_assign::setTime(double x, const char* units)
{
  return setValue(x, units);
}

const std::string&
param_assign::setBandwidth(double x, const char* units) 
{
  return setValue(x, units);
}

const std::string&
param_assign::setFrequency(double x, const char* units) 
{
  return setValue(x, units);
}

const std::string&
param_assign::setByteLength(long x, const char* units)
{
  return setValue(x, units);
}

sim_parameters::sim_parameters() :
  parent_(0)
{
}

sim_parameters::sim_parameters(const key_value_map& p) :
  params_(p),
  parent_(0),
  namespace_("global")
{
}

sim_parameters::sim_parameters(const std::string& filename) :
  parent_(0),
  namespace_("global")
{
  //don't fail, but don't overwrite anything
  //parameters from file get lowest priority
  parse_file(filename, false, false);
}

sim_parameters::~sim_parameters()
{
  params_.clear();
  std::map<std::string, sim_parameters*>::const_iterator it, end = subspaces_.end();
  for (it=subspaces_.begin(); it != end; ++it){
    sim_parameters* subspace = it->second;
    delete subspace;
  }
}

bool
sim_parameters::has_namespace(const std::string &ns) const
{
  return local_has_namespace(ns);
}

sim_parameters*
sim_parameters::get_namespace(const std::string &ns) 
{
  KeywordRegistration::validate_namespace(ns);
  std::map<std::string, sim_parameters*>::const_iterator it = subspaces_.find(ns);
  if (it == subspaces_.end()){
    pretty_print_params();
    spkt_throw_printf(input_error,
        "cannot enter namespace %s, does not exist inside namespace %s",
        ns.c_str(), namespace_.c_str());
  }
  return it->second;
}

void
sim_parameters::copy_param(const std::string &oldname, const std::string &newname)
{
  add_param_override(newname, get_param(oldname));
}

void
sim_parameters::copy_optional_param(const std::string &oldname, const std::string &newname)
{
  if (has_param(oldname))
    add_param_override(newname, get_param(oldname));
}

void
sim_parameters::add_param_override(const std::string &key, double val, const char* units)
{
  add_param_override(key, printf("%20.8f%s", val, units));
}


void
sim_parameters::add_param_override(const std::string &key, double val)
{
  add_param_override(key, printf("%20.8f", val));
}

void
sim_parameters::add_param_override(const std::string &key, int val)
{
  add_param_override(key, printf("%d", val));
}

void
sim_parameters::get_vector_param(const std::string& key,
                                 std::vector<std::string>& vals)
{
  std::deque<std::string> tok;
  std::string space = " ";
  std::string param_value_str = get_param(key);
  pst::BasicStringTokenizer::tokenize(param_value_str, tok, space);
  std::deque<std::string>::const_iterator it, end = tok.end();
  for (it = tok.begin(); it != end; ++it) {
    std::string core = *it;
    if (core.size() > 0) {
      vals.push_back(core);
    }
  }
}

std::string
sim_parameters::deprecated_param(const std::string &key)
{
  return get_param(key);
}

std::string
sim_parameters::deprecated_optional_param(const std::string &key, const std::string &def)
{
  return get_optional_param(key, def);
}

std::string
sim_parameters::get_optional_param(const std::string &key, const std::string &def)
{
  if (has_param(key)) {
    return get_param(key);
  } else if (parent_){
    return parent_->get_optional_param(key,def);
  } else {
   return def;
  }
}

sim_parameters*
sim_parameters::get_optional_namespace(const std::string& ns)
{
  sim_parameters*& params = subspaces_[ns];
  if (params == 0){
   params = new sim_parameters;
   if (namespace_ == "global"){
     params->set_namespace(ns);
   } else {
     params->set_namespace(sprockit::printf("%s.%s",
                  namespace_.c_str(), ns.c_str()));
   }
   params->set_parent(this);
  }
  return params;
}

long
sim_parameters::get_long_param(const std::string &key)
{
  std::string v = get_param(key);
  const char* begin = v.c_str();
  char* end = const_cast<char*>(begin);
  long ret = ::strtol(begin, &end, 0);
  if (begin == end) {
    spkt_abort_printf("sim_parameters::get_long_param: param %s with value %s is not formatted as an integer",
                     key.c_str(), v.c_str());
  }
  return ret;
}

long
sim_parameters::deprecated_long_param(const std::string &key)
{
  return get_long_param(key);
}

long
sim_parameters::deprecated_optional_long_param(const std::string &key, long def)
{
  return get_optional_long_param(key, def);
}

long
sim_parameters::get_optional_long_param(const std::string &key, long def)
{
  if (has_param(key)) {
    return get_long_param(key);
  }
  return def;
}


double
sim_parameters::get_time_param(const std::string& key)
{
  return get_time_from_str(get_param(key).c_str(), key.c_str());

}

double
sim_parameters::deprecated_time_param(const std::string &key)
{
  return get_time_param(key);
}

double
sim_parameters::get_optional_time_param(const std::string &key,
                                        double def)
{
  if (has_param(key)) {
    return get_time_param(key);
  }
  return def;
}

double
sim_parameters::deprecated_optional_time_param(const std::string &key,
    double def)
{
  return get_optional_time_param(key, def);
}

double
sim_parameters::reread_double_param(const std::string &key)
{
  return get_double_param(key);
}

double
sim_parameters::reread_optional_double_param(const std::string &key,
    double def)
{
  return get_optional_double_param(key, def);
}


double
sim_parameters::get_quantity(const std::string& key)
{
  bool error = false;
  std::string value = get_param(key);
  return get_quantity_with_units(value.c_str(), key.c_str());
}

double
sim_parameters::get_optional_quantity(const std::string &key, double def)
{
  if (has_param(key)){
    return get_quantity(key);
  } else {
    return def;
  }
}

double
sim_parameters::get_double_param(const std::string& key)
{
  std::string v = get_param(key);
  const char* begin = v.c_str();
  char* end = const_cast<char*>(begin);
  double ret = ::strtod(begin, &end);
  if (begin == end) {
    spkt_abort_printf("sim_parameters::get_double_param: param %s with value %s is not formatted as a double",
                     key.c_str(), v.c_str());
  }
  return ret;
}

double
sim_parameters::get_optional_double_param(const std::string &key, double def)
{
  if (has_param(key)) {
    return get_double_param(key);
  }

  return def;
}

double
sim_parameters::deprecated_double_param(const std::string &key)
{
  return get_double_param(key);
}

double
sim_parameters::deprecated_optional_double_param(const std::string &key, double def)
{
  return get_optional_double_param(key, def);
}


int
sim_parameters::deprecated_optional_int_param(const std::string &key, int def)
{
  return get_optional_int_param(key, def);
}

int
sim_parameters::get_optional_int_param(const std::string &key, int def)
{
  if (has_param(key)) {
    return get_int_param(key);
  }
  return def;
}

int
sim_parameters::get_int_param(const std::string& key)
{
  std::string v = get_param(key);
  const char* begin = v.c_str();
  char* end = const_cast<char*>(begin);
  long ret = ::strtol(begin, &end, 0);
  if (begin == end) {
    spkt_abort_printf("sim_parameters::get_int_param: param %s with value %s is not formatted as an integer",
                     key.c_str(), v.c_str());
  }
  return ret;
}

int
sim_parameters::reread_optional_int_param(const std::string &key, int def)
{
  return get_optional_int_param(key,def);
}

int
sim_parameters::reread_int_param(const std::string &key)
{
  return get_int_param(key);
}

int
sim_parameters::deprecated_int_param(const std::string &key)
{
  return get_int_param(key);
}

bool
sim_parameters::deprecated_optional_bool_param(const std::string &key, bool def)
{
  return get_optional_bool_param(key, def);
}

bool
sim_parameters::get_optional_bool_param(const std::string &key, int def)
{
  if (has_param(key)) {
    return get_bool_param(key);
  }
  return def;
}

bool
sim_parameters::reread_optional_bool_param(const std::string &key, bool def)
{
  return get_optional_bool_param(key,def);
}

bool
sim_parameters::reread_bool_param(const std::string &key)
{
  return get_bool_param(key);
}

bool
sim_parameters::deprecated_bool_param(const std::string &key)
{
  return get_bool_param(key);
}

bool
sim_parameters::get_bool_param(const std::string &key)
{
  std::string v = get_param(key);
  if (v == "true" || v == "1") {
    return true;
  }
  else if (v != "false" && v != "0") {
    spkt_abort_printf("sim_parameters::get_bool_param: param %s with value %s is not formatted as a proper boolean",
                     key.c_str(), v.c_str());
  }
  return false;
}

void
sim_parameters::get_vector_param(const std::string& key, std::vector<double>& vals)
{
  bool errorflag = false;
  std::string param_value_str = get_param(key);
  std::stringstream sstr(param_value_str);
  vals.reserve(10); //optimistically assume not that big
  while (sstr && sstr.tellg() != -1){
    double val;
    sstr >> val;
    vals.push_back(val);
  }
}

void
sim_parameters::get_vector_param(const std::string& key, std::vector<int>& vals)
{
  bool errorflag = false;
  std::string param_value_str = get_param(key);
  get_intvec(param_value_str.c_str(), errorflag, vals);
  if (errorflag) {
    spkt_abort_printf("improperly formatted integer vector (%s) for parameter %s",
                     param_value_str.c_str(), key.c_str());
  }
}

double
sim_parameters::get_freq_param(const std::string &key)
{
  bool errorflag = false;
  std::string param_value_str = get_param(key);
  return get_freq_from_str(param_value_str.c_str(), key.c_str());
}

double
sim_parameters::deprecated_freq_param(const std::string &key)
{
  return get_freq_param(key);
}

double
sim_parameters::deprecated_optional_freq_param(const std::string &key, double def)
{
  return get_optional_freq_param(key, def);
}

double
sim_parameters::get_optional_freq_param(const std::string &key, double def)
{
  bool errorflag = false;
  std::string freq_str = printf("%eHz", def);
  std::string param_value_str = get_optional_param(key, freq_str);
  return get_freq_from_str(param_value_str.c_str(), key.c_str());
}

long
sim_parameters::get_byte_length_param(const std::string &key)
{
  bool errorflag = false;
  std::string param_value_str = get_param(key);
  return get_byte_length_from_str(param_value_str.c_str(), key.c_str());
}

long
sim_parameters::deprecated_byte_length_param(const std::string &key)
{
  return get_byte_length_param(key);
}

long
sim_parameters::deprecated_optional_byte_length_param(const std::string &key, long def)
{
  return get_optional_byte_length_param(key, def);
}

long
sim_parameters::get_optional_byte_length_param(const std::string& key, long length)
{
  std::string length_str = printf("%ldB", length);
  std::string param_value_str = get_optional_param(key, length_str);
  return get_byte_length_from_str(param_value_str.c_str(), key.c_str());
}

double
sim_parameters::get_bandwidth_param(const std::string &key)
{
  std::string param_value_str = get_param(key);
  return get_bandwidth_from_str(param_value_str.c_str(), key.c_str());
}

double
sim_parameters::reread_bandwidth_param(const std::string &key)
{
  return get_bandwidth_param(key);
}

double
sim_parameters::reread_optional_bandwidth_param(const std::string &key, double def)
{
  return get_optional_bandwidth_param(key, def);
}

double
sim_parameters::deprecated_bandwidth_param(const std::string &key)
{
  return get_bandwidth_param(key);
}

double
sim_parameters::deprecated_optional_bandwidth_param(const std::string &key, double def)
{
  return get_optional_bandwidth_param(key, def);
}

double
sim_parameters::get_optional_bandwidth_param(const std::string &key, const std::string& def)
{
  bool errorflag = false;
  std::string param_value_str = get_optional_param(key, def);
  double val = get_bandwidth(param_value_str.c_str(), errorflag);
  if (errorflag) {
    spkt_abort_printf("improperly formatted bandwidth (%s) for parameter %s",
                     param_value_str.c_str(), key.c_str());
  }
  return val;
}

double
sim_parameters::get_optional_bandwidth_param(const std::string &key, double def)
{
  std::string bwstr = printf("%ebytes/sec", def);
  return get_optional_bandwidth_param(key, bwstr);
}

void
sim_parameters::print_params(
    const key_value_map &pmap,
    std::ostream &os,
    bool pretty_print,
    std::list<std::string>& namespaces) const
{
  std::stringstream sstr;
  if (pretty_print){ //just tab to the depth
    std::stringstream sstr;
    int depth = namespaces.size();
    for (int i=0; i < depth; ++i){
      sstr << "  ";
    }
  } else {
    std::list<std::string>::iterator it, end = namespaces.end();
    for (it=namespaces.begin(); it != end; ++it){
      sstr << *it << ".";
    }
  }
  std::string prefix = sstr.str();

  key_value_map::const_iterator it, end = pmap.end();
  for (it = pmap.begin() ; it != end; ++it) {
    std::string key = it->first;
    const parameter_entry& entry = it->second;
    os << prefix << key;
    if (pretty_print){
      for (int i=key.size(); i < 25; ++i){
        os << " ";
      }
    }
    os << " = " << entry.value << "\n";
  }

  std::map<std::string, sim_parameters*>::const_iterator nsit, nsend = subspaces_.end();
  for (nsit=subspaces_.begin(); nsit != nsend; ++nsit){
    std::string ns = nsit->first;
    sim_parameters* subspace = nsit->second;
    namespaces.push_back(ns);
    if (pretty_print){ //print the next namespace as a heading
        os << prefix << subspace->namespace_ << "\n";
    } //else explicitly print the full namespace with each param
    subspace->print_params(os, pretty_print, namespaces);
    namespaces.pop_back();
  }
}

void
sim_parameters::try_to_parse(
  const std::string& fname,
  bool fail_on_existing,
  bool override_existing)
{
  std::string inc_file = fname;
  std::string dir = "";
  std::string f_firstchar = inc_file.substr(0, 1);
  if (f_firstchar == "/") {
    //do nothing - this is an absolute path
  }
  else {
    size_t pos = fname.find_last_of('/');
    if (pos != std::string::npos) {
      dir = fname.substr(0, pos + 1);
    }
  }
  inc_file = trim_str(inc_file);
  try {
    parse_file(dir + inc_file, fail_on_existing, override_existing);
  }
  catch (io_error& e) {
    parse_file(inc_file, fail_on_existing, override_existing);
  }
}

sim_parameters*
sim_parameters::get_scope_and_key(const std::string& key, std::string& final_key)
{
  sim_parameters* scope = this;
  final_key = key;
  size_t ns_pos = final_key.find(".");
  while (ns_pos != std::string::npos) {
    std::string ns = final_key.substr(0, ns_pos);
    final_key  = final_key.substr(ns_pos+1);
    scope = scope->get_optional_namespace(ns);
    ns_pos = final_key.find(".");
  }
  return scope;
}

void
sim_parameters::parse_keyval(
  const std::string& key,
  const std::string& value,
  bool fail_on_existing,
  bool override_existing,
  bool mark_as_read)
{
  std::string final_key;
  sim_parameters* scope = get_scope_and_key(key, final_key);
  scope->do_add_param(final_key, value,
    fail_on_existing, override_existing, mark_as_read);
}

void
sim_parameters::split_line(const std::string& line, std::pair<std::string, std::string>& p)
{
  std::string lhs = line.substr(0, line.find("="));
  std::string rhs = line.substr(line.find("=") + 1);
  p.first = sprockit::trim_str(lhs);
  p.second = sprockit::trim_str(rhs);
}

void
sim_parameters::parse_line(const std::string& line,
  bool fail_on_existing, bool override_existing)
{
  std::pair<std::string, std::string> keyval;
  sim_parameters::split_line(line, keyval);
  parse_keyval(keyval.first, keyval.second,
   fail_on_existing,
   override_existing,
   false/*do not mark as read*/);
}

void
param_bcaster::bcast_string(std::string& str, int me, int root)
{
  if (me == root){
    int size = str.size();
    bcast(&size, sizeof(int), me, root);
    char* buf = const_cast<char*>(str.c_str());
    bcast(buf, size, me, root);
  } else {
    int size;
    bcast(&size, sizeof(int), me, root);
    str.resize(size);
    char* buf = const_cast<char*>(str.c_str());
    bcast(buf, size, me, root);
  }
}

void
sim_parameters::parallel_build_params(sprockit::sim_parameters* params, int me, int nproc, const std::string& filename, param_bcaster *bcaster)
{
  bool fail_on_existing = false;
  bool overwrite_existing = true;
  try {
    int root = 0;
    if (me == 0){
      //I don't want all processes hitting the network and reading the file
      //Proc 0 reads it and then broadcasts
      params->parse_file(filename, fail_on_existing, overwrite_existing);
      if (nproc > 1){
        //this is a bit more complicated than bcast_file_stream
        //in parsing the main file, root might open more files
        //thus read in all possible params chasing down all include files
        //then build the full text of all params
        std::stringstream sstr;
        params->print_params(sstr);
        std::string all_text = sstr.str();
        bcaster->bcast_string(all_text, me, root);
      }
    } else {
      std::string param_text;
      bcaster->bcast_string(param_text, me, root);
      std::stringstream sstr(param_text);
      params->parse_stream(sstr, fail_on_existing, overwrite_existing);
    }
  } catch (const std::exception &e) {
    std::cout.flush();
    std::cerr.flush();
    std::cerr << "Caught exception while setting up simulation:\n"
              << e.what() << "\n";
    throw;
  }
}

void
sim_parameters::parse_stream(std::istream& in,
  bool fail_on_existing, bool override_existing)
{
  std::string line;
  while (in.good()) {
    std::getline(in, line);
    line = trim_str(line);

    if (line[0] == '#') {
      //a comment
      continue;
    }
    else if (line.find("set var ") != std::string::npos) {
      line = line.substr(8);
      std::pair<std::string, std::string> keyval;
      sim_parameters::split_line(line, keyval);
      variables_[keyval.first] = keyval.second;
    }
    else if (line.find("=") != std::string::npos) {
      //an assignment
      parse_line(line, fail_on_existing, override_existing);
    }
    else if (line.find("include") != std::string::npos) {
      //an include line
      std::string included_file = trim_str(line.substr(7));
      try_to_parse(included_file, fail_on_existing, override_existing);
    }
    else if (line.find("unset") != std::string::npos) {
      std::string key;
      sim_parameters* scope = get_scope_and_key(trim_str(line.substr(5)), key);
      scope->remove_param(key);
    }
    else if (line.size() == 0) {
      //empty
    }
    else {
      spkt_throw_printf(input_error, "invalid input file line of size %d:\n%s---", line.size(), line.c_str());
    }
  }
}

void
sim_parameters::parse_file(
  const std::string& input_fname,
  bool fail_on_existing,
  bool override_existing)
{
  std::string fname = trim_str(input_fname);

  std::ifstream in;
  SpktFileIO::open_file(in, fname);

  if (in.is_open()) {
    parse_stream(in, fail_on_existing, override_existing);
  } else {
    SpktFileIO::not_found(fname);
  }
}

void
sim_parameters::remove_param(const std::string & key)
{
  params_.erase(key);
}

bool
sim_parameters::print_unread_params(std::ostream &os) const
{
  bool have_unread = false;
  key_value_map::const_iterator it, end = params_.end();
  {for (it=params_.begin(); it != end; ++it){
    const parameter_entry& entry = it->second;
    if (!entry.read){
      os << sprockit::printf("Unused in namespace %30s: %s = %s\n",
               namespace_.c_str(), it->first.c_str(), entry.value.c_str());
      have_unread = true;
    }
  }}

  {std::map<std::string, sim_parameters*>::const_iterator it, end = subspaces_.end();
  for (it=subspaces_.begin(); it != end; ++it){
    sim_parameters* params = it->second;
    have_unread = have_unread || params->print_unread_params(os);
  }}

  return have_unread;
}

void
sim_parameters::throw_key_error(const std::string& key) const
{
  std::cerr << "Parameters given in namespace: " << std::endl;
  print_params(std::cerr);
  spkt_throw_printf(sprockit::value_error,
           "sim_parameters: could not find parameter %s in namespace %s",
          key.c_str(), namespace_.c_str());
}

bool
sim_parameters::get_scoped_param(std::string& inout,
                          const std::string& key)
{
  key_value_map::iterator it = params_.find(key);
  if (it == params_.end()){
    return false;
  }
  parameter_entry& entry = it->second;
  entry.read = true;
  inout = entry.value;
  return true;
}

bool
sim_parameters::get_param(std::string& inout, const std::string& key)
{
  bool found = get_scoped_param(inout, key);
  if (!found && parent_){
    return parent_->get_param(inout, key);
  } else {
    return found;
  }
}

std::string
sim_parameters::get_param(const std::string& key, bool throw_on_error)
{
  debug_printf(dbg::params | dbg::read_params,
    "sim_parameters: getting key %s\n",
    key.c_str());

  std::string match;
  bool found = get_param(match, key);
  if (!found && throw_on_error){
    throw_key_error(key);
  }

  return match;
}

std::string
sim_parameters::get_scoped_param(const std::string& key, bool throw_on_error)
{
  std::string ret;
  bool found = get_scoped_param(ret, key);
  if (!found && throw_on_error){
    throw_key_error(key);
  }
  return ret;
}

sim_parameters*
sim_parameters::top_parent() 
{
  if (parent_){
    return parent_->top_parent();
  }
  return this;
}

bool
sim_parameters::has_scoped_param(const std::string& key) const
{
  return params_.find(key) != params_.end();
}

bool
sim_parameters::has_param(const std::string& key) const
{
  bool has_here = has_scoped_param(key);
  if (!has_here && parent_){
    return parent_->has_param(key);
  } else {
    return has_here;
  }
}

void
sim_parameters::do_add_param(
  const std::string& key,
  const std::string& val,
  bool fail_on_existing,
  bool override_existing,
  bool mark_as_read)
{
  if (val.c_str()[0] == '$'){
    std::map<std::string, std::string>::const_iterator it = variables_.find(val.substr(1));
    if (it == variables_.end()){
      spkt_throw_printf(input_error,
        "unknown variable name %s", val.c_str());
    }
    do_add_param(key, it->second,
      fail_on_existing, override_existing, mark_as_read);
    return;
  }

  debug_printf(dbg::params, //| dbg::write_params,
    "sim_parameters: setting key %s to value %s\n",
    key.c_str(), val.c_str());

  KeywordRegistration::validate_keyword(key,val);

  key_value_map::iterator it = params_.find(key);

  if (it != params_.end()){
    if (fail_on_existing){
      spkt_throw_printf(sprockit::value_error,
      "sim_parameters::add_param - key already in params: %s", key.c_str());
    } else if (override_existing){
      parameter_entry& entry = it->second;
      entry.value = val;
      entry.read = mark_as_read;
    } else {
      //do nothing - don't override and don't fail
    }
  } else {
    parameter_entry entry;
    entry.value = val;
    entry.read = mark_as_read;
    params_.insert(it, std::make_pair(key, entry));
  }
}

param_assign
sim_parameters::operator[](const std::string& key)
{
  return param_assign(params_[key].value, key);
}

void
sim_parameters::add_param(
  const std::string &key,
  const std::string &val)
{
  parse_keyval(key, val,
   true/*fail on existing*/,
   false/*do not override existing*/,
   false/*do not mark as read*/);
}

void
sim_parameters::add_param_override(
  const std::string& key,
  const std::string& val)
{
  parse_keyval(key, val,
   false/*do not fail on existing*/,
   true/*override existing*/,
   true/*mark as read*/);
}

void
sim_parameters::combine_into(sim_parameters* sp,
                             bool fail_on_existing,
                             bool override_existing,
                             bool mark_as_read)
{
  {key_value_map::iterator it, end = params_.end();
  for (it=params_.begin(); it != end; ++it){
    const std::string& key = it->first;
    const parameter_entry& value = it->second;
    sp->parse_keyval(key, value.value,
                 fail_on_existing, override_existing, mark_as_read);
  }}

  {std::map<std::string, sim_parameters*>::iterator it, end = subspaces_.end();
  for (it=subspaces_.begin(); it != end; ++it){
    std::string name = it->first;
    sim_parameters* my_subspace = it->second;
    sim_parameters* his_subspace = sp->get_optional_namespace(name);
    my_subspace->combine_into(his_subspace,
             fail_on_existing, override_existing, mark_as_read);
  }}

}

void
sim_parameters::print_params(std::ostream &os, bool pretty_print, std::list<std::string>& namespaces) const
{
  sim_parameters::print_params(params_, os, pretty_print, namespaces);
}

}
