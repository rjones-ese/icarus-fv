# include  <iverilog/ivl_target.h>
# include <stdio.h>
# include <stdlib.h>

#define DEBUG0(...) printf("\e[1;34m AIG TARGET DEBUG:\e[0m \t" __VA_ARGS__)
#define WARNING(...) printf("\e[0;33m AIG TARGET WARNING:\e[0m \t" __VA_ARGS__)


int process_scope(ivl_scope_t * scope);
static int show_process(ivl_process_t net, void * x);
int show_constants(ivl_design_t des);
static int process_statements(ivl_statement_t net, int level);

int target_design(ivl_design_t des)
{
  ivl_scope_t  ** scopes;
  scopes = malloc(sizeof(ivl_scope_t *));
  ivl_scope_t scope;
  int num_scopes;
  const char * bleh;
  DEBUG0("Design targeted for AIG\n");

  ivl_design_roots(des,scopes,&num_scopes);
  DEBUG0("Processing %d scopes\n",num_scopes);

  int i = 0;
  while (i < num_scopes){
    process_scope(*scopes++);
    i++;
  }

  ivl_design_process(des,show_process,0);

  show_constants(des);

  return 0;
}

int process_scope(ivl_scope_t * scope){
  DEBUG0("Scope name: %s\n",ivl_scope_name(*scope));
  return 0;
}
/*
      IVL_ST_NONE    = 0,
      IVL_ST_NOOP    = 1,
      IVL_ST_ALLOC   = 25,
      IVL_ST_ASSIGN    = 2,
      IVL_ST_ASSIGN_NB = 3,
      IVL_ST_BLOCK   = 4,
      IVL_ST_CASE    = 5,
      IVL_ST_CASER   = 24, // Case statement with real expressions.
      IVL_ST_CASEX   = 6,
      IVL_ST_CASEZ   = 7,
      IVL_ST_CASSIGN = 8,
      IVL_ST_CONDIT  = 9,
      IVL_ST_CONTRIB = 27,
      IVL_ST_DEASSIGN = 10,
      IVL_ST_DELAY   = 11,
      IVL_ST_DELAYX  = 12,
      IVL_ST_DISABLE = 13,
      IVL_ST_FORCE   = 14,
      IVL_ST_FOREVER = 15,
      IVL_ST_FORK    = 16,
      IVL_ST_FREE    = 26,
      IVL_ST_RELEASE = 17,
      IVL_ST_REPEAT  = 18,
      IVL_ST_STASK   = 19,
      IVL_ST_TRIGGER = 20,
      IVL_ST_UTASK   = 21,
      IVL_ST_WAIT    = 22,
      IVL_ST_WHILE   = 23
*/
static int show_process(ivl_process_t net, void * x){

  process_statements(ivl_process_stmt(net),0);

  //ivl_scope_t scope = ivl_process_scope(net);
  //process_scope(&scope);

  return 0;
}

static int process_statements(ivl_statement_t net,int level){
  switch(ivl_statement_type(net)) {
    case IVL_ST_ASSIGN:
      if ( level != 0 ) // Probably an initial condition
        DEBUG0("Assign Statement\n");
      else
        DEBUG0("Assign Initial Condition statement\tLine: %d\tFile: %s\n",ivl_stmt_lineno(net),ivl_stmt_file(net));
      break;
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
  unsigned i;

  for(i=0;i<ivl_design_consts(des);i++){
    ivl_net_const_t net_const = ivl_design_const(des,i);
    //ivl_variable_type_t var_type = ivl_const_type(net_const);
    DEBUG0("%d\tDesign Constant type \n",i);
  }
  return 0;
}


