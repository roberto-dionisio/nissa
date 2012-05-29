#ifndef _spin_h
#define _spin_h
void as2t_saturate(complex out,as2t a,as2t b);
void get_spincolor_from_colorspinspin(spincolor out,colorspinspin in,int id_source);
void get_spincolor_from_su3spinspin(spincolor out,su3spinspin in,int id_source,int ic_source);
void print_spinspin(spinspin s);
void put_spincolor_into_colorspinspin(colorspinspin out,spincolor in,int id_source);
void put_spincolor_into_su3spinspin(su3spinspin out,spincolor in,int id_source,int ic_source);
void safe_dirac_prod_spin(spin out,dirac_matr *m,spin in);
void safe_spinspin_spin_prod(spin out,spinspin a,spin b);
void safe_spinspin_spinspin_prod(spinspin out,spinspin a,spinspin b);
void safe_spinspin_spinspindag_prod(spinspin out,spinspin a,spinspin b);
void spin_copy(spin b,spin a);
void spin_subt(spin a,spin b,spin c);
void spin_subt_the_complex_conj2_prod(spin a,spin b,complex c);
void spin_subt_the_complex_prod(spin a,spin b,complex c);
void spin_summ(spin a,spin b,spin c);
void spin_summ_the_complex_conj2_prod(spin a,spin b,complex c);
void spin_summ_the_complex_prod(spin a,spin b,complex c);
void spinspin_prod_double(spinspin a,spinspin b,double c);
void spinspin_put_to_id(spinspin a);
void spinspin_put_to_zero(spinspin a);
void spinspin_subt_the_complex_conj2_prod(spinspin a,spinspin b,complex c);
void spinspin_subt_the_complex_prod(spinspin a,spinspin b,complex c);
void spinspin_summ_the_complex_prod(spinspin a,spinspin b,complex c);
void spinspin_summ_the_spinspin_prod(spinspin out,spinspin a,spinspin b);
void spinspin_summ_the_spinspindag_prod(spinspin out,spinspin a,spinspin b);
void spinspin_the_complex_conj2_prod(spinspin a,spinspin b,complex c);
void summ_the_trace_prod_dirac_spinspin(complex c,dirac_matr *a,spinspin b);
void trace_prod_dirac_spinspin(complex c,dirac_matr *a,spinspin b);
void unsafe_dirac_prod_spin(spin out,dirac_matr *m,spin in);
void unsafe_spinspin_spin_prod(spin out,spinspin a,spin b);
void unsafe_spinspin_spinspin_prod(spinspin out,spinspin a,spinspin b);
void unsafe_spinspin_spinspindag_prod(spinspin out,spinspin a,spinspin b);
#endif
