#ifndef _SMOOTH_HPP
#define _SMOOTH_HPP

#include "APE.hpp"
#include "cooling.hpp"
#include "HYP.hpp"
#include "stout.hpp"
#include "Wflow.hpp"

namespace nissa
{
  //parameters to smooth a configuration
  struct smooth_pars_t
  {
    enum method_t{COOLING,STOUT,WFLOW,APE,HYP};
    
    //basic
    method_t method;
    int meas_each_nsmooth;
    method_t def_method(){return COOLING;}
    int def_meas_each_nsmooth(){return 1;}
    
    //space, time or full spacetime
    enum space_or_time_t{SPACE,TIME,SPACETIME};
    space_or_time_t space_or_time;
    space_or_time_t def_space_or_time(){return SPACETIME;}
    
    //returns the directions to smooth according to parameter
    static which_dir_t get_dirs(space_or_time_t space_or_time)
    {
      which_dir_t res={};
      
      switch(space_or_time)
      {
      case SPACE:
	res=all_other_dirs[0];
	break;
      case TIME:
	res=only_dir[0];
	break;
      case SPACETIME:
	res=all_dirs;
	break;
      default:
	crash("Unknown type");
      }
      
      return res;
    }
    
    //returns the minimal staple direction according to parameter
    static int get_staple_min_dir(space_or_time_t space_or_time)
    {
      int res=0;
      
      switch(space_or_time)
      {
      case SPACE:
	res=1;
	break;
      case TIME:
	res=1;
	break;
      case SPACETIME:
	res=0;
	break;
      default:
	res=0;
	crash("Unknown type");
      }
      
      return res;
    }
    
    //convert a space_or_time_t into a str
    inline std::string space_or_time_str_from_name(space_or_time_t space_or_time)
    {
      std::string res;
      
      switch(space_or_time)
      {
      case SPACE:
	res="Space";
	break;
      case TIME:
	res="Time";
	break;
      case SPACETIME:
	res="SpaceTime";
	break;
      default:
	res="Boh";
	crash("Unknown type");
      }
      
      return res;
    }
    
    //pars
    cool_pars_t cool;
    stout_pars_t stout;
    Wflow_pars_t Wflow;
    ape_pars_t ape;
    hyp_pars_t hyp;
    
    std::string get_method_name()
    {
      std::string res;
      
      switch(method)
	{
	case COOLING: res="Cooling";break;
	case STOUT: res="Stout";break;
	case WFLOW: res="WFlow";break;
	case APE: res="Ape";break;
	case HYP: res="Hyp";break;
	default: crash("not meant to be reached");res="";
	}
      
      return res;
    }
    
    /// return the next measure strictly after nsmooth
    int next_nsmooth_meas(int nsmooth) const 
    {
      return (nsmooth+meas_each_nsmooth)/meas_each_nsmooth*meas_each_nsmooth;
    }
    
    //returns the number of smooth
    int nsmooth() const
    {
      int res=0;
      
      switch(method)
	{
	case COOLING:res=cool.nsteps;break;
	case STOUT:res=stout.nlevels;break;
	case WFLOW:res=Wflow.nflows;break;
	case APE:res=ape.nlevels;break;
	case HYP:res=hyp.nlevels;break;
	default:crash("not meant to be reached");res=0;
	}
      
      return res;
    }
    
    //returns the number of measurement, without 0
    int nmeas_nonzero()
    {return nsmooth()/meas_each_nsmooth;}
    
    int master_fprintf(FILE *fout,bool full) {return nissa::master_fprintf(fout,"%s",get_str().c_str());}
    std::string get_str(bool full=false);
    
    int is_nonstandard()
    {
      return
	method!=def_method() or
	space_or_time!=def_space_or_time() or
	meas_each_nsmooth!=def_meas_each_nsmooth();
    }
    
    smooth_pars_t() :
      method(def_method()),
      meas_each_nsmooth(def_meas_each_nsmooth()),
      space_or_time(def_space_or_time())
    {}
  };
  
  void smooth_lx_conf_one_step(quad_su3 *smoothed_conf,smooth_pars_t &sp,const which_dir_t& dirs=all_dirs,int staple_min_dir=0);
  
  bool smooth_lx_conf_until_next_meas(LxField<quad_su3>& smoothed_conf,
				      const smooth_pars_t &sp,
				      int &nsmooth,
				      const which_dir_t& dirs,
				      const int& staple_min_dir=0);
  
  void smooth_lx_conf(quad_su3 *smoothed_conf,smooth_pars_t &sp,const which_dir_t& dirs=all_dirs,int staple_min_dir=0);
}

#endif
