#include <limits.h>
#include <unistd.h>
#include <ctype.h>
#include <topomatch.h>
#include <tm_timings.h>
#include "tm_config.h"

int test_mapping(char *arch_filename, tm_file_type_t arch_file_type, char *com_filename, int bind_flag,
		 char *bind_filename, int opt_topo_flag, tm_metric_t metric, int oversub_fact, int display_solution_flag) {
   /*tm_tree_t      *comm_tree = NULL;*/
   tm_topology_t  *topology;
   int            verbose_level = tm_get_verbose_level();
   tm_affinity_mat_t *aff_mat;
   tm_solution_t  *sol;

   
   TIC;
   TIC;
   /* Parse the arch file according to its type XML or TGT*/
   topology = tm_load_topology(arch_filename, arch_file_type);

   /* build affinity matrices and get the number of processes*/
   aff_mat = tm_load_aff_mat(com_filename);
   /* If we have binding constraints store them in the topology */
   if (bind_flag) {
     if(!tm_topology_add_binding_constraints(bind_filename, topology))
       exit(-1);
   }
   double io_dur = TOC;

   if(verbose_level == INFO)
     tm_display_arity(topology);
   else if(verbose_level >= DEBUG)
     tm_display_topology(topology);


   /* Manage the case where oversubscribing is requested */
   if(oversub_fact>1){
     tm_enable_oversubscribing(topology, oversub_fact);
   }

   /* optimize the toplogy in order to decompose arity of the tree into subtrees.
      Usualy it speeds up tm_build_tree_from_topology
    */
   TIC;
   if(opt_topo_flag)
     tm_optimize_topology(&topology);
   double    optimize_dur = TOC;

   if(verbose_level == INFO)
     tm_display_arity(topology);
   else if(verbose_level >= DEBUG)
     tm_display_topology(topology);

   TIC;
   /* Compute the mapping and build the solution*/
   sol = tm_compute_mapping(topology, aff_mat , NULL , NULL);
   double    map_dur = TOC;

   double    duration = TOC;

   tm_verbose_fprintf(TIMING, verbose_stdout, "I/O:            %10.4f ms\n", io_dur * 1000);
   tm_verbose_fprintf(TIMING, verbose_stdout, "Optimize:       %10.4f ms\n", optimize_dur * 1000);
   tm_verbose_fprintf(TIMING, verbose_stdout, "Mapping:        %10.4f ms\n", map_dur * 1000);
   tm_verbose_fprintf(TIMING, verbose_stdout, "Total duration: %10.4f ms\n", duration * 1000);
   

   if(display_solution_flag){
     /* display the solution. Warnig: use the oiginal topology not the optimized one*/
     printf("TopoMatch: ");
     tm_display_solution(topology, aff_mat, sol, metric);
     /* display the solution of other heuristics*/
     tm_display_other_heuristics(topology, aff_mat, metric);
   }
   
   /* Free the allocated memory*/
   tm_free_topology(topology);
   tm_free_solution(sol);
   tm_free_affinity_mat(aff_mat);
   tm_finalize();

   return 0;
}

void printUsage(char **argv) {
  fprintf(stderr,"This is the mapping tool for %s (https://gitlab.inria.fr/ejeannot/topomatch) version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
#ifdef HAVE_LIBSCOTCH
  if(SCOTCH_NUM_SIZE == 64){
    fprintf(stderr,"This version is compiled with Scotch (64 bits).\n");
  }else{
    fprintf(stderr,"This version is compiled with Scotch (32 bits).\n");
    fprintf(stderr,"WARNING: Scotch library compiled with 32 bits integer. This might not work for large values in the communication pattern.\n");
    fprintf(stderr,"         We advise you to compile Scotch with -DINTSIZE64 in the CFLAGS variable of the Scotch Makfefile.inc\n");
  }
#else
  fprintf(stderr,"This version is not compiled with Scotch.\n");
#endif  
  fprintf(stderr,
	   "Usage: %s\n\t-t|x <filename> Architecture file[tgt|xml]\n\t-c <filename> Communication pattern file\n",
	   argv[0]);
  fprintf(stderr,
	  "Options:\n\t-a <threshold>: number in [0,1] that defnies how the communication pattern will be sparsify when Scotch is used.\n\tOnly values above max_val*threshold will be considered (max_val being the maximum value of the communicarion pattern). Default value is %.1f.\n\t-b <filename> binding constraint file\n\t-f <filename> filename where to output the verbose messages\n\t-g <threshold>: number of thousands of groups above which we use the bucket grouping (faster but less quality) strategy instead of the standard startegy (default = 30)\n\t-l <Strategy for tleaf topology> (0: default, 1: Scotch).\n\t-m <metric> evaluation metric (1: SUM_COM (default), 2: MAX_COM, 3: HOP_BYTE)\n\t-o <factor> oversubscribing factor (default 1)\n\t-p <nb thread> maximum number of threads used in parallel sections (default: number of cores)\n\t-v <level> verbose level. Default: 2. From 0 (disabled) to 7 (full debug verbose).\n",tm_get_sparse_factor());

    fprintf(stderr,
	  "Flags:\n\t-d disable topology optimization\n\t-e force exhaustive search\n\t-k force greedy k-partionniong\n\t-n force physical numbering (usefull only for XML/hwloc)\n\t-s disable display of solution (useful for timings -- with -v 4)\n\t-h display this help\n");
}



int main(int argc, char **argv) {
  int             c;
  char            *arch_filename    = NULL;
  char            *com_filename     = NULL;
  char            *bind_filename    = NULL;
  int             bind_flag = 0;
  int             opt_topo_flag = 1;
  int             display_solution_flag = 1;
  int             verbose_level = ERROR;
  int             strat;
  tm_file_type_t  arch_file_type = TM_FILE_TYPE_UNDEF;
  tm_metric_t     metric = TM_METRIC_SUM_COM;
  int             oversub_fact = 1;
  float           sf;

  tm_set_exhaustive_search_flag(0);
  tm_set_greedy_flag(0);
  tm_set_numbering(TM_NUMBERING_LOGICAL);
  tm_set_mapping_strat(TM_STRAT_TM);
  /*single character <=> flag ; 
    character + ':' <=> option*/
   while ((c = getopt(argc, argv, "hsnkedt:x:c:b:v:m:o:p:f:l:g:a:")) != -1) {
     switch (c) {
         case 'a':
	   sf = atof(optarg);
	   if(sf < 0){
	     sf = 0.0;
	   }else if (sf > 1){
	     sf = 1.0;
	   }
	   tm_set_sparse_factor(sf);
	   break; 
         case 'g':
	   tm_set_bucket_grouping_threshold(atol(optarg));
	   break; 
         case 'l':
	   strat = atoi(optarg);
	   if(strat == 1)
	     tm_set_mapping_strat(TM_STRAT_SCOTCH);
	   break; 
         case 's':
	   display_solution_flag = 0;
	   break;
         case 'h':
	   printUsage(argv);
	   exit(-1);
         case 'd':
	   opt_topo_flag = 0;
	   break;
         case 'k':
	   tm_set_greedy_flag(1);
	   break;
         case 'n':
	   tm_set_numbering(TM_NUMBERING_PHYSICAL);
	   break;
         case 'e':
	   tm_set_exhaustive_search_flag(1);
	   tm_set_bucket_grouping_threshold(LONG_MAX);
	   break;
         case 'p':
	   tm_set_max_nb_threads(atoi(optarg));
	   break;
	 case 'f':
	    tm_open_verbose_file(optarg);	    
	    break;
	 case 't':
	    arch_filename = optarg;
	    arch_file_type = TM_FILE_TYPE_TGT;
	    break;
	 case 'x':
	    arch_filename = optarg;
	    arch_file_type = TM_FILE_TYPE_XML;
	    break;
	 case 'c':
	    com_filename = optarg;
	    break;
	 case 'b':
	    bind_flag = 1;
	    bind_filename = optarg;
	    break;
	 case 'v':
	   verbose_level  = atoi(optarg);
	   break;
	 case 'm':
	   metric  = atoi(optarg);
	   break;
         case 'o':
	   oversub_fact = atoi(optarg);
           break;
	 case '?':
	    if (isprint(optopt))
	       fprintf(stderr, "Unknown option `-%c'.\n", optopt);
	    else
	       fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);

	    printUsage(argv);
	    exit(-1);

	 default:
	    printUsage(argv);
	    exit(-1);
      }
   }

   if (!arch_filename || !com_filename) {
      fprintf(stderr,"Error: Missing \"Architecture file\" and/or \"Communication pattern file\"!\n");
      printUsage(argv);
      abort();
   }

   tm_set_verbose_level(verbose_level);
   
   return test_mapping(arch_filename, arch_file_type, com_filename, bind_flag, bind_filename,
		       opt_topo_flag, metric, oversub_fact, display_solution_flag);

}
