#include "orgstructure_module.hpp"
#include "agent/check_isomorphism_agent.hpp"
SC_MODULE_REGISTER(OrgStructureModule)
  ->Agent<CheckIsomorphismAgent>();
