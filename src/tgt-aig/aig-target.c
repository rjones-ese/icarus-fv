# include  <iverilog/ivl_target.h>
# include "aiger.h"
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

// Logging
# define LOG_LEVEL_DEBUG0
# ifdef  LOG_LEVEL_DEBUG0
# define DEBUG0(...)  printf("\e[1;32mIFV AIG TARGET DEBUG   \e[0m | \t" __VA_ARGS__)
# else
# define DEBUG0(...)
# endif
# define WARNING(...) printf("\e[1;33mIFV AIG TARGET WARNING \e[0m | \t" __VA_ARGS__)
# define ERROR(...)   printf("\e[1;31mIFV AIG TARGET ERROR   \e[0m | \t" __VA_ARGS__)
# define INFO(...)    printf("\e[1;34mIFV AIG TARGET INFO    \e[0m | \t" __VA_ARGS__)

# define IFV_AIGER_COMMENT "Auto-Generated by Icarus Formal Verification Target "\
                           "(https://github.com/rjones-ese/icarus-fv/)"

//Other useful macros
# define IVL_SIGNAL_IS_INPUT( sig ) ( IVL_SIP_INPUT == ivl_signal_port(sig) )
# define IVL_SIGNAL_IS_OUTPUT( sig ) ( IVL_SIP_OUTPUT == ivl_signal_port(sig) )


/* --  Method Declarations -- */
int process_scope(ivl_scope_t scope, void * cd);
static int show_process(ivl_process_t net, void * x);
int show_constants(ivl_design_t des);
static int process_statements(ivl_statement_t net, int level);

// Aggregates outputs of top level scope
//    - Requires: an ( already allocated ) array of signals and
//      the maximum value that the array can hold.
//    - Returns: -1 if number of outputs exceed array or the 
//      number of outputs in the scope
int aggregate_outputs ( ivl_signal_t * output, unsigned max );

// Global declarations
char file_path [40];
aiger * aiger_handle;
unsigned aiger_index = 0;

// Literal Lookup Table Declarations

// Literal type
typedef struct lit_s {
  unsigned index;
  ivl_signal_t signal;
} lit_t;

// Crude lookup table structure
struct lit_lut_s {
  lit_t * literals;
  lit_t * literal_head;
  unsigned literal_len;
  unsigned literal_max;
};

//Global table
struct lit_lut_s lit_lut;

//Lookup table methods
int          lit_init     ( void );
void         lit_free     ( void );
unsigned     lit_add      ( const char * name );
unsigned     lit_get_index( const char * name );
const char * lit_get_name ( unsigned idx );
void         lit_iterate  ( void (* lit_iterate_cb ) ( unsigned idx, ivl_signal_t * sig ));
int process_nexus( ivl_nexus_t nexus );

#define MAX_LIT 100

int lit_init ( void ){
  lit_lut.literals = malloc( MAX_LIT * sizeof(lit_t));
  lit_lut.literal_head = lit_lut.literals;
  lit_lut.literal_max = MAX_LIT;
  return 0;
}

void lit_free ( void ){
  free(lit_lut.literals);
}

unsigned lit_add ( const char * name ){
  if ( lit_lut.literal_len < lit_lut.literal_max ){
    return lit_lut.literal_len++;
  }
  return 0;
}


/******************************************************
 *      Method targetted by Icarus Verilog            *
 *****************************************************/
int target_design(ivl_design_t des)
{
  int ret_val;

  //Open File For Writing
  strcpy(file_path, ivl_design_flag(des, "-o"));
  INFO("File \"%s\" targetted for output \n",file_path);

  //Initialize Aiger Library
  aiger_handle = aiger_init();

  //Init Lookup Table
  lit_init();

  //Obtain Scopes from Design
  int num_scopes;
  ivl_scope_t  ** scopes, ** _scopes;
  scopes = malloc(sizeof(ivl_scope_t *));
  _scopes = scopes;
  ivl_design_roots(des,scopes,&num_scopes);

  DEBUG0("Processing %d scope%c\n",num_scopes,(num_scopes==1)?' ':'s');
  int idx = 0;
  while (idx < num_scopes){
    ret_val = process_scope(**scopes++,0);
    idx++;
  }

  // Parse Design Processes
  ivl_design_process(des,show_process,0);

  show_constants(des);

  //Write file
  aiger_add_comment(aiger_handle,IFV_AIGER_COMMENT);
  INFO("Writing to file %s\n",file_path);
  if ( !aiger_open_and_write_to_file(aiger_handle, file_path) ){
    ERROR("Unable to write to file\n");
    ret_val = -1;
  }else{
    DEBUG0("Successfully wrote to file\n");
  }

  //Free Aiger Lib
  aiger_free(aiger_handle);

  //Free Scope
  free(_scopes);

  //Free Lookup Table
  lit_free();

  return ret_val;
}

int process_scope(ivl_scope_t scope,void * cd){
  INFO("Scope name: %s\n",ivl_scope_name(scope));

  //Iterate through signals to find output ports
  DEBUG0("Determining output pins within scope\n");
  int idx;

  for ( idx = 0; idx < ivl_scope_sigs(scope); idx++ ) {
    ivl_nexus_t nexus;
    ivl_signal_t sig = ivl_scope_sig(scope,idx);
    if ( IVL_SIGNAL_IS_OUTPUT( sig ) ){
        INFO("Output signal (%s)\n",ivl_signal_name(sig));
        nexus = ivl_signal_nex(sig,0);
        process_nexus(nexus);
    }
  }


  /*
  ivl_statement_t stmt_net = ivl_scope_def(scope);
  if ( stmt_net ) {
    switch(ivl_statement_type( stmt_net )) {
      case IVL_ST_ASSIGN_NB:
        DEBUG0("Non-blocking assign statement\n");
        break;
      case IVL_ST_BLOCK:
        DEBUG0("Block of some sort\n");
        break;
      case IVL_ST_WAIT:
        DEBUG0("Probably an @ process \n");
        break;
      case IVL_ST_CASE:
        DEBUG0("Case statement\n");
        break;
      default:
        break;
        WARNING("Unsupported statement \t Line: %d \t File: %s \t (Error: %d)\n",ivl_stmt_lineno(stmt_net), ivl_stmt_file(stmt_net), ivl_statement_type(stmt_net));
    }
  }
  int idx;
  //Get switches from scope
  DEBUG0("Getting %d switches from scope\n",ivl_scope_switches(scope));
  for ( idx = 0; idx < ivl_scope_switches(scope); idx++ ){
    DEBUG0("%d event\n",idx);
  }
  //Get logic from scope
  DEBUG0("Getting %d logic devices from scope\n",ivl_scope_logs(scope));
  for ( idx = 0; idx < ivl_scope_logs(scope); idx++ ){
    DEBUG0("%d logic %s\n",idx,ivl_logic_basename(ivl_scope_log(scope,idx)));
  }
  //Iterate through signals to find output ports
  DEBUG0("Determining output pins within scope\n");
  DEBUG0("IVL Scope %s\n",ivl_scope_tname(scope));
  for ( idx = 0; idx < ivl_scope_sigs(scope); idx++ ) {
    ivl_signal_t sig = ivl_scope_sig(scope,idx);
    switch(ivl_signal_port(sig)){
      case IVL_SIP_NONE:
        DEBUG0("Not a port signal (%s)\n",ivl_signal_name(sig));
        break;
      case IVL_SIP_INOUT:
        WARNING("Inout signal not supported(%s)\n",ivl_signal_basename(sig));
        INFO("***Treating signal as input***\n");
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
  */
  ivl_scope_children(scope,process_scope,0);

  return 0;
}

int process_nexus( ivl_nexus_t nexus ){
  INFO("Processing Nexus\n");
  int idx, ptrs;
  ivl_nexus_ptr_t nexus_ptr;
  ivl_signal_t nex_signal;
  ivl_net_logic_t nex_logic;
  ivl_switch_t nex_switch;
  ivl_branch_t nex_branch;
  ivl_lpm_t nex_lpm;

  ptrs = ivl_nexus_ptrs(nexus);
  DEBUG0("Num Nexus of Pointers %d\n",ptrs);

  for ( idx = 0; idx < ptrs; idx++ ){
    nexus_ptr = ivl_nexus_ptr(nexus,idx);

    nex_signal = ivl_nexus_ptr_sig(nexus_ptr);
    nex_logic  = ivl_nexus_ptr_log(nexus_ptr);
    nex_switch = ivl_nexus_ptr_switch(nexus_ptr);
    nex_branch = ivl_nexus_ptr_branch(nexus_ptr);
    nex_lpm    = ivl_nexus_ptr_lpm(nexus_ptr);

    /*
    DEBUG0("Ptr %d %d\n",
    ivl_nexus_ptr_pin( nexus_ptr ),
    ivl_nexus_ptr_con( nexus_ptr )
    );
    */

    if( 0 != nex_signal ) //Pointer is a signal object
      DEBUG0("Ptr Signal %s\n", ivl_signal_name(nex_signal));
    else if( 0 != nex_logic ) //Pointer is a logic object
      DEBUG0("Ptr Logic %s\n", ivl_logic_name(nex_logic));
    else if( 0 != nex_switch )
      DEBUG0("Ptr Switch %s\n",ivl_switch_basename(nex_switch));
    else if( 0 != nex_branch )
      DEBUG0("Ptr Branch\n");
    else if( 0 != nex_lpm )
      DEBUG0("Ptr LPM%s\n",ivl_lpm_basename(nex_lpm));

  }
  int num_nexus_ptrs = ivl_nexus_ptrs(nexus);
  DEBUG0("Nexus PTRS %d\n",num_nexus_ptrs);

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
        WARNING("Blocking statements not supported \t\t\tLine: %d\tFile: %s\n",ivl_stmt_lineno(net),ivl_stmt_file(net));
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


