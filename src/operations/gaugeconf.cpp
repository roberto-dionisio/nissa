#ifdef HAVE_CONFIG_H
 #include "config.hpp"
#endif

#include "base/bench.hpp"
#include "base/random.hpp"
#include "communicate/borders.hpp"
#include "communicate/edges.hpp"
#include "geometry/geometry_eo.hpp"
#include "linalgs/reduce.hpp"
#include "new_types/su3_op.hpp"
#include "operations/gaugeconf.hpp"
#include "operations/su3_paths/gauge_sweeper.hpp"
#include "measures/gauge/topological_charge.hpp"
#include "routines/mpi_routines.hpp"

/*
  rotate a field anti-clockwise by 90 degrees
  
   0---1---2---0        0---6---3---0
   |   |   |   |        |   |   |   |
   6---7---8---6        2---8---5---2
   |   |   |   |        |   |   |   |
   3---4---5---3        1---7---4---1
   |   |   |   |        |   |   |   |
   O---1---2---0        O---6---3---0
   
   d2
   O d1
   
   where d1=axis+1
   and   d2=d1+1
   
*/

namespace nissa
{
  void ac_rotate_vector(void *out,void *in,int axis,size_t bps)
  {
    //find the two swapping direction
    int d1=1+(axis-1+1)%3;
    int d2=1+(axis-1+2)%3;
    
    //check that the two directions have the same size and that we are not asking 0 as axis
    if(glbSize[d1]!=glbSize[d2]) crash("Rotation works only if dir %d and %d have the same size!",glbSize[d1],glbSize[d2]);
    if(axis==0) crash("Error, only spatial rotations implemented");
    int L=glbSize[d1];
    
    //allocate destinations and sources
    coords_t *xto=nissa_malloc("xto",locVol,coords_t);
    coords_t *xfr=nissa_malloc("xfr",locVol,coords_t);
    
    //scan all local sites to see where to send and from where to expect data
    NISSA_LOC_VOL_LOOP(ivol)
    {
      //copy 0 and axis coord to "to" and "from" sites
      xto[ivol][0]=xfr[ivol][0]=glbCoordOfLoclx[ivol][0];
      xto[ivol][axis]=xfr[ivol][axis]=glbCoordOfLoclx[ivol][axis];
      
      //find reamining coord of "to" site
      xto[ivol][d1]=(L-glbCoordOfLoclx[ivol][d2])%L;
      xto[ivol][d2]=glbCoordOfLoclx[ivol][d1];
      
      //find remaining coord of "from" site
      xfr[ivol][d1]=glbCoordOfLoclx[ivol][d2];
      xfr[ivol][d2]=(L-glbCoordOfLoclx[ivol][d1])%L;
    }
    
    //call the remapping
    //remap_vector((char*)out,(char*)in,xto,xfr,bps);
    crash("to be reimplemented");
    
    //free vectors
    nissa_free(xfr);
    nissa_free(xto);
  }
  
  /*
    rotate the gauge configuration anti-clockwise by 90 degrees
    this is more complicated than a single vector because of link swaps
    therefore the rotation is accomplished through 2 separates steps
    
    .---.---.---.     .---.---.---.       .---.---.---.
    |           |     |           |       |           |
    .   B 3 C   .     .   B 2'C   .       .   C 4'D   .
    |   2   4   |     |   1   3   |       |   3   1   |
    .   A 1 D   .     .   A 4'D   .       .   B 2'A   .
    |           |     |           |       |           |
    O---.---.---.     O---.---.---.       O---.---.---.
    
    d2
    O d1
    
  */
  
  void ac_rotate_gauge_conf(quad_su3 *out,quad_su3 *in,int axis)
  {
    int d0=0;
    int d1=1+(axis-1+1)%3;
    int d2=1+(axis-1+2)%3;
    int d3=axis;
    
    //allocate a temporary conf with borders
    quad_su3 *temp_conf=nissa_malloc("temp_conf",locVol+bord_vol,quad_su3);
    memcpy(temp_conf,in,locVol*sizeof(quad_su3));
    communicate_lx_quad_su3_borders(temp_conf);
    
    //now reorder links
    NISSA_LOC_VOL_LOOP(ivol)
    {
      //copy temporal direction and axis
      memcpy(out[ivol][d0],temp_conf[ivol][d0],sizeof(su3));
      memcpy(out[ivol][d3],temp_conf[ivol][d3],sizeof(su3));
      //swap the other two
      unsafe_su3_hermitian(out[ivol][d1],temp_conf[loclxNeighdw[ivol][d2]][d2]);
      memcpy(out[ivol][d2],temp_conf[ivol][d1],sizeof(su3));
    }
    
    //rotate rigidly
    ac_rotate_vector(out,out,axis,sizeof(quad_su3));
  }
  
  //put boundary conditions on the gauge conf
  void put_boundaries_conditions(quad_su3 *conf,const momentum_t& theta_in_pi,int putonbords,int putonedges)
  {
    complex theta[NDIM];
    for(int idir=0;idir<NDIM;idir++)
      {
	theta[idir][0]=cos(theta_in_pi[idir]*M_PI/glbSize[idir]);
	theta[idir][1]=sin(theta_in_pi[idir]*M_PI/glbSize[idir]);
      }
    
    int nsite=locVol;
    if(putonbords) nsite+=bord_vol;
    if(putonedges) nsite+=edge_vol;
    
    int ivolIncr,rankIncr;
    get_loclx_and_rank_of_coord(ivolIncr,rankIncr,{glbSize[0]-1,8,23,7});
    master_printf("putting boundary on conf of ptr: %p\n",conf);
    
    // if(rank==rankIncr)
    //   {
    // 	printf("before phasing the bulk of the conf on rank %d is:\n",rank);
    // 	su3_print(conf[ivolIncr][0]);
    //   }
    
    NISSA_PARALLEL_LOOP(ivol,0,nsite)
      for(int idir=0;idir<NDIM;idir++) safe_su3_prod_complex(conf[ivol][idir],conf[ivol][idir],theta[idir]);
    NISSA_PARALLEL_LOOP_END;
    
    // if(rank==rankIncr)
    //   {
    // 	printf("after phasing the bulk of the conf on rank %d is:\n",rank);
    // 	su3_print(conf[ivolIncr][0]);
    // 	printf("bord valid: %d\n",check_borders_valid(conf));
    //   }
    
    
    if(!putonbords) set_borders_invalid(conf);
    if(!putonedges) set_edges_invalid(conf);
  }
  
  void rem_boundaries_conditions(quad_su3 *conf,const momentum_t& theta_in_pi,int putonbords,int putonedges)
  {
    const momentum_t minus_theta_in_pi={-theta_in_pi[0],-theta_in_pi[1],-theta_in_pi[2],-theta_in_pi[3]};
    put_boundaries_conditions(conf,minus_theta_in_pi,putonbords,putonedges);
  }
  
  //Adapt the border condition
  void adapt_theta(quad_su3 *conf,momentum_t& old_theta,const momentum_t& put_theta,int putonbords,int putonedges)
  {
    momentum_t diff_theta;
    int adapt=0;
    
    for(int idir=0;idir<NDIM;idir++)
      {
	adapt=adapt || (old_theta[idir]!=put_theta[idir]);
	diff_theta[idir]=put_theta[idir]-old_theta[idir];
	old_theta[idir]=put_theta[idir];
      }
    
    if(adapt)
      {
	master_printf("Necessary to add boundary condition: %f %f %f %f\n",diff_theta[0],diff_theta[1],diff_theta[2],diff_theta[3]);
	put_boundaries_conditions(conf,diff_theta,putonbords,putonedges);
      }
  }
  
  //generate an identical conf
  void generate_cold_eo_conf(eo_ptr<quad_su3> conf)
  {
    for(int par=0;par<2;par++)
      {
	NISSA_LOC_VOLH_LOOP(ivol)
	  for(int mu=0;mu<NDIM;mu++)
	    su3_put_to_id(conf[par][ivol][mu]);
	
	set_borders_invalid(conf[par]);
      }
  }
  
  //generate a random conf
  void generate_hot_eo_conf(eo_ptr<quad_su3> conf)
  {
    if(loc_rnd_gen_inited==0) crash("random number generator not inited");
    
    for(int par=0;par<2;par++)
      {
	NISSA_LOC_VOLH_LOOP(ieo)
        {
	  int ilx=loclx_of_loceo[par][ieo];
	  for(int mu=0;mu<NDIM;mu++)
	    su3_put_to_rnd(conf[par][ieo][mu],loc_rnd_gen[ilx]);
	}
	
	set_borders_invalid(conf[par]);
      }
  }
  
  //generate an identical conf
  void generate_cold_lx_conf(quad_su3 *conf)
  {
    NISSA_LOC_VOL_LOOP(ivol)
      for(int mu=0;mu<NDIM;mu++)
	su3_put_to_id(conf[ivol][mu]);
    
    set_borders_invalid(conf);
  }
  
  //generate a random conf
  void generate_hot_lx_conf(quad_su3 *conf)
  {
    if(loc_rnd_gen_inited==0) crash("random number generator not inited");
    
    NISSA_LOC_VOL_LOOP(ivol)
      for(int mu=0;mu<NDIM;mu++)
	su3_put_to_rnd(conf[ivol][mu],loc_rnd_gen[ivol]);
    
    set_borders_invalid(conf);
  }
  
  //perform a unitarity check on a lx conf
  void unitarity_check_lx_conf(unitarity_check_result_t &result,quad_su3 *conf)
  {
    //results
    double* loc_avg=nissa_malloc("loc_avg",locVol,double);
    double* loc_max=nissa_malloc("loc_max",locVol,double);
    int64_t* loc_nbroken=nissa_malloc("loc_nbroken",locVol,int64_t);
    
    NISSA_LOC_VOL_LOOP(ivol)
      for(int idir=0;idir<NDIM;idir++)
	{
	  const double err=su3_get_non_unitariness(conf[ivol][idir]);
	  
	  //compute average and max deviation
	  loc_avg[ivol]=err;
	  loc_max[ivol]=err;
	  loc_nbroken[ivol]=(err>1e-13);
	}
    
    glb_reduce(&result.average_diff,loc_avg,locVol);
    result.average_diff/=glbVol*NDIM;
    
    glbReduce(&result.max_diff,loc_max,locVol,
	      [] CUDA_DEVICE (double& res,const double& acc)  __attribute__((always_inline))
	      {
		if(acc>res)
		  res=acc;
	      });
    glb_reduce(&result.nbroken_links,loc_nbroken,locVol);
    
    nissa_free(loc_avg);
    nissa_free(loc_max);
    nissa_free(loc_nbroken);
  }
  
  //unitarize an a lx conf
  void unitarize_lx_conf_orthonormalizing(quad_su3* conf)
  {
    START_TIMING(unitarize_time,nunitarize);
    
    NISSA_PARALLEL_LOOP(ivol,0,locVol)
      for(int idir=0;idir<NDIM;idir++)
	{
	  su3 t;
	  su3_unitarize_orthonormalizing(t,conf[ivol][idir]);
	  su3_copy(conf[ivol][idir],t);
	}
    NISSA_PARALLEL_LOOP_END;
    set_borders_invalid(conf);
    STOP_TIMING(unitarize_time);
  }
  
  //unitarize the conf by explicitly by projecting it maximally to su3
  void unitarize_lx_conf_maximal_trace_projecting(quad_su3* conf)
  {
    START_TIMING(unitarize_time,nunitarize);
    
    NISSA_PARALLEL_LOOP(ivol,0,locVol)
      for(int mu=0;mu<NDIM;mu++)
        su3_unitarize_maximal_trace_projecting(conf[ivol][mu],conf[ivol][mu]);
    NISSA_PARALLEL_LOOP_END;
    
    set_borders_invalid(conf);
    STOP_TIMING(unitarize_time);
  }
  
  //eo version
  void unitarize_eo_conf_maximal_trace_projecting(eo_ptr<quad_su3> conf)
  {
    START_TIMING(unitarize_time,nunitarize);
    
    for(int par=0;par<2;par++)
      {
        NISSA_PARALLEL_LOOP(ivol,0,locVolh)
          for(int mu=0;mu<NDIM;mu++)
            su3_unitarize_maximal_trace_projecting(conf[par][ivol][mu],conf[par][ivol][mu]);
	NISSA_PARALLEL_LOOP_END;
        
        set_borders_invalid(conf[par]);
      }
    
    STOP_TIMING(unitarize_time);
  }
  
  //overrelax an lx configuration
  void overrelax_lx_conf_handle(su3 out,su3 staple,int ivol,int mu,void *pars)
  {su3_find_overrelaxed(out,out,staple,((int*)pars)[0]);}
  void overrelax_lx_conf(quad_su3* conf,gauge_sweeper_t* sweeper,int nhits)
  {sweeper->sweep_conf(conf,overrelax_lx_conf_handle,(void*)&nhits);}
  
  //same for heatbath
  namespace heatbath_lx_conf_ns
  {
    struct pars_t
    {
      double beta;
      int nhits;
      pars_t(double beta,int nhits) : beta(beta),nhits(nhits){}
    };
    void handle(su3 out,su3 staple,int ivol,int mu,void *pars)
    {su3_find_heatbath(out,out,staple,((pars_t*)pars)->beta,((pars_t*)pars)->nhits,loc_rnd_gen+ivol);}
  }
  void heatbath_lx_conf(quad_su3* conf,gauge_sweeper_t* sweeper,double beta,int nhits)
  {heatbath_lx_conf_ns::pars_t pars(beta,nhits);sweeper->sweep_conf(conf,heatbath_lx_conf_ns::handle,&pars);}
  
  //same for cooling
  void cool_lx_conf_handle(su3 out,su3 staple,int ivol,int mu,void *pars)
  {su3_unitarize_maximal_trace_projecting_iteration(out,staple);}
  void cool_lx_conf(quad_su3* conf,gauge_sweeper_t* sweeper)
  {sweeper->sweep_conf(conf,cool_lx_conf_handle,NULL);}
  
  //measure the average gauge energy
  void average_gauge_energy(double* energy,quad_su3* conf)
  {
    communicate_lx_quad_su3_edges(conf);
    double *loc_energy=nissa_malloc("energy",locVol,double);
    
    NISSA_PARALLEL_LOOP(ivol,0,locVol)
      {
	//compute the clover-shape paths
	as2t_su3 leaves;
	four_leaves_point(leaves,conf,ivol);
	
	loc_energy[ivol]=0.0;
	for(int i=0;i<NDIM*(NDIM-1)/2;i++)
	  {
	    su3 A;
	    unsafe_su3_subt_su3_dag(A,leaves[i],leaves[i]);
	    su3_prodassign_double(A,1.0/8.0); //factor 1/2 for the antihermitian, 1/4 for average leave
	    complex temp;
	    trace_su3_prod_su3(temp,A,A);
	    loc_energy[ivol]-=temp[RE];
	  }
	loc_energy[ivol]/=glbVol;
      }
    NISSA_PARALLEL_LOOP_END;
    THREAD_BARRIER();
    
    glb_reduce(energy,loc_energy,locVol);
    
    nissa_free(loc_energy);
  }
  
  std::string gauge_obs_meas_pars_t::get_str(bool full)
  {
    std::ostringstream os;
    
    os<<"MeasPlaqPol\n";
    if(each!=def_each() or full) os<<" Each\t\t=\t"<<each<<"\n";
    if(after!=def_after() or full) os<<" After\t\t=\t"<<after<<"\n";
    if(path!=def_path() or full) os<<" Path\t\t=\t\""<<path.c_str()<<"\"\n";
    if(meas_plaq!=def_meas_plaq() or full) os<<" MeasPlaq\t\t=\t"<<meas_plaq<<"\n";
    if(meas_energy!=def_meas_energy() or full) os<<" MeasEnergy\t\t=\t"<<meas_energy<<"\n";
    if(meas_poly!=def_meas_poly() or full) os<<" MeasPoly\t\t=\t"<<meas_poly<<"\n";
    if(use_smooth!=def_use_smooth() or full) os<<" UseSmooth\t\t=\t"<<use_smooth<<"\n";
    os<<smooth_pars.get_str(full);
    
    return os.str();
  }
}
