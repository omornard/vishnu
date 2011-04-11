#include <string>
#include <iostream>
#include "JobOutPutProxy.hpp"
#include "utilsClient.hpp"
#include "utilsTMSClient.hpp"
#include "emfTMSUtils.hpp"


using namespace vishnu;

/**
* \param session The object which encapsulates the session information
* \param machine The object which encapsulates the machine information
* \param jobResult The job results data structure
* \param listJobResults the list of job results data structure
* \brief Constructor
*/
JobOutPutProxy::JobOutPutProxy( const SessionProxy& session,
                  const std::string& machineId)
:msessionProxy(session), mmachineId(machineId) {
}

/**
* \brief Function to get the job results
* \param jobId The Id of the
* \return The job results data structure
*/
TMS_Data::JobResult_ptr
JobOutPutProxy::getJobOutPut(const std::string& jobId) {

  diet_profile_t* profile = NULL;
  std::string sessionKey;
  char* jobResultToString;
  char* jobResultInString;
  char* errorInfo = NULL;
  TMS_Data::JobResult jobResult;
  jobResult.setJobId(jobId);

  std::string serviceName = "jobOutPutGetResult_";
  serviceName.append(mmachineId);

  profile = diet_profile_alloc(serviceName.c_str(), 2, 2, 4);
  sessionKey = msessionProxy.getSessionKey();

  std::string msgErrorDiet = "call of function diet_string_set is rejected ";
   //IN Parameters
  if (diet_string_set(diet_parameter(profile,0), strdup(sessionKey.c_str()), DIET_VOLATILE)) {
    msgErrorDiet += "with sessionKey parameter "+sessionKey;
    raiseDietMsgException(msgErrorDiet);
  }

  if (diet_string_set(diet_parameter(profile,1), strdup(mmachineId.c_str()), DIET_VOLATILE)) {
    msgErrorDiet += "with machineId parameter "+mmachineId;
    raiseDietMsgException(msgErrorDiet);
  }

   const char* name = "getJobOutPut";
  ::ecorecpp::serializer::serializer _ser(name);
  //To serialize the options object in to optionsInString
  jobResultToString =  strdup(_ser.serialize(const_cast<TMS_Data::JobResult_ptr>(&jobResult)).c_str());

  if (diet_string_set(diet_parameter(profile,2), jobResultToString, DIET_VOLATILE)) {
    msgErrorDiet += "with the job result parameter "+std::string(jobResultToString);
    raiseDietMsgException(msgErrorDiet);
  }

   //OUT Parameters
  diet_string_set(diet_parameter(profile,3), NULL, DIET_VOLATILE);
  diet_string_set(diet_parameter(profile,4), NULL, DIET_VOLATILE);

  if(!diet_call(profile)) {
    if(diet_string_get(diet_parameter(profile,3), &jobResultInString, NULL)){
      msgErrorDiet += " by receiving User serialized  message";
      raiseDietMsgException(msgErrorDiet);
    }
    if(diet_string_get(diet_parameter(profile,4), &errorInfo, NULL)){
      msgErrorDiet += " by receiving errorInfo message";
      raiseDietMsgException(msgErrorDiet);
    }
  }
  else {
    raiseDietMsgException("DIET call failure");
  }

  /*To raise a vishnu exception if the receiving message is not empty*/
  TMSUtils::raiseTMSExceptionIfNotEmptyMsg(errorInfo);

  TMS_Data::JobResult_ptr outJobResult;
  //To parse JobResult object serialized
  if (!vishnu::parseTMSEmfObject(std::string(jobResultInString), outJobResult, "Error when receiving JobResult object serialized")) {
    throw UserException(ERRCODE_INVALID_PARAM);
  }

  return outJobResult;
}

/**
* \brief Function to get the results of all job submitted
* \return The list of the job results
*/

TMS_Data::ListJobResults_ptr
JobOutPutProxy::getAllJobsOutPut() {
  diet_profile_t* profile = NULL;
  std::string sessionKey;
  char* listJobResultInString = NULL;
  char* errorInfo = NULL;
  TMS_Data::ListJobResults_ptr listJobResults_ptr = NULL;

  std::string serviceName = "jobOutPutGetAllResult_";
  serviceName.append(mmachineId);

  profile = diet_profile_alloc(serviceName.c_str(), 1, 1, 3);
  sessionKey = msessionProxy.getSessionKey();

  std::string msgErrorDiet = "call of function diet_string_set is rejected ";
   //IN Parameters
  if (diet_string_set(diet_parameter(profile,0), strdup(sessionKey.c_str()), DIET_VOLATILE)) {
    msgErrorDiet += "with sessionKey parameter "+sessionKey;
    raiseDietMsgException(msgErrorDiet);
  }

  if (diet_string_set(diet_parameter(profile,1), strdup(mmachineId.c_str()), DIET_VOLATILE)) {
    msgErrorDiet += "with machineId parameter "+mmachineId;
    raiseDietMsgException(msgErrorDiet);
  }

   //OUT Parameters
  diet_string_set(diet_parameter(profile,3), NULL, DIET_VOLATILE);
  diet_string_set(diet_parameter(profile,4), NULL, DIET_VOLATILE);

  if(!diet_call(profile)) {
    if(diet_string_get(diet_parameter(profile,3), &listJobResultInString, NULL)){
      msgErrorDiet += " by receiving User serialized  message";
      raiseDietMsgException(msgErrorDiet);
    }
    if(diet_string_get(diet_parameter(profile,4), &errorInfo, NULL)){
      msgErrorDiet += " by receiving errorInfo message";
      raiseDietMsgException(msgErrorDiet);
    }
  }
  else {
    raiseDietMsgException("DIET call failure");
  }

  /*To raise a vishnu exception if the receiving message is not empty*/
  TMSUtils::raiseTMSExceptionIfNotEmptyMsg(errorInfo);

  //To parse ListJobResult object serialized
  if (!vishnu::parseTMSEmfObject(std::string(listJobResultInString), listJobResults_ptr, "Error when receiving ListJobResult object serialized")) {
    throw UserException(ERRCODE_INVALID_PARAM);
  }

  return listJobResults_ptr;
}

/**
* \brief Destructor
*/

JobOutPutProxy::~JobOutPutProxy() {
}
