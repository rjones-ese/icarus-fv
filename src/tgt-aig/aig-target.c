# include  <iverilog/ivl_target.h>
# include <stdio.h>
# include <stdlib.h>

int process_scope(ivl_scope_t * scope);

int target_design(ivl_design_t des)
{
  ivl_scope_t  ** scopes;
  scopes = malloc(sizeof(ivl_scope_t *));
  ivl_scope_t scope;
  int num_scopes;
  const char * bleh;
  printf("Design targeted for AIG\n");

  ivl_design_roots(des,scopes,&num_scopes);
  printf("There are %d scopes\n",num_scopes);

  int i = 0;
  while (i < num_scopes){
    process_scope(*scopes++);
    i++;
  }

  return 0;
}

int process_scope(ivl_scope_t * scope){
  printf("Scope name: %s\n",ivl_scope_name(*scope));
}
