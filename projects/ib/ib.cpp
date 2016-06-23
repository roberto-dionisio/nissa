#include <nissa.hpp>

#include "conf.hpp"
#include "contr.hpp"
#include "pars.hpp"
#include "prop.hpp"

using namespace nissa;

///////////////////////////////// initialise the library, read input file, allocate /////////////////////////////////////

void init_simulation(char *path)
{
  //open input file
  open_input(path);
  
  //init the grid
  read_init_grid();
  
  //Wall time
  read_str_double("WallTime",&wall_time);
  
  //Sources
  read_seed_start_random();
  read_stoch_source();
  read_set_dilutions_if_stoch_source(stoch_source);
  read_nhits();
  int nsources;
  read_str_int("NSources",&nsources);
  ori_source_name_list.resize(nsources);
  //discard header
  expect_str("Name");
  if(stoch_source)
    {
      expect_str("NoiseType");
      expect_str("Tins");
    }
  expect_str("Store");
  //loop over sources
  for(int isource=0;isource<nsources;isource++)
    {
      //name
      char name[1024];
      read_str(name,1024);
      //noise type and tins
      rnd_t noise_type=RND_ALL_PLUS_ONE;
      int tins=ALL_TIMES;
      if(stoch_source)
	{
	  char str_noise_type[20];
	  read_str(str_noise_type,20);
	  noise_type=convert_str_to_rnd_t(str_noise_type);
	  read_int(&tins);
	}
      //store
      int store_source;
      read_int(&store_source);
      //add
      ori_source_name_list[isource]=name;
      Q[name].init_as_source(noise_type,tins,store_source);
    }
  
  //Twisted run
  read_str_int("TwistedRun",&twisted_run);
  if(!twisted_run)
    {
      nr_lep=1;
      base=WILSON_BASE;
    }
  else
    {
      nr_lep=2;
      base=MAX_TWIST_BASE;
    }
  //kappa for twisted run
  double glb_kappa;
  if(twisted_run) read_str_double("Kappa",&glb_kappa);
  //Clover run
  read_str_int("CloverRun",&clover_run);
  //cSW for clover run
  if(clover_run) read_str_double("cSW",&glb_cSW);
  //NProps
  int nprops;
  read_str_int("NProps",&nprops);
  qprop_name_list.resize(nprops);
  //Discard header
  expect_str("Name");
  expect_str("Ins");
  expect_str("SourceName");
  expect_str("Tins");
  if(!twisted_run) expect_str("Kappa");
  else
    {
      expect_str("Mass");
      expect_str("R");
    }
  expect_str("Theta");
  expect_str("Residue");
  expect_str("Store");
  for(int iq=0;iq<nprops;iq++)
    {
      //name
      char name[1024];
      read_str(name,1024);
      master_printf("Read variable 'Name' with value: %s\n",name);
      if(Q.find(name)!=Q.end()) crash("name \'%s\' already included",name);
      
      //ins name
      char ins[3];
      read_str(ins,2);
      master_printf("Read variable 'Ins' with value: %s\n",ins);
      
      //source_name
      char source_name[1024];
      read_str(source_name,1024);
      if(Q.find(source_name)==Q.end()) crash("unable to find source %s",source_name);
      master_printf("Read variable 'Sourcename' with value: %s\n",source_name);
      
      //insertion time
      int tins;
      read_int(&tins);
      master_printf("Read variable 'Tins' with value: %d\n",tins);
      
      double kappa,mass,theta,residue;
      int r,store_prop;
      if(!twisted_run)
	{
	  mass=0;
	  read_double(&kappa);
	  master_printf("Read variable 'Kappa' with value: %lg\n",kappa);
	  r=0;
	}
      else
	{
	  read_double(&mass);
	  master_printf("Read variable 'Mass' with value: %lg\n",mass);
	  read_int(&r);
	  master_printf("Read variable 'R' with value: %d\n",r);
	  kappa=glb_kappa;
	  
	  //include tau in the mass
	  mass*=tau3[r];
	}
      read_double(&theta);
      master_printf("Read variable 'Theta' with value: %lg\n",theta);
      read_double(&residue);
      master_printf("Read variable 'Residue' with value: %lg\n",residue);
      read_int(&store_prop);
      master_printf("Read variable 'Store' with value: %d\n",store_prop);
      Q[name].init_as_propagator(ins_from_tag(ins[0]),source_name,tins,residue,kappa,mass,r,theta,store_prop);
      qprop_name_list[iq]=name;
    }
  
  read_photon_pars();
  
  read_free_theory_flag();
  read_random_gauge_transform();
  
  read_loc_hadr_curr();
  read_loc_muon_curr();
  
  //mesons
  read_mes2pts_contr_pars();
  
  //meslept
  read_meslep_contr_pars();
  
  //baryons
  set_Cg5();
  read_bar2pts_contr_pars();
  
  read_ngauge_conf();
  
  ///////////////////// finished reading apart from conf list ///////////////
  
  if(clover_run)
    {
      Cl=nissa_malloc("Cl",loc_vol,clover_term_t);
      invCl=nissa_malloc("invCl",loc_vol,inv_clover_term_t);
    }
  
  allocate_loop_source();
  allocate_photon_fields();
  
  allocate_mes2pts_contr();
  
  nmeslep_corr=nquark_lep_combos*nindep_meslep_weak*norie*nins;
  meslep_hadr_part=nissa_malloc("hadr",loc_vol,spinspin);
  meslep_contr=nissa_malloc("meslep_contr",glb_size[0]*nindep_meslep_weak*nmeslep_proj*nmeslep_corr,complex);
  
  allocate_bar2pts_contr();
  
  allocate_L_prop();
  temp_lep=nissa_malloc("temp_lep",loc_vol+bord_vol,spinspin);
  conf=nissa_malloc("conf",loc_vol+bord_vol+edge_vol,quad_su3);
  ape_smeared_conf=nissa_malloc("ape_smeared_conf",loc_vol+bord_vol,quad_su3);
}

//close deallocating everything
void close()
{
  print_statistics();
  
  Q.clear();
  
  free_photon_fields();
  free_loop_source();
  free_L_prop();
  nissa_free(conf);
  nissa_free(ape_smeared_conf);
  
  free_mes2pts_contr();
  
  nissa_free(meslep_hadr_part);
  nissa_free(meslep_contr);
  nissa_free(lep_contr_iq1);
  nissa_free(lep_contr_iq2);
  nissa_free(leps);
  nissa_free(lep_energy);
  nissa_free(neu_energy);
  
  if(clover_run)
    {
      nissa_free(Cl);
      nissa_free(invCl);
    }
  free_bar2pts_contr();
}

void in_main(int narg,char **arg)
{
  //Basic mpi initialization
  tot_prog_time-=take_time();
  
  //check argument
  if(narg<2) crash("Use: %s input_file",arg[0]);
  
  //init simulation according to input file
  init_simulation(arg[1]);
  
  //loop over the configs
  int iconf=0;
  while(read_conf_parameters(iconf,finish_file_present))
    {
      //setup the conf and generate the source
      start_new_conf();
      
      for(int ihit=0;ihit<nhits;ihit++)
	{
	  start_hit(ihit);
	  generate_propagators(ihit);
	  compute_contractions();
	}
      
      print_contractions();
      
      mark_finished();
    }
  
  //close the simulation
  tot_prog_time+=take_time();
  close();
}

int main(int narg,char **arg)
{
  init_nissa_threaded(narg,arg,in_main);
  close_nissa();
  
  return 0;
}