# include  <iverilog/ivl_target.h>
# include "aiger.h"
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

// Logging
#define LOG_LEVEL_DEBUG0
#ifdef  LOG_LEVEL_DEBUG0
#define DEBUG0(...)  printf("\e[1;32m AIG TARGET DEBUG   \e[0m| \t" __VA_ARGS__)
#else
#define DEBUG0(...)
#endif
#define WARNING(...) printf("\e[1;33m AIG TARGET WARNING \e[0m| \t" __VA_ARGS__)
#define ERROR(...)   printf("\e[1;31m AIG TARGET ERROR   \e[0m| \t" __VA_ARGS__)
#define INFO(...)    printf("\e[1;34m AIG TARGET INFO    \e[0m| \t" __VA_ARGS__)


// Method Declarations
int process_scope(ivl_scope_t scope);
static int show_process(ivl_process_t net, void * x);
int show_constants(ivl_design_t des);
static int process_statements(ivl_statement_t net, int level);

// Global declarations
char file_path [40];

/******************************************************
 *      Method targetted by Icarus Verilog            *
 *****************************************************/
int target_design(ivl_design_t des)
{
  //Open File For Writing
  strcpy(file_path, ivl_design_flag(des, "-o"));
  INFO("File \"%s\" targetted for output \n",file_path);


  //Obtain Scopes from Design
  int num_scopes;
  ivl_scope_t  ** scopes, ** _scopes;
  scopes = malloc(sizeof(ivl_scope_t *));
  _scopes = scopes;
  ivl_design_roots(des,scopes,&num_scopes);
  int idx = 0;
  DEBUG0("Processing %d scopes\n",num_scopes);
  while (idx < num_scopes){
    process_scope(**scopes++);
    idx++;
  }

  // Parse Design Processes
  ivl_design_process(des,show_process,0);

  show_constants(des);

  //Close File

  //Free Scope
  free(_scopes);

  return 0;
}

int process_scope(ivl_scope_t scope){
  DEBUG0("Scope name: %s\n",ivl_scope_name(scope));

  //Iterate through signals to find output ports
  DEBUG0("Determining output pins within scope\n");
  int idx;
  for ( idx = 0; idx < ivl_scope_sigs(scope); idx++ ) {
    ivl_signal_t sig = ivl_scope_sig(scope,idx);
    switch(ivl_signal_port(sig)){
      case IVL_SIP_NONE:
        DEBUG0("Not an port signal (%s)\n",ivl_signal_name(sig));
        break;
      case IVL_SIP_INOUT:
        WARNING("Inout signal not supported(%s)\n",ivl_signal_basename(sig));
        INFO("Treating signal as input\n");
      case IVL_SIP_INPUT:
        DEBUG0("Input signal (%s)\n",ivl_signal_basename(sig));
        break;
      case IVL_SIP_OUTPUT:
        INFO("Output signal (%s)\n",ivl_signal_basename(sig));
        break;

      default:
        ERROR("Unknown signal type\n");
        return -1;
    }
  }
  return 0;
}

static int show_process(ivl_process_t net, void * x){

  process_statements(ivl_process_stmt(net),0);

  //ivl_scope_t scope = ivl_process_scope(net);
  //process_scope(&scope);

  return 0;
}

static int process_statements(ivl_statement_t net,int level){
  switch(ivl_statement_type(net)) {
    case IVL_ST_ASSIGN:
      if ( level != 0 ){
        WARNING("Blocking statements not supported\tLine: %d\tFile: %s\n",ivl_stmt_lineno(net),ivl_stmt_file(net));
        INFO("*** Treating blocking assignment as non-blocking assignment ***\n");
      }else{
        WARNING("Assign Initial Condition statement not supported\tLine: %d\tFile: %s\n",ivl_stmt_lineno(net),ivl_stmt_file(net));
        break;
      }
    case IVL_ST_ASSIGN_NB:
      DEBUG0("Non-blocking assign statement\n");
      break;
    case IVL_ST_BLOCK:
      DEBUG0("Block of some sort\n");
      break;
    case IVL_ST_WAIT:
      DEBUG0("Probably an @ process \n");
      process_statements(ivl_stmt_sub_stmt(net),++level);
      break;
    case IVL_ST_CASE:
      DEBUG0("Case statement\n");
      break;
    default:
      WARNING("Unsupported statement \t Line: %d \t File: %s \t (Error: %d)\n",ivl_stmt_lineno(net), ivl_stmt_file(net), ivl_statement_type(net));
  }
  return 0;
}

int show_constants(ivl_design_t des){
  /*
  unsigned i;

  for(i=0;i<ivl_design_consts(des);i++){
    ivl_net_const_t net= ivl_design_const(des,i);
    //ivl_variable_type_t var_type = ivl_const_type(net);
    DEBUG0("%d\tDesign Constant type \n",i);
  }
  */
  return 0;
}


