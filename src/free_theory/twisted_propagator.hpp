#ifndef _TWISTED_PROPAGATOR_HPP
#define _TWISTED_PROPAGATOR_HPP

#include "free_theory_types.hpp"

namespace nissa
{
  void mom_space_twisted_propagator_of_imom(spin1prop prop,quark_info qu,int imom);
  void multiply_from_left_by_mom_space_twisted_propagator(spin *out,spin *in,quark_info qu);
  void compute_mom_space_twisted_propagator(spinspin *prop,quark_info qu);
  void compute_x_space_twisted_propagator_by_fft(spinspin *prop,quark_info qu);
  void compute_x_space_twisted_squared_propagator_by_fft(spinspin *sq_prop,quark_info qu);
  void multiply_from_left_by_x_space_twisted_propagator_by_fft(spin *prop,spin *ext_source,quark_info qu);
  void multiply_from_left_by_x_space_twisted_propagator_by_fft(spinspin *prop,spinspin *ext_source,quark_info qu);
  void multiply_from_left_by_x_space_twisted_propagator_by_inv(spin *prop,spin *ext_source,quark_info qu);
  void multiply_from_left_by_x_space_twisted_propagator_by_inv(spinspin *prop,spinspin *ext_source,quark_info qu);
  void compute_x_space_twisted_propagator_by_inv(spinspin *prop,quark_info qu);
}

#endif
