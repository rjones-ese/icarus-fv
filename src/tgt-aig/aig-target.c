# include  <iverilog/ivl_target.h>
# include "aiger.h"
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

/* -- Logging -- */
# define LOG_LEVEL_DEBUG0
//# define LUT_DEBUG 1
# ifdef  LOG_LEVEL_DEBUG0
# define DEBUG0(...)  printf("\e[1;32mIFV AIG TARGET DEBUG   \e[0m | \t" __VA_ARGS__)
# else
# define DEBUG0(...)
# endif
# define WARNING(...) printf("\e[1;33mIFV AIG TARGET WARNING \e[0m | \t" __VA_ARGS__)
# define ERROR(...)   printf("\e[1;31mIFV AIG TARGET ERROR   \e[0m | \t" __VA_ARGS__)
# define INFO(...)    printf("\e[1;34mIFV AIG TARGET INFO    \e[0m | \t" __VA_ARGS__)

# define IFV_AIGER_COMMENT "Auto-Generated by Icarus Aiger Target "\
                           "(https://github.com/rjones-ese/icarus-fv/)"

//IVL Macros
# define IVL_SIGNAL_IS_INPUT( sig ) ( IVL_SIP_INPUT == ivl_signal_port(sig) )
# define IVL_SIGNAL_IS_OUTPUT( sig ) ( IVL_SIP_OUTPUT == ivl_signal_port(sig) )

/* --  Method Declarations -- */
int process_scope(ivl_scope_t scope, void * cd);
static int show_process(ivl_process_t net, void * x);
unsigned process_statements(ivl_statement_t net, unsigned condition);
unsigned process_assignments(ivl_statement_t net, unsigned condition);
unsigned process_nexus ( ivl_signal_t parent, ivl_nexus_t nexus );
unsigned process_lpm   ( ivl_signal_t parent, ivl_lpm_t lpm );
unsigned process_logic ( ivl_signal_t parent, ivl_net_logic_t log );
unsigned process_signal( ivl_signal_t sig );
unsigned process_constant( ivl_signal_t parent, ivl_net_const_t net );
int process_pli   ( ivl_statement_t net  );

/* --  Aiger Declarations -- */
# define MAX_AIGER_LITERAL 1000
char file_path [40];
aiger * aiger_handle;
unsigned aiger_index = 0;
ivl_signal_t aiger_signals[MAX_AIGER_LITERAL];
unsigned aiger_signal_len = 0;
unsigned get_aiger_signal_index( ivl_signal_t sig );
# define IDX(x) get_aiger_signal_index(x)
# define LIT(x) aiger_var2lit(IDX(x))
# define FALSE_LIT LIT( (ivl_signal_t) NULL )


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

  //Obtain Scopes from Design
  int num_scopes;
  ivl_scope_t  ** scopes, ** _scopes;
  scopes = malloc(sizeof(ivl_scope_t *));
  _scopes = scopes;
  ivl_design_roots(des,scopes,&num_scopes);

  //Iterate through scopes
  DEBUG0("Processing %d scope%c\n",num_scopes,(num_scopes==1)?' ':'s');
  int idx = 0;
  while (idx < num_scopes){
    ret_val = process_scope(**scopes++,0);
    idx++;
  }

  // Parse Design Processes
  ivl_design_process(des,show_process,0);

  //Write file
  aiger_add_comment(aiger_handle,IFV_AIGER_COMMENT);
  INFO("Writing to file %s\n",file_path);
  if ( !aiger_open_and_write_to_file(aiger_handle, file_path) ){
    ERROR("Unable to write to file\n");
    ret_val = -1;
  }else{
    DEBUG0("Successfully wrote to file\n");
  }

  INFO("Printing lookup table\n");
  for(idx = 0; idx < aiger_signal_len; idx++ ){
    ivl_signal_t sig = aiger_signals[idx];
    if ( NULL != sig && !ivl_signal_local(sig))
      fprintf(stdout, "%30.30s\t%u\n", ivl_signal_name(sig),aiger_var2lit(idx+1));
  }

  //Free Aiger Lib
  aiger_free(aiger_handle);

  //Free Scope
  free(_scopes);
  if ( ret_val < 0 )
    return ret_val;

  return ret_val;
}

int process_scope(ivl_scope_t scope,void * cd){
  INFO("Scope name: %s\n",ivl_scope_name(scope));

  int idx;
  unsigned input_lit, output_lit, ret_lit;

  //Iterate through signals to find output ports
  for ( idx = 0; idx < ivl_scope_sigs(scope); idx++ ) {
    ivl_nexus_t nexus;
    ivl_signal_t sig = ivl_scope_sig(scope,idx);
    if ( IVL_SIGNAL_IS_OUTPUT( sig ) ){
        output_lit = LIT(sig);
        aiger_add_output(aiger_handle,output_lit,ivl_signal_name(sig));
        INFO("Output signal (%s)\n",ivl_signal_name(sig));
        nexus = ivl_signal_nex(sig,0);
        ret_lit = process_nexus(sig,nexus);
        aiger_add_and( aiger_handle, output_lit, ret_lit, ret_lit );
    }
  }

  //Iterate through signals to find input ports
  for ( idx = 0; idx < ivl_scope_sigs(scope); idx++ ) {
    ivl_signal_t sig = ivl_scope_sig(scope,idx);
    if ( IVL_SIGNAL_IS_INPUT( sig ) ){
        input_lit = LIT(sig);
        aiger_add_input(aiger_handle,input_lit,ivl_signal_name(sig));
        INFO("Input signal %s (@%u)\n",ivl_signal_name(sig),input_lit);
    }
  }
  ivl_scope_children(scope,process_scope,0);

  return 0;
}

unsigned process_nexus( ivl_signal_t parent, ivl_nexus_t nexus ){

  // Lock other processes out of this node once it is solved
  if ( ivl_nexus_get_private(nexus)){ return 0; }
  ivl_nexus_set_private( nexus,(void * ) 1);

  // Iterate through nexus pointers to try to find driving element
  int idx;
  ivl_nexus_ptr_t nexus_ptr;

  for ( idx = 0; idx < ivl_nexus_ptrs(nexus); idx++ ){
    nexus_ptr = ivl_nexus_ptr(nexus,idx);

    ivl_switch_t nex_switch = ivl_nexus_ptr_switch(nexus_ptr);
    ivl_branch_t nex_branch = ivl_nexus_ptr_branch(nexus_ptr);

    if( 0 != nex_switch )
      WARNING("Switch unimplimented %s [file %s line number %d] \n",ivl_switch_basename(nex_switch),ivl_switch_file(nex_switch),ivl_switch_lineno(nex_switch));
    else if( 0 != nex_branch )
      WARNING("Branch unimplimented \n");

#define RETURN_IF_VALID(x) if (x > 0) return x
    unsigned con_lit =process_constant( parent, ivl_nexus_ptr_con(nexus_ptr));
    RETURN_IF_VALID(con_lit);
    unsigned sig_lit =process_signal( ivl_nexus_ptr_sig (nexus_ptr) );
    RETURN_IF_VALID(sig_lit);
    unsigned log_lit =process_logic ( parent, ivl_nexus_ptr_log (nexus_ptr) );
    RETURN_IF_VALID(log_lit);
    unsigned lpm_lit =process_lpm   ( parent, ivl_nexus_ptr_lpm (nexus_ptr) );
    RETURN_IF_VALID(lpm_lit);
#undef RETURN_IF_VALID

  }
  //Should not be reached
  return 0;
}

unsigned process_lpm( ivl_signal_t parent, ivl_lpm_t lpm ){
  if ( 0 == lpm )
    return 0;
  ERROR("LPM Devices are not enabled\n");
  return 0;
}

unsigned process_logic(ivl_signal_t parent, ivl_net_logic_t log ){
  int idx;
  if ( 0 == log )
    return 0;

  unsigned lit, false_lit, false_lit2, false_lit3, ret_lit, ret_lit2;
  lit = LIT(parent);

  switch ( ivl_logic_type(log)){

    // Implemented logical functions
    case IVL_LO_BUF    :
    case IVL_LO_BUFIF0 :
    case IVL_LO_BUFIF1 :
    case IVL_LO_BUFZ   :
      ret_lit = process_nexus( parent, ivl_logic_pin(log,1));
      DEBUG0("Buffer (@%u) getting literal %u\n",lit,ret_lit);
      return ret_lit;
    case IVL_LO_NOT    :
      ret_lit = aiger_not(process_nexus( parent, ivl_logic_pin(log,1)));
      false_lit = FALSE_LIT;
      DEBUG0("NOT (@%u) getting literal %u\n",lit,ret_lit);
      aiger_add_and( aiger_handle, false_lit, ret_lit , ret_lit );
      return false_lit;
    case IVL_LO_AND    :
      //DEBUG0("And pins %d\n", ivl_logic_pins(log) );
      false_lit = FALSE_LIT;
      ret_lit = process_nexus( parent, ivl_logic_pin(log,1));
      ret_lit2 = process_nexus( parent, ivl_logic_pin(log,2));
      DEBUG0("AND (@%u) getting literals %u %u\n",lit,ret_lit,ret_lit2);
      aiger_add_and(aiger_handle, false_lit, ret_lit, ret_lit2);
      return false_lit;
    case IVL_LO_OR     :
      false_lit = FALSE_LIT;
      ret_lit =  process_nexus( parent, ivl_logic_pin(log,1));
      ret_lit2 = process_nexus( parent, ivl_logic_pin(log,2));
      DEBUG0("OR (@%u) getting literals %u %u\n",lit,ret_lit,ret_lit2);
      aiger_add_and(aiger_handle, false_lit, ret_lit, ret_lit2);
      return aiger_not(false_lit);
    case IVL_LO_XOR    :
      false_lit = FALSE_LIT;
      false_lit2 = FALSE_LIT;
      false_lit3 = FALSE_LIT;
      ret_lit =  process_nexus( parent, ivl_logic_pin(log,1));
      ret_lit2 = process_nexus( parent, ivl_logic_pin(log,2));

      aiger_add_and( aiger_handle, false_lit2, ret_lit, aiger_not(ret_lit2) );
      aiger_add_and( aiger_handle, false_lit3, aiger_not(ret_lit), ret_lit2 );
      aiger_add_and( aiger_handle, false_lit, aiger_not(false_lit2),aiger_not(false_lit3));

      DEBUG0("XOR (@%u) getting literals %u %u\n",lit,ret_lit,ret_lit2);
      return aiger_not(false_lit);

    // Unimplemented logical functions
    case IVL_LO_NONE   :
    case IVL_LO_CMOS   :
    case IVL_LO_NAND   :
    case IVL_LO_NMOS   :
    case IVL_LO_NOR    :
    case IVL_LO_NOTIF0 :
    case IVL_LO_NOTIF1 :
    case IVL_LO_PULLDOWN:
    case IVL_LO_PULLUP :
    case IVL_LO_RCMOS  :
    case IVL_LO_RNMOS  :
    case IVL_LO_RPMOS  :
    case IVL_LO_PMOS   :
    case IVL_LO_XNOR   :
    default:
      WARNING("Unsupported logic element:\t%s\n",ivl_logic_basename(log));
  }
  return lit;
}

unsigned process_constant( ivl_signal_t parent, ivl_net_const_t net ){
  if( 0 == net)
    return 0;
  return '1' == ivl_const_bits(net)[0];
}

unsigned process_signal( ivl_signal_t sig ){
  if ( 0 == sig )
    return 0;

  unsigned lit = LIT(sig);
  unsigned ret_lit;

  DEBUG0("Processing signal %s (@%u)\n",ivl_signal_basename(sig),lit);
  if ( IVL_SIGNAL_IS_INPUT( sig ) ){
      return lit;
  }

  if ( ivl_signal_type(sig) == IVL_SIT_REG ){
    DEBUG0("Reg @ %u\n",lit);
    return lit;
  }

  ret_lit = process_nexus(sig, ivl_signal_nex(sig,0));
  return ret_lit;
}
static int show_process(ivl_process_t net, void * x){
  return process_statements(ivl_process_stmt(net),0);
}
unsigned process_assignments(ivl_statement_t net, unsigned condition){
  ivl_signal_t lsig,rsig;
  unsigned llit,rlit;
  ivl_expr_t rexpr;
  lsig = ivl_lval_sig(ivl_stmt_lval(net,0));
  llit = LIT(lsig);
  DEBUG0("LHS Signal %s (@%u)\n",ivl_signal_basename(lsig),llit);

  rexpr = ivl_stmt_rval( net );
  switch ( ivl_expr_type( rexpr )){
    case IVL_EX_SIGNAL:
      rsig = ivl_expr_signal(rexpr);
      //rlit = LIT(rsig);
      DEBUG0("RHS expression signal %s (@%u)\n",ivl_signal_basename(rsig),rlit);
      rlit = process_signal(rsig);
      if (condition){
        unsigned false_lit = FALSE_LIT;
        aiger_add_and( aiger_handle, false_lit, rlit, condition);
        aiger_add_latch(aiger_handle, llit, false_lit, ivl_signal_name(lsig));
      }else{
        aiger_add_latch(aiger_handle, llit, rlit,ivl_signal_name(lsig));
      }
      return rlit;

    case IVL_EX_BINARY:
      WARNING("Unsupported RHS expression binary\n");
      break;
    case IVL_EX_TERNARY:
      WARNING("Unsupported RHS ternary expression\n");
      break;
    default:
      WARNING("Unsupported RHS expression %d\n", ivl_expr_type( ivl_stmt_rval(net) ));
  }

  return 0;
}
unsigned process_statements(ivl_statement_t net, unsigned condition){

  ivl_expr_t expr;
  unsigned condition_lit, false_lit, false_lit2;
  ivl_statement_t next_net;

  switch(ivl_statement_type(net)) {
    case IVL_ST_ASSIGN:
      WARNING("Blocking statements not supported \t\t\tLine: %d\tFile: %s\n",ivl_stmt_lineno(net),ivl_stmt_file(net));
      INFO("*** Treating blocking assignment as non-blocking assignment ***\n");
      //WARNING("Assign Initial Condition statement not supported\tLine: %d\tFile: %s\n",ivl_stmt_lineno(net),ivl_stmt_file(net));
    case IVL_ST_ASSIGN_NB:
      DEBUG0("Non-blocking assign statement\n");
      return process_assignments(net, condition);
    case IVL_ST_BLOCK:
      DEBUG0("Block of some sort\n");
      break;
    case IVL_ST_WAIT:
      DEBUG0("Probably an @ process \n");
      process_statements(ivl_stmt_sub_stmt(net),0);
      break;
    case IVL_ST_CASE:
      DEBUG0("Case statement\n");
      break;
    case IVL_ST_CONDIT:
      DEBUG0("Conditional statement\n");
      WARNING("Conditional statements only working for if clause right now\n");
      expr = ivl_stmt_cond_expr(net);
      condition_lit = process_signal(ivl_expr_signal(expr));
      false_lit = FALSE_LIT;
      process_statements( ivl_stmt_cond_true(net), condition_lit );
      if(condition){ //append another condition
        aiger_add_and( aiger_handle, false_lit, condition_lit, condition );
      }
      next_net = ivl_stmt_cond_false(net);
      if (0 != next_net)
        process_statements( next_net, aiger_not(condition_lit));
      break;
    case IVL_ST_STASK:
      process_pli(net);
      break;
    default:
      WARNING("Unsupported statement \t Line: %d \t File: %s \t (Error: %d)\n",ivl_stmt_lineno(net), ivl_stmt_file(net), ivl_statement_type(net));
  }
  return 0;
}

int process_pli ( ivl_statement_t net ){
  DEBUG0("PLI %s \n",ivl_stmt_name(net));
  ivl_expr_t predicate_expr, predicate_value_expr, desc_expr;
  int idx, func_idx;
  ivl_signal_t predicate, predicate_value;
  unsigned predicate_lit, predicate_value_lit;

  char * function_description;

  const char * pli_funcs[] = { "$aig_bad", "$aig_constraint", "$aig_justice", "$aig_fairness" };
  # define AIGBAD  0
  # define AIGCON  1
  # define AIGJUS  2
  # define AIGFAIR 3

  func_idx = -1;
  for ( idx = 0; idx < 4 && func_idx < 0; idx++ ){
    if(!strcmp(ivl_stmt_name(net),pli_funcs[idx]))
        func_idx = idx;
  }
  if ( func_idx < 0 ){
    ERROR("PLI Not a valid function name \"%s\"\n",ivl_stmt_name(net));
    return -1;
  }

  //Check for correct number of arguments
  if ( ivl_stmt_parm_count(net) != 2 ){
    ERROR("PLI %s Expects two arguments: predicate (as wire), and description (as string)\n",ivl_stmt_name(net));
    return -1;
  }

  predicate_expr = ivl_stmt_parm(net,0);
  desc_expr = ivl_stmt_parm(net,1);
  //predicate_value_expr = ivl_stmt_parm(net,1);

  //Check types
  if ( ivl_expr_type(predicate_expr) != IVL_EX_SIGNAL ){
    ERROR("PLI %s Expects first argument (predicate) to be wire\n",ivl_stmt_name(net));
    return -2;
  }

  if ( ivl_expr_type(desc_expr) != IVL_EX_STRING ){
    ERROR("PLI %s Expects second argument (description) to be string\n",ivl_stmt_name(net));
    return -2;
  }

  //Get Signals From Expressions
  predicate = ivl_expr_signal(predicate_expr);

  //Propogate signals down (may be redundant)
  process_signal(predicate);

  //Get literal indexes
  predicate_lit = LIT(predicate);

  switch ( func_idx ){
    case AIGBAD:
      aiger_add_bad( aiger_handle, predicate_lit, ivl_expr_string(desc_expr) );
      break;
    case AIGCON:
      aiger_add_constraint( aiger_handle, predicate_lit, ivl_expr_string(desc_expr) );
      break;
    case AIGJUS:
      break;
    case AIGFAIR:
      aiger_add_fairness( aiger_handle, predicate_lit, ivl_expr_string(desc_expr) );
      break;
    default:
      ERROR("PLI Internal Error: Cannot process function %s\n",ivl_stmt_name(net));
      return -1;
  }

  return 0;
}

#ifdef LUT_DEBUG
#define DEBUG_LUT(...) DEBUG0("LUT: "__VA_ARGS__) 
#else
#define DEBUG_LUT(...)
#endif

unsigned get_aiger_signal_index( ivl_signal_t sig ){
  int idx;

  //Return index of signal if already stored in array
  if( NULL != sig ){
    for(idx=0;idx<aiger_signal_len;idx++){
      if ( NULL != aiger_signals[idx] && 0 == strcmp( ivl_signal_basename(sig), ivl_signal_basename( aiger_signals[idx] ))){
          DEBUG_LUT("LUT: Returning %s @ index %d\n",ivl_signal_basename(sig),idx+1);
          return idx+1;
      }
    }
  }

  //Quit if array has exceeded max
  //TODO Dynamically allocate more memory
  if ( aiger_signal_len == MAX_AIGER_LITERAL ){
    ERROR("LUT: Maximum signals exceeded\n");
    return 0;
  }

  //Add element to array
  aiger_signals [aiger_signal_len++] = sig;
  DEBUG_LUT("LUT: Adding signal %s at index %d\n",(NULL == sig)? "FALSE LIT":ivl_signal_basename(sig),aiger_signal_len);

  return aiger_signal_len;
}
