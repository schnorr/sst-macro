#
#   This file is part of SST/macroscale: 
#                The macroscale architecture simulator from the SST suite.
#   Copyright (c) 2011 Sandia Corporation.
#   This software is distributed under the BSD License.
#   Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#   the U.S. Government retains certain rights in this software.
#   For more information, see the LICENSE file in the top 
#   SST/macroscale directory.
#
include $(top_srcdir)/Makefile.common

AM_CPPFLAGS += -I$(top_srcdir)/tests -I$(top_builddir)/sst-dumpi -I$(top_srcdir)/sst-dumpi
AM_CPPFLAGS += -I$(top_srcdir)/sumi -I$(top_builddir)/sumi

if !INTEGRATED_SST_CORE
SUBDIRS = util

check_PROGRAMS = \
 test_packet_flow \
 test_serializable \
 test_unit_test \
 test_routing \
 test_topology

EXTRA_DIST = \
    topology/test_topology.h 

test_routing_SOURCES = \
    routing/test_routing.cc \
    routing/route_crossbar.cc \
    routing/route_butterfly.cc \
    routing/route_dragonfly.cc \
    routing/route_fattree.cc \
    routing/route_fbfly.cc \
    routing/route_hdtorus.cc

test_serializable_SOURCES = \
    test_serializable.cc 

test_packet_flow_SOURCES = \
    hardware/test_packet_flow.cc

test_topology_SOURCES = \
    topology/test_topology.cc \
    topology/top_crossbar.cc \
    topology/top_butterfly.cc \
    topology/top_dragonfly.cc \
    topology/top_fattree.cc \
    topology/top_fbfly.cc \
    topology/top_hdtorus.cc

test_unit_test_SOURCES = \
    test_unit_test.cc 


TEST_LDFLAGS = \
  $(top_builddir)/sstmac/install/libsstmac.la \
  $(top_builddir)/sprockit/sprockit/libsprockit.la \
  util/libsstmac_unit_test_util.la

if EXTERNAL_BOOST
  TEST_LDFLAGS += $(BOOST_LDFLAGS)
endif

test_packet_flow_LDADD = $(TEST_LDFLAGS) 
test_routing_LDADD = $(TEST_LDFLAGS)
test_serializable_LDADD = $(TEST_LDFLAGS)
test_unit_test_LDADD = $(TEST_LDFLAGS)
test_topology_LDADD = $(TEST_LDFLAGS)

endif
