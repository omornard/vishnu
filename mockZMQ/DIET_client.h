//
// DIET_client.h
// Mock de la couche DIET par ZMQ dans VISHNU pour UMS
// Le 02/05/2012
// Auteur K. COULOMB
//

#ifndef __DIETMOCK__
#define __DIETMOCK__

#include "mdcliapi.hpp"
#include <string>
#include <boost/shared_ptr.hpp>

typedef struct diet_profile_t {
  char ** param;
  int IN;
  int INOUT;
  int OUT;
  char* name;
}diet_profile_t;

typedef struct diet_arg_t {
  int pos;
  diet_profile_t* prof;
}diet_arg_t;

#define DIET_VOLATILE 1

diet_profile_t*
diet_profile_alloc(const char* name, int IN, int INOUT, int OUT);

int
diet_string_set(diet_arg_t* arg, char* value, int pers);

int
diet_call(diet_profile_t* prof);

int
diet_string_get(diet_arg_t* arg, char** value, void* ptr);

int
diet_profile_free(diet_profile_t* prof);

diet_arg_t*
diet_parameter(diet_profile_t* prof, int pos);


boost::shared_ptr<diet_profile_t>
my_deserialize(const std::string& prof);

std::string
my_serialize(diet_profile_t* prof);

int
diet_initialize(const char* cfg, int argc, char** argv);

int
diet_finalize();

#endif // __DIETMOCK__