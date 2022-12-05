#ifdef HAVE_CONFIG_H
 #include "config.hpp"
#endif

#include "base/field.hpp"
#include "communicate/borders.hpp"
#include "geometry/geometry_eo.hpp"
#include "new_types/su3_op.hpp"
#include "threads/threads.hpp"

namespace nissa
{
  void apply_st2Doe(OddField<color>& out,
		    const EoField<quad_su3>& conf,
		    const EvnField<color>& in)
  {
    conf.updateHalo();
    in.updateHalo();
    
    NISSA_PARALLEL_LOOP(io,0,locVolh)
      {
	//neighbours search
	const int evup0=loceo_neighup[ODD][io][0];
	const int evdw0=loceo_neighdw[ODD][io][0];
	
	//derivative in the time direction - without self-summ
	unsafe_su3_prod_color(      out[io],conf[ODD][io   ][0],in[evup0]);
	su3_dag_subt_the_prod_color(out[io],conf[EVN][evdw0][0],in[evdw0]);
	
	//derivatives in the spatial direction - with self summ
	for(int mu=1;mu<NDIM;mu++)
	  {
	    const int evup=loceo_neighup[ODD][io][mu];
	    const int evdw=loceo_neighdw[ODD][io][mu];
	    
	    su3_summ_the_prod_color(    out[io],conf[ODD][io  ][mu],in[evup]);
	    su3_dag_subt_the_prod_color(out[io],conf[EVN][evdw][mu],in[evdw]);
	  }
      }
    NISSA_PARALLEL_LOOP_END;
    
    set_borders_invalid(out);
  }
  
  //put the 0.5 factor
  void apply_stDoe(OddField<color>& out,
		   const EoField<quad_su3>& conf,
		   const EvnField<color>& in)
  {
    apply_st2Doe(out,conf,in);
    
    NISSA_PARALLEL_LOOP(io,0,locVolh)
      for(int ic=0;ic<NCOL;ic++)
	for(int ri=0;ri<2;ri++)
	  out[io][ic][ri]*=0.5;
    NISSA_PARALLEL_LOOP_END;
    
    set_borders_invalid(out);
  }
  
  void apply_stDeo_half(EvnField<color>& out,
			const EoField<quad_su3>& conf,
			const OddField<color>& in)
  {
    conf.updateHalo();
    in.updateHalo();
    
    NISSA_PARALLEL_LOOP(ie,0,locVolh)
      {
	//neighbours search
	const int odup0=loceo_neighup[EVN][ie][0];
	const int oddw0=loceo_neighdw[EVN][ie][0];
	
	//derivative in the time direction - without self-summ
	unsafe_su3_prod_color(      out[ie],conf[EVN][ie   ][0],in[odup0]);
	su3_dag_subt_the_prod_color(out[ie],conf[ODD][oddw0][0],in[oddw0]);
	
	//derivatives in the spatial direction - with self summ
	for(int mu=1;mu<NDIM;mu++)
	  {
	    const int odup=loceo_neighup[EVN][ie][mu];
	    const int oddw=loceo_neighdw[EVN][ie][mu];
	    
	    su3_summ_the_prod_color(    out[ie],conf[EVN][ie  ][mu],in[odup]);
	    su3_dag_subt_the_prod_color(out[ie],conf[ODD][oddw][mu],in[oddw]);
	  }
	
	//Doe contains 1/2, we put an additional one
	color_prod_double(out[ie],out[ie],0.25);
      }
    NISSA_PARALLEL_LOOP_END;
    
    set_borders_invalid(out);
  }
  
  void apply_stD2ee_m2(EvnField<color>& out,
		       const EoField<quad_su3>& conf,
		       OddField<color>& temp,
		       const double& mass2,
		       const EvnField<color>& in)
  {
    if(IS_MASTER_THREAD)
      {
	//check arguments
	if(out==in)   crash("out==in!");
      }
    START_TIMING(portable_stD_app_time,nportable_stD_app);
    
    conf.updateHalo();
    in.updateHalo();
    
    NISSA_PARALLEL_LOOP_EXP(io,0,locVolh)
      {
	//neighbours search
	const int evup0=loceo_neighup[ODD][io][0];
	const int evdw0=loceo_neighdw[ODD][io][0];
	
	//derivative in the time direction - without self-summ
	unsafe_su3_prod_color(      temp[io],conf[ODD][io   ][0],in[evup0]);
	su3_dag_subt_the_prod_color(temp[io],conf[EVN][evdw0][0],in[evdw0]);
	
	//derivatives in the spatial direction - with self summ
	for(int mu=1;mu<NDIM;mu++)
	  {
	    const int evup=loceo_neighup[ODD][io][mu];
	    const int evdw=loceo_neighdw[ODD][io][mu];
	    
	    su3_summ_the_prod_color(    temp[io],conf[ODD][io  ][mu],in[evup]);
	    su3_dag_subt_the_prod_color(temp[io],conf[EVN][evdw][mu],in[evdw]);
	  }
      }
    NISSA_PARALLEL_LOOP_END_EXP;
    
    set_borders_invalid(temp);
    temp.updateHalo();
    
    //we still apply Deo, but then we put a - because we should apply Doe^+=-Deo
    NISSA_PARALLEL_LOOP_EXP(ie,0,locVolh)
      {
	const int odup0=loceo_neighup[EVN][ie][0];
	const int oddw0=loceo_neighdw[EVN][ie][0];
	
	unsafe_su3_prod_color(      out[ie],conf[EVN][ie   ][0],temp[odup0]);
	su3_dag_subt_the_prod_color(out[ie],conf[ODD][oddw0][0],temp[oddw0]);
	
	for(int mu=1;mu<NDIM;mu++)
	  {
	    const int odup=loceo_neighup[EVN][ie][mu];
	    const int oddw=loceo_neighdw[EVN][ie][mu];
	    
	    su3_summ_the_prod_color(    out[ie],conf[EVN][ie  ][mu],temp[odup]);
	    su3_dag_subt_the_prod_color(out[ie],conf[ODD][oddw][mu],temp[oddw]);
	  }
      }
    NISSA_PARALLEL_LOOP_END_EXP;
    
    if(mass2!=0)
      {
	NISSA_PARALLEL_LOOP_EXP(ie,0,locVolh)
	  for(int ic=0;ic<NCOL;ic++)
	    for(int ri=0;ri<2;ri++)
	      out[ie][ic][ri]=mass2*in[ie][ic][ri]-out[ie][ic][ri]*0.25;
	NISSA_PARALLEL_LOOP_END_EXP;
      }
    else
      {
	NISSA_PARALLEL_LOOP_EXP(ie,0,locVolh)
	  for(int ic=0;ic<NCOL;ic++)
	    for(int ri=0;ri<2;ri++)
	      out[ie][ic][ri]*=-0.25;
	NISSA_PARALLEL_LOOP_END_EXP;
      }
    set_borders_invalid(out);
    
    STOP_TIMING(portable_stD_app_time);
  }
}
