/**
 * \file JobServer.cpp
 * \brief This file contains the VISHNU JobServer class.
 * \author Daouda Traore (daouda.traore@sysfera.com)
 * \date April 2011
 */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>
#include "JobServer.hpp"
#include "TMSVishnuException.hpp"
#include "LocalAccountServer.hpp"
#include "SSHJobExec.hpp"
#include "utilVishnu.hpp"
#include "utilServer.hpp"
#include "DbFactory.hpp"
#include "ScriptGenConvertor.hpp"
#include "api_fms.hpp"
#include "utils.hpp"
#include "BatchFactory.hpp"
#include <pwd.h>
#include <cstdlib>
#include "Logger.hpp"


/**
 * \brief Constructor
 * \param authKey The session info
 * \param machineId The machine identifier
 * \param sedConfig A pointer to the SeD configuration
 */
JobServer::JobServer(const std::string& authKey,
                     const std::string& machineId,
                     const ExecConfiguration_Ptr sedConfig)
  : mauthKey(authKey), mmachineId(machineId), msedConfig(sedConfig) {

  DbFactory factory;
  mdatabaseInstance = factory.getDatabaseInstance();

  if (msedConfig) {
    std::string value;
    msedConfig->getRequiredConfigValue<std::string>(vishnu::BATCHTYPE, value);
    mbatchType = vishnu::convertToBatchType(value);

    if (mbatchType != DELTACLOUD) {
      msedConfig->getRequiredConfigValue<std::string>(vishnu::BATCHVERSION, value);
      mbatchVersion = value;
    } else {
      mbatchVersion = "";
    }
  }
  if (! msedConfig->getConfigValue<int>(vishnu::STANDALONE, mstandaloneSed)) {
    mstandaloneSed = false;
  }
  checkMachineId(machineId);
  vishnu::validateAuthKey(mauthKey, mmachineId, mdatabaseInstance, muserSessionInfo);
}


/**
 * \brief Destructor
 */
JobServer::~JobServer() { }

/**
 * \brief Function to submit job
 * \param scriptContent the content of the script
 * \param options a json object describing options
 * \param vishnuId The VISHNU identifier
 * \param defaultBatchOption The default batch options
 * \return The resulting job ID. Raises an exception on error
 */
std::string
JobServer::submitJob(std::string& scriptContent,
                     JsonObject* options,
                     int vishnuId,
                     const std::vector<std::string>& defaultBatchOption)
{

  LOG("[INFO] Request to submit job", LogInfo);
  const std::string JOB_ID = vishnu::getObjectId(vishnuId, "formatidjob", vishnu::JOB, mmachineId);
  TMS_Data::Job jobInfo;
  jobInfo.setJobId(JOB_ID);

  LOG(boost::str(boost::format("[INFO] Job entry added %1% into the database, performing submission process...") % JOB_ID), LogInfo);

  if (scriptContent.empty()) {
    throw UserException(ERRCODE_INVALID_PARAM, "Empty script content");
  }

  try {
    int usePosix = options->getIntProperty("posix");
    if (usePosix != JsonObject::UNDEFINED_PROPERTY && usePosix != 0) {
      mbatchType = POSIX;
    }

    jobInfo.setWorkId(options->getIntProperty("scriptpath", 0));
    jobInfo.setWorkId(options->getIntProperty("workid", 0));
    setRealFilePaths(scriptContent, options, jobInfo);
    jobInfo.setSubmitMachineId(mmachineId);
    jobInfo.setStatus(vishnu::STATE_UNDEFINED);

    // the way of setting job owner varies from classical batch scheduler to cloud backend
    switch (mbatchType) {
      case OPENNEBULA:
      case DELTACLOUD:
        jobInfo.setOwner( vishnu::getVar(vishnu::CLOUD_ENV_VARS[vishnu::CLOUD_VM_USER], true, "root") );
        break;
      default:
        jobInfo.setOwner(muserSessionInfo.user_aclogin);
        break;
    }

    exportJobEnvironments(jobInfo);

    if (mstandaloneSed != 0) {
      handleNativeBatchExec(SubmitBatchAction,
                            createJobScriptExecutaleFile(scriptContent, options, defaultBatchOption),
                            options,
                            jobInfo,
                            mbatchType,
                            mbatchVersion);
    } else {
      handleSshBatchExec(SubmitBatchAction,
                         createJobScriptExecutaleFile(scriptContent, options, defaultBatchOption),
                         options,
                         jobInfo,
                         mbatchType,
                         mbatchVersion);
    }
  } catch (VishnuException& ex) {
    jobInfo.setSubmitError(ex.what());
    jobInfo.setErrorPath("");
    jobInfo.setOutputPath("");
    jobInfo.setOutputDir("");
    jobInfo.setStatus(vishnu::STATE_FAILED);
    updateJobRecordIntoDatabase(SubmitBatchAction, jobInfo);
    throw;
  }
  return JOB_ID;
}

/**
 * @brief Submit job using ssh mechanism
 * @param action action The type of action (cancel, submit...)
 * @param scriptPath The path of the script to executed
 * @param baseJobInfo The base job info
 * @param options: an object containing options
 * @param batchType The batch type
 * @param batchVersion The batch version
*/
void
JobServer::handleSshBatchExec(int action,
                              const std::string& scriptPath,
                              JsonObject* options,
                              TMS_Data::Job& baseJobInfo,
                              int batchType,
                              const std::string& batchVersion) {

  SSHJobExec sshJobExec(muserSessionInfo.user_aclogin,
                        muserSessionInfo.machine_name,
                        static_cast<BatchType>(batchType),
                        batchVersion, // ignored for POSIX backend
                        JsonObject::serialize(baseJobInfo),
                        options->encode());

  sshJobExec.setDebugLevel(mdebugLevel);
  TMS_Data::ListJobs jobSteps;

  switch(action) {
    case SubmitBatchAction:
      sshJobExec.sshexec("SUBMIT", scriptPath, jobSteps);
      // Submission with deltacloud doesn't make copy of the script
      // So the script needs to be kept until the end of the execution
      // Clean the temporary script if not deltacloud
      if (mbatchType != DELTACLOUD && mdebugLevel) {
        vishnu::deleteFile(scriptPath.c_str());
      }
      updateAndSaveJobSteps(jobSteps, baseJobInfo);
      break;
    case CancelBatchAction:
      sshJobExec.sshexec("CANCEL", "", jobSteps);
      updateJobRecordIntoDatabase(action, *(jobSteps.getJobs().get(0)));
      break;
    default:
      throw TMSVishnuException(ERRCODE_INVALID_PARAM, "unknown batch action");
      break;
  }

}

/**
 * @brief Submit job using ssh mechanism
 * @param action action The type of action (cancel, submit...)
 * @param scriptPath The path of the script to executed
 * @param options: an object containing options
 * @param jobInfo The default information provided to the job
 * @param batchType The batch type. Ignored for POSIX backend
 * @param batchVersion The batch version. Ignored for POSIX backend
*/
void
JobServer::handleNativeBatchExec(int action,
                                 const std::string& scriptPath,
                                 JsonObject* options,
                                 TMS_Data::Job& jobInfo,
                                 int batchType,
                                 const std::string& batchVersion) {
  BatchFactory factory;
  BatchServer* batchServer = factory.getBatchServerInstance(batchType, batchVersion);
  if (! batchServer) {
    throw TMSVishnuException(ERRCODE_BATCH_SCHEDULER_ERROR,
                             boost::str(boost::format("getBatchServerInstance return NULL (batch: %1%, version: %2%)")
                                        % vishnu::convertBatchTypeToString(static_cast<BatchType>(batchType))
                                        % batchVersion));
  }

  int ipcPipe[2];
  char ipcMsgBuffer[255];

  if (pipe(ipcPipe) != 0)	 {	/* Create communication pipe*/
    throw TMSVishnuException(ERRCODE_RUNTIME_ERROR, "Pipe creation failed");
  }

  pid_t pid = fork();

  if (pid < 0) {
    throw TMSVishnuException(ERRCODE_RUNTIME_ERROR, "Fork failed");
  }

  int handlerExitCode = 0;
  std::string errorMsg = "SUCCESS";
  if (pid == 0)  /** Child process */ {
    close(ipcPipe[0]);
    handlerExitCode = 0;
    // if not cloud-mode submission, switch user before running the request
    if (mbatchType != OPENNEBULA && mbatchType != DELTACLOUD) {
      handlerExitCode = setuid(getSystemUid(muserSessionInfo.user_aclogin));
      if (handlerExitCode != 0) {
        // write error message to pipe for the parent
        errorMsg = std::string(strerror(errno));
        write(ipcPipe[1], strerror(errno), errorMsg.size());
        exit(handlerExitCode);
      }
    }
    try {
      handlerExitCode = 0;
      switch(action) {
        case SubmitBatchAction: {
          // create output dir if needed
          if (! jobInfo.getOutputDir().empty()) {
            vishnu::createDir(jobInfo.getOutputDir());
          }

          // submit the job
          TMS_Data::ListJobs jobSteps;
          handlerExitCode = batchServer->submit(vishnu::copyFileToUserHome(scriptPath), options->getSubmitOptions(), jobSteps, NULL);
          updateAndSaveJobSteps(jobSteps, jobInfo);
        }
          break;
        case CancelBatchAction:
          if (mbatchType == DELTACLOUD || mbatchType == OPENNEBULA) {
            handlerExitCode = batchServer->cancel(jobInfo.getVmId());
          } else {
            handlerExitCode = batchServer->cancel(jobInfo.getBatchJobId());
          }
          jobInfo.setStatus(vishnu::STATE_CANCELLED);
          updateJobRecordIntoDatabase(action, jobInfo);
          break;
        default:
          throw TMSVishnuException(ERRCODE_INVALID_PARAM, "Unknown batch action");
          break;
      }
    } catch(const TMSVishnuException & ex) {
      handlerExitCode = ex.getTypeI();
      errorMsg = std::string(ex.what());
      LOG("[ERROR] "+ errorMsg, LogErr);
    } catch (const VishnuException & ex) {
      handlerExitCode = ex.getTypeI();
      errorMsg = std::string(ex.what());
      LOG("[ERROR] "+ errorMsg, LogErr);
    }

    // write error message to pipe for the parent
    write(ipcPipe[1], errorMsg.c_str(), errorMsg.size());
    exit(handlerExitCode);
  } else { /** Parent process*/
    close(ipcPipe[1]);
    // wait that child exists
    int exitCode;
    waitpid(pid, &exitCode, 0);

    // get possible error message send by the child
    size_t nbRead = read(ipcPipe[0], ipcMsgBuffer, 255);
    errorMsg = std::string(ipcMsgBuffer, nbRead);
    if (errorMsg != "SUCCESS") {
      throw TMSVishnuException(ERRCODE_RUNTIME_ERROR,
                               boost::str(boost::format("Job worker process exited with status %1%, message: %2%")
                                          % exitCode
                                          % errorMsg));
    }
  }
}

/**
  * \brief Function to treat the default submission options
  * \param scriptOptions The list of the option value
  * \param cmdsOptions The list of the option value
  * \return raises an exception on error
*/
void
JobServer::processDefaultOptions(const std::vector<std::string>& defaultBatchOption,
                                 std::string& content, std::string& key) {
  size_t position = 0;
  std::string key1;
  int count = 0;
  int countOptions = defaultBatchOption.size();
  while (count < countOptions) {
    key1 =  defaultBatchOption.at(count);
    position = 0;
    int found =0;
    while (position != std::string::npos && ! found) {
      position = content.find(key.c_str(), position);
      if (position != std::string::npos) {
        size_t pos1 = content.find("\n", position);
        std::string line = content.substr(position, pos1-position);
        position++;
        size_t pos2 = line.find(key1.c_str());
        if (pos2 != std::string::npos) {
          found =1;
          break;
        }
      }
    }
    if (!found) {
      std::string lineoption = key + " " + defaultBatchOption.at(count) + " " + defaultBatchOption.at(count +1) + "\n";
      insertOptionLine(lineoption, content, key);
    }
    count +=2;
  }
}
/**
  * \brief Function to insert option into string
  * \param optionLineToInsert the option to insert
  * \param content The buffer containing the inserted option
  * \return raises an exception on error
*/
void
JobServer::insertOptionLine(std::string& optionLineToInsert,
                            std::string& content, std::string& key) {
  size_t pos = 0;
  size_t posLastDirective = 0;

  while (pos!=std::string::npos) {
    pos = content.find(key.c_str(), pos);
    if (pos!=std::string::npos) {
      size_t pos1 = 0;
      pos1 = content.find("\n", pos);
      while (content.compare(pos1-1,1,"\\") == 0) {
        pos1 = content.find("\n", pos1 + 1);
      }
      std::string line = content.substr(pos, pos1-pos);
      if(content.compare(pos-1,1,"\n")==0) {
        if(mbatchType==LOADLEVELER) {
          std::string line_tolower(line);
          std::transform(line.begin(), line.end(), line_tolower.begin(), ::tolower);
          if(line_tolower.find("queue")!=std::string::npos) {
            break;
          }
        }
        posLastDirective = pos + line.size() + 1;
      }
      pos++;
    }
  }
  content.insert(posLastDirective, optionLineToInsert);
}

/**
  * \brief Function to cancel job
  * \param options Object containing options
  * \return raises an exception on error
*/
int JobServer::cancelJob(JsonObject* options)
{

  std::string jobId = options->getStringProperty("jobid");
  std::string userId = options->getStringProperty("user");

  // Only a admin user can use the option 'all' for the job id
  if (userId == ALL_KEYWORD
      && muserSessionInfo.user_privilege != vishnu::PRIVILEGE_ADMIN) {
    throw TMSVishnuException(ERRCODE_PERMISSION_DENIED,
                             boost::str(boost::format("Option privileged users can cancel all user jobs")));
  }

  // Only a admin user can delete jobs from another user
  if (! userId.empty()
      && muserSessionInfo.user_privilege != vishnu::PRIVILEGE_ADMIN) {
    throw TMSVishnuException(ERRCODE_PERMISSION_DENIED,
                             boost::str(boost::format("Only privileged users can cancel other users jobs")));
  }
  // Checking the jobid belongs to the user
  if (!jobId.empty()){
    std::string reqTmp = boost::str(boost::format("SELECT job.owner "
                                                  " FROM job, vsession "
                                                  " WHERE job.jobid='%1%' "
                                                  " AND job.vsession_numsessionid=vsession.numsessionid "
                                                  " AND vsession.sessionkey='%2%'"
                                                  )
                                    % mdatabaseInstance->escapeData(jobId)
                                    % mdatabaseInstance->escapeData(mauthKey));
    boost::scoped_ptr<DatabaseResult> sqlQueryResult(mdatabaseInstance->getResult(reqTmp));
    if (sqlQueryResult->getNbTuples() == 0 && muserSessionInfo.user_privilege != vishnu::PRIVILEGE_ADMIN) {
      throw TMSVishnuException(ERRCODE_PERMISSION_DENIED,
                               boost::str(boost::format("Only privileged users can cancel other users jobs")));
    }
  }


  bool cancelAllJobs = jobId.empty()
                       || jobId == ALL_KEYWORD
                       || userId == ALL_KEYWORD;

  std::string baseSqlQuery = boost::str(boost::format("SELECT job.owner, job.status, "
                                                      "       job.jobId, job.batchJobId, "
                                                      "       job.vmId, job.batchType"
                                                      " FROM job, vsession "
                                                      " WHERE job.status < %1%") % vishnu::STATE_COMPLETED);
  std::string sqlQuery;
  if (! cancelAllJobs) {
    sqlQuery = boost::str(boost::format("%1%"
                                        " AND vsession.numsessionid=job.vsession_numsessionid"
                                        " AND jobId='%2%';")
                          % baseSqlQuery
                          % mdatabaseInstance->escapeData(jobId));

    LOG(boost::str(boost::format("[WARN] Request to cancel job: %1%") % jobId), LogWarning);
  } else {

    // This block works as follow:
    // * if admin:
    //    ** if JobId = 'all', then cancel all jobs submitted through vishnu, regardless of the users
    //    ** else perform cancel as for a normal user
    // *if normal user (not admin), cancel alls jobs submitted through vishnu by the user
    bool addUserFilter = true;
    if (muserSessionInfo.user_privilege == vishnu::PRIVILEGE_ADMIN && userId == ALL_KEYWORD) {
      addUserFilter = false;
    }

    // Set the SQL query accordingly
    if (addUserFilter) {
      if (userId.empty()) {
        // here we'll delete all the jobs of the logged user
        sqlQuery = boost::str(boost::format("%1%"
                                            " AND vsession.numsessionid=job.vsession_numsessionid"
                                            " AND owner='%2%'"
                                            " AND submitMachineId='%3%';"
                                            )
                              % baseSqlQuery
                              % mdatabaseInstance->escapeData(muserSessionInfo.user_aclogin)
                              % mdatabaseInstance->escapeData(mmachineId));
      } else {
        // here we'll delete jobs submitted by the given user
        sqlQuery = boost::str(boost::format("SELECT job.owner, job.status, "
                                            "       job.jobId, job.batchJobId, "
                                            "       job.vmId, job.batchType"
                                            " FROM users, job"
                                            " WHERE job.status < %1%"
                                            "  AND users.numuserid=job.job_owner_id"
                                            "  AND users.userid='%2%'"
                                            "  AND submitMachineId='%3%';"
                                            )
                              % vishnu::STATE_COMPLETED
                              % mdatabaseInstance->escapeData(userId)
                              % mdatabaseInstance->escapeData(mmachineId));
      }
      LOG(boost::str(boost::format("[WARN] request to cancel all jobs submitted by %1%")
                     % userId), LogWarning);
    } else {
      sqlQuery = boost::str(boost::format("%1%"
                                          " AND vsession.numsessionid=job.vsession_numsessionid"
                                          " AND submitMachineId='%2%';")
                            % baseSqlQuery
                            % mdatabaseInstance->escapeData(mmachineId));
      LOG(boost::str(boost::format("[WARN] received request to cancel all user jobs from %1%")
                     % muserSessionInfo.user_aclogin), LogWarning);
    }
  }
  // Process the query and treat the resulting jobs
  boost::scoped_ptr<DatabaseResult> sqlQueryResult(mdatabaseInstance->getResult(sqlQuery));

  if (sqlQueryResult->getNbTuples() == 0) {
    if (! cancelAllJobs) {
      LOG(boost::str(boost::format("[INFO] invalid cancel request with job id %1%")
                     % jobId), LogInfo);
      throw TMSVishnuException(ERRCODE_UNKNOWN_JOBID, "perhaps the job is not longer running");
    } else {
      LOG("[INFO] no job matching the call", LogInfo);
    }
  } else {
    int resultCount = sqlQueryResult->getNbTuples();
    std::vector<std::string> results;
    for (size_t i = 0; i < resultCount; ++i) {
      results.clear();
      results = sqlQueryResult->get(i);

      TMS_Data::Job currentJob;
      std::vector<std::string>::iterator resultIterator = results.begin();
      currentJob.setOwner( *resultIterator++ );  // IMPORTANT: gets the value and increments the iterator
      currentJob.setStatus(vishnu::convertToInt( *resultIterator++ ));
      currentJob.setJobId( *resultIterator++ );
      currentJob.setBatchJobId( *resultIterator++ );
      currentJob.setVmId( *resultIterator++ );
      int batchType = vishnu::convertToInt(*resultIterator);

      switch (currentJob.getStatus()) {
        case vishnu::STATE_COMPLETED:
          throw TMSVishnuException(ERRCODE_ALREADY_TERMINATED, currentJob.getJobId());
          break;
        case vishnu::STATE_CANCELLED:
          throw TMSVishnuException(ERRCODE_ALREADY_CANCELED, currentJob.getJobId());
          break;
        default:
          break;
      }

      if (currentJob.getOwner() != muserSessionInfo.user_aclogin
          && muserSessionInfo.user_privilege != vishnu::PRIVILEGE_ADMIN) {
        throw TMSVishnuException(ERRCODE_PERMISSION_DENIED);
      }

      if (mstandaloneSed != 0) {
        handleNativeBatchExec(CancelBatchAction, "", options, currentJob, batchType, mbatchVersion);
      } else {
        handleSshBatchExec(CancelBatchAction, "", options, currentJob, batchType, mbatchVersion);
      }
    }
  }

  return 0;
}

/**
  * \brief Function to get job information
  * \param jobId The id of the job
  * \return The job data structure
*/
TMS_Data::Job
JobServer::getJobInfo(const std::string& jobId)
{
  TMS_Data::ListJobs jobSteps;

  getJobStepInfo(jobId, jobSteps);

  if (jobSteps.getJobs().size() == 0) {
    throw TMSVishnuException(ERRCODE_UNKNOWN_JOBID);
  }
  return *(jobSteps.getJobs().get(0));
}


/**
 * \brief get Information about a given-job steps
 * \param jobId The id of the job
 * \param jobSteps List of job steps
 * \return The job data structure
 */
void
JobServer::getJobStepInfo(const std::string& jobId, TMS_Data::ListJobs& jobSteps)
{
  std::string sqlQuery = boost::str(boost::format(
                                      "SELECT vsessionid, submitMachineId, submitMachineName, "
                                      "   jobId, jobName, batchJobId, jobPath, workId, relatedSteps, "
                                      "   outputPath, errorPath, outputDir, jobWorkingDir, "
                                      "   jobPrio, nbCpus, job.status, submitDate, endDate, "
                                      "   owner, jobQueue,wallClockLimit, groupName, memLimit,"
                                      "   nbNodes, nbNodesAndCpuPerNode, userid, vmId, vmIp, jobDescription"
                                      " FROM job, vsession, users "
                                      " WHERE vsession.numsessionid=job.vsession_numsessionid "
                                      "   AND vsession.users_numuserid=users.numuserid"
                                      "   AND (job.jobId='%1%' OR job.jobId like '%1%._%%');"
                                      ) % mdatabaseInstance->escapeData(jobId));

  boost::scoped_ptr<DatabaseResult> sqlResult(mdatabaseInstance->getResult(sqlQuery));
  if (sqlResult->getNbTuples() == 0) {
    throw TMSVishnuException(ERRCODE_UNKNOWN_JOBID);
  }

  for (int step = 0; step < sqlResult->getNbTuples(); ++step) {

    TMS_Data::Job_ptr job = new TMS_Data::Job();

    std::vector<std::string> results = sqlResult->get(step);
    std::vector<std::string>::iterator curEntry = results.begin();

    job->setSessionId(*curEntry);
    job->setSubmitMachineId(*(++curEntry));
    job->setSubmitMachineName(*(++curEntry));
    job->setJobId(*(++curEntry));
    job->setJobName(*(++curEntry));
    job->setBatchJobId(*(++curEntry));
    job->setJobPath(*(++curEntry));
    job->setWorkId(vishnu::convertToLong(*(++curEntry)));
    job->setRelatedSteps(*(++curEntry));
    job->setOutputPath(*(++curEntry));
    job->setErrorPath(*(++curEntry));
    job->setOutputDir(*(++curEntry));
    job->setJobWorkingDir(*(++curEntry));
    job->setJobPrio(vishnu::convertToInt(*(++curEntry)));
    job->setNbCpus(vishnu::convertToInt(*(++curEntry)));
    job->setStatus(vishnu::convertToInt(*(++curEntry)));
    job->setSubmitDate(vishnu::string_to_time_t(*(++curEntry)));
    job->setEndDate(vishnu::string_to_time_t(*(++curEntry)));
    job->setOwner(*(++curEntry));
    job->setJobQueue(*(++curEntry));
    job->setWallClockLimit(vishnu::convertToInt(*(++curEntry)));
    job->setGroupName(*(++curEntry));
    job->setMemLimit(vishnu::convertToInt(*(++curEntry)));
    job->setNbNodes(vishnu::convertToInt(*(++curEntry)));
    job->setNbNodesAndCpuPerNode(*(++curEntry));
    job->setUserId(*(++curEntry));
    job->setVmId(*(++curEntry));
    job->setVmIp(*(++curEntry));
    job->setJobDescription(*(++curEntry));

    jobSteps.getJobs().push_back(job);
    results.clear();
  }
}


/**
  * \brief Function to scan VISHNU error message
  * \param errorInfo the error information to scan
  * \param code The code The code of the error
  * \param message The message associeted to the error code
  * \return raises an exception on erroor
*/
void JobServer::scanErrorMessage(const std::string& errorInfo, int& code, std::string& message) {

  code = ERRCODE_INVEXCEP;

  size_t pos = errorInfo.find('#');
  if (pos!=std::string::npos) {
    std::string codeInString = errorInfo.substr(0,pos);
    if (! codeInString.empty()) {
      try {
        code = boost::lexical_cast<int>(codeInString);
      }
      catch(boost::bad_lexical_cast &){
        code = ERRCODE_INVEXCEP;
      }
      message = errorInfo.substr(pos+1);
    }
  }
}

/**
  * \brief Function to convert a given date into correspondant long value
  * \param date The date to convert
  * \return The converted value
*/
long long JobServer::convertToTimeType(std::string date) {

  if (date.empty()  // For mysql empty date is 0000-00-00, not empty. Need this test to avoid problem in ptime
      || date.find("0000-00-00")!=std::string::npos) {
    return 0;
  }

  boost::posix_time::ptime pt(boost::posix_time::time_from_string(date));
  boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
  boost::posix_time::time_duration::sec_type time = (pt - epoch).total_seconds();

  return (long long) time_t(time);
}

/**
  * \brief To get the main configuration
  * \return the pointer to configuration object
*/
ExecConfiguration_Ptr
JobServer::getSedConfig() const {
  return msedConfig;
}

/*
  * \brief Return the directive associated to the batch scheduler
  * \param seperator Hold the seperator used to define parameter
*/
std::string JobServer::getBatchDirective(std::string& seperator) const {

  seperator =  " ";
  std::string directive = "";
  switch(mbatchType) {
    case TORQUE :
      directive = "#PBS";
      break;
    case LOADLEVELER :
      directive = "# @";
      seperator = " = ";
      break;
    case SLURM :
      directive = "#SBATCH";
      break;
    case LSF :
      directive = "#BSUB";
      break;
    case SGE :
      directive = "#$";
      break;
    case PBSPRO :
      directive = "#PBS";
      break;
    case POSIX :
      directive = "#%";
      break;
    case DELTACLOUD : // return default ""
    default :
      break;
  }
  return directive;
}

/**
  * \brief Set specific parameters for job submission
  * \param specificParamss The string containing the list of parameters
  * \param scriptContent The content of the script when required
*/
void JobServer::handleSpecificParams(const std::string& specificParams,
                                     std::string& scriptContent) {

  std::string sep = " ";
  std::string directive = getBatchDirective(sep);
  size_t pos1 = 0;
  size_t pos2 = 0;
  std::string& params = const_cast<std::string&>(specificParams);
  pos2 = params.find("=");
  while (pos2 != std::string::npos) {
    size_t pos3 = 0;
    pos3 = params.find(" ");
    if(pos3 != std::string::npos) {
      std::string lineoption = directive +" "+ params.substr(pos1, pos2-pos1) + sep +  params.substr(pos2+1, pos3-pos2-1) + "\n";
      insertOptionLine(lineoption, scriptContent, directive);
      params.erase(0, pos3);
      boost::algorithm::trim_left(params);
    } else {
      std::string lineoption = directive +" "+ params.substr(pos1, pos2-pos1)+ sep +  params.substr(pos2+1, params.size()-pos2) + "\n";
      insertOptionLine(lineoption, scriptContent, directive);
      break;
    }
    pos2 = params.find("=");
  }
}

/**
  * \brief Function to set the Working Directory
  * \param scriptContent The script content
  * \param options a json object describing options
  * \param jobInfo The job information, could be altered with output path
*/
void
JobServer::setRealFilePaths(std::string& scriptContent,
                            JsonObject* options,
                            TMS_Data::Job& jobInfo)
{
  std::string workingDir = muserSessionInfo.user_achome;
  std::string scriptPath = "";
  std::string inputDir = "";

  if (mbatchType == DELTACLOUD && mbatchType == OPENNEBULA) {
    std::string mountPoint = vishnu::getVar(vishnu::CLOUD_ENV_VARS[vishnu::CLOUD_NFS_MOUNT_POINT], true);
    if (mountPoint.empty()) {
      workingDir = boost::str(boost::format("/tmp/%1%")
                              % vishnu::generatedUniquePatternFromCurTime(jobInfo.getJobId()));
    } else {
      workingDir = boost::str(boost::format("%1%/%2%")
                              % mountPoint
                              % vishnu::generatedUniquePatternFromCurTime(jobInfo.getJobId()));
    }

    inputDir =  boost::str(boost::format("%1%/INPUT") % workingDir);
    scriptPath = boost::str(boost::format("%1%/vishnu-job-script-%2%%3%")
                            % inputDir
                            % jobInfo.getJobId()
                            % bfs::unique_path("%%%%%%").string());

    vishnu::createDir(workingDir, true);
    vishnu::createDir(inputDir, true);

    std::string directory = "";
    try {
      directory = vishnu::moveFileData(options->getStringProperty("fileparams"), inputDir);
    } catch(bfs::filesystem_error &ex) {
      throw SystemException(ERRCODE_RUNTIME_ERROR, ex.what());
    }

    if(directory.length() > 0) {
      std::string fileparams = options->getStringProperty("fileparams");
      vishnu::replaceAllOccurences(fileparams, directory, inputDir);
      options->setProperty("fileparams", fileparams);
    }
  } else {
    std::string path = options->getStringProperty("workingdir");
    if (! path.empty()) {
      workingDir = path;
    } else {
      options->setProperty("workingdir", workingDir);
    }
    scriptPath = boost::str(boost::format("/tmp/vishnuJobScript%1%-%2%")
                            % bfs::unique_path("%%%%%%").string()
                            % jobInfo.getJobId());
  }

  if (scriptContent.find("VISHNU_OUTPUT_DIR") != std::string::npos
      || mbatchType == DELTACLOUD
      || mbatchType == OPENNEBULA) {
    std::string outputDir = boost::str(boost::format("%1%/VISHNU_OUTPUT_DIR_%2%")
                                       % workingDir
                                       % vishnu::generatedUniquePatternFromCurTime(jobInfo.getJobId()));
    vishnu::replaceAllOccurences(scriptContent, "$VISHNU_OUTPUT_DIR", outputDir);
    vishnu::replaceAllOccurences(scriptContent, "${VISHNU_OUTPUT_DIR}", outputDir);
    jobInfo.setOutputDir(outputDir);
  } else {
    jobInfo.setOutputDir("");
  }

  jobInfo.setJobWorkingDir(workingDir);
  options->setProperty("scriptpath", scriptPath);
}


/**
 * \brief Function to process the script with options
 * \param content The script content
 * \param options the options to submit job
 * \param defaultBatchOption The default batch options
 * \return the processed script content
*/
std::string
JobServer::processScript(std::string& content,
                         JsonObject* options,
                         const std::vector<std::string>& defaultBatchOption,
                         const std::string& machineName)
{
  std::string convertedScript;

  vishnu::replaceAllOccurences(content, "$VISHNU_SUBMIT_MACHINE_NAME", machineName);
  vishnu::replaceAllOccurences(content, "${VISHNU_SUBMIT_MACHINE_NAME}", machineName);

  std::string currentOption = options->getStringProperty("textparams");
  if (! currentOption.empty()) {
    vishnu::setParams(content, currentOption);
  }
  currentOption = options->getStringProperty("fileparams");
  if (! currentOption.empty()) {
    vishnu::setParams(content, currentOption) ;
  }
  boost::shared_ptr<ScriptGenConvertor> scriptConvertor(vishnuScriptGenConvertor(mbatchType, content));
  if(scriptConvertor->scriptIsGeneric()) {
    std::string genScript = scriptConvertor->getConvertedScript();
    convertedScript = genScript;
  } else {
    convertedScript = content;
  }
  std::string sep = " ";
  std::string directive = getBatchDirective(sep);
  currentOption = options->getStringProperty("specificparams");
  if (! currentOption.empty()) {
    handleSpecificParams(currentOption, convertedScript);
  }

  if (! defaultBatchOption.empty()){
    processDefaultOptions(defaultBatchOption, convertedScript, directive);
  }
  if(mbatchType == DELTACLOUD) {
    const std::string NODE_FILE = boost::str(boost::format("%1%/NODEFILE") % options->getStringProperty("outputdir"));
    vishnu::replaceAllOccurences(content, "$VISHNU_BATCHJOB_NODEFILE", NODE_FILE);
    vishnu::replaceAllOccurences(content, "${VISHNU_BATCHJOB_NODEFILE}", NODE_FILE);
  }
  return convertedScript;
}


/**
 * @brief Update the result job steps with the base information of the job and save them
 * @param jobSteps The list of steps
 * @param baseJobInfo The base job info
 */
void
JobServer::updateAndSaveJobSteps(TMS_Data::ListJobs& jobSteps, TMS_Data::Job& baseJobInfo)
{
  if (jobSteps.getJobs().size() == 1) {
    TMS_Data::Job_ptr currentJobPtr = jobSteps.getJobs().get(0);
    currentJobPtr->setSubmitMachineId(baseJobInfo.getSubmitMachineId());
    currentJobPtr->setWorkId(baseJobInfo.getWorkId());
    currentJobPtr->setJobPath(baseJobInfo.getJobPath());
    currentJobPtr->setOwner(baseJobInfo.getOwner());
    currentJobPtr->setJobId(baseJobInfo.getJobId());
    currentJobPtr->setOutputDir(baseJobInfo.getOutputDir());
    currentJobPtr->setJobWorkingDir(baseJobInfo.getJobWorkingDir());
    updateJobRecordIntoDatabase(SubmitBatchAction, *currentJobPtr);
  } else {
    int nbSteps = jobSteps.getJobs().size();

    // first set steps' id and other default parameters
    for (int step = 0; step < nbSteps; ++step) {
      TMS_Data::Job_ptr currentJobPtr = jobSteps.getJobs().get(step);
      currentJobPtr->setJobId(boost::str(boost::format("%1%.%2%") % baseJobInfo.getJobId() % step));
      currentJobPtr->setSubmitMachineId(baseJobInfo.getSubmitMachineId());
      currentJobPtr->setWorkId(baseJobInfo.getWorkId());
      currentJobPtr->setJobPath(baseJobInfo.getJobPath());
      currentJobPtr->setOwner(baseJobInfo.getOwner());
      currentJobPtr->setOutputDir(baseJobInfo.getOutputDir());

      // create an entry to the database for the step
      mdatabaseInstance->process(boost::str(boost::format("INSERT INTO job (jobid, vsession_numsessionid)"
                                                          " VALUES ('%1%', %2%)"
                                                          ) % currentJobPtr->getJobId() % muserSessionInfo.num_session));
    }

    // now each job's related steps and record to database
    for (int step = 0; step < nbSteps; ++step) {
      std::string relatedStepList =  "";
      for (int relatedStep = 0; relatedStep < nbSteps; ++relatedStep) {
        if (relatedStep == step) continue;
        if (! relatedStepList.empty()) relatedStepList+=",";
        relatedStepList+=jobSteps.getJobs().get(relatedStep)->getJobId();
      }
      TMS_Data::Job_ptr currentJobPtr = jobSteps.getJobs().get(step);
      currentJobPtr->setRelatedSteps(relatedStepList);
      updateJobRecordIntoDatabase(SubmitBatchAction, *currentJobPtr);
    }
  }
}

/**
 * \brief Function to save the encapsulated job into the database
 * @param action The type of action to finalize (submit, cancel...)
 * @param job The concerned job
 */
void
JobServer::updateJobRecordIntoDatabase(int action, TMS_Data::Job& job)
{
  if (action == CancelBatchAction) {
    std::string query = boost::str(boost::format("UPDATE job set status=%1% where jobid='%2%';")
                                   % vishnu::convertToString(job.getStatus())
                                   % job.getJobId());
    mdatabaseInstance->process(query);
    LOG(boost::str(boost::format("[INFO] job cancelled: %1%")
                   % job.getJobId()), LogInfo);

  } else if (action == SubmitBatchAction) {
    // Append the machine name to the error and output path if necessary
    size_t pos = job.getOutputPath().find(":");
    std::string prefixOutputPath = (pos == std::string::npos)? muserSessionInfo.machine_name+":" : "";
    job.setOutputPath(prefixOutputPath+job.getOutputPath());
    pos = job.getErrorPath().find(":");
    std::string prefixErrorPath = (pos == std::string::npos)? muserSessionInfo.machine_name+":" : "";
    job.setErrorPath(prefixErrorPath+job.getErrorPath());

    // Update the database with the result
    std::string query = "UPDATE job set ";
    query+="vsession_numsessionid="+vishnu::convertToString(muserSessionInfo.num_session)+", ";
    query+="job_owner_id="+vishnu::convertToString(muserSessionInfo.num_user)+", ";
    query+="owner='"+mdatabaseInstance->escapeData(muserSessionInfo.user_aclogin)+"', ";
    query+="submitMachineId='"+mdatabaseInstance->escapeData(mmachineId)+"', ";
    query+="submitMachineName='"+mdatabaseInstance->escapeData(muserSessionInfo.machine_name)+"', ";
    query+="batchJobId='"+mdatabaseInstance->escapeData(job.getBatchJobId())+"', ";
    query+="batchType="+vishnu::convertToString(mbatchType)+", ";
    query+="jobName='"+mdatabaseInstance->escapeData(job.getJobName())+"', ";
    query+="jobPath='"+mdatabaseInstance->escapeData(job.getJobPath())+"', ";
    query+="outputPath='"+mdatabaseInstance->escapeData(job.getOutputPath())+"',";
    query+="errorPath='"+mdatabaseInstance->escapeData(job.getErrorPath())+"',";
    query+="scriptContent='job', ";
    query+="jobPrio="+vishnu::convertToString(job.getJobPrio())+", ";
    query+="nbCpus="+vishnu::convertToString(job.getNbCpus())+", ";
    query+="jobWorkingDir='"+mdatabaseInstance->escapeData(job.getJobWorkingDir())+"', ";
    query+="status="+vishnu::convertToString(job.getStatus())+", ";
    query+="submitDate=CURRENT_TIMESTAMP, ";
    query+="jobQueue='"+mdatabaseInstance->escapeData(job.getJobQueue())+"', ";
    query+="wallClockLimit="+vishnu::convertToString(job.getWallClockLimit())+", ";
    query+="groupName='"+mdatabaseInstance->escapeData(job.getGroupName())+"',";
    query+="jobDescription='"+mdatabaseInstance->escapeData(job.getJobDescription())+"', ";
    query+="memLimit="+vishnu::convertToString(job.getMemLimit())+", ";
    query+="nbNodes="+vishnu::convertToString(job.getNbNodes())+", ";
    query+="nbNodesAndCpuPerNode='"+mdatabaseInstance->escapeData(job.getNbNodesAndCpuPerNode())+"', ";
    query+="outputDir='"+mdatabaseInstance->escapeData(job.getOutputDir())+"', ";
    query+= job.getWorkId()? "workId="+vishnu::convertToString(job.getWorkId())+", " : "";
    query+="vmId='"+mdatabaseInstance->escapeData(job.getVmId())+"', ";
    query+="vmIp='"+mdatabaseInstance->escapeData(job.getVmIp())+"', ";
    query+="relatedSteps='"+mdatabaseInstance->escapeData(job.getRelatedSteps())+"'";
    query+=" WHERE jobid='"+mdatabaseInstance->escapeData(job.getJobId())+"';";

    mdatabaseInstance->process(query);

    // logging
    if (job.getSubmitError().empty()) {
      LOG(boost::str(boost::format("[INFO] job submitted: %1%. User: %2%. Owner: %3%")
                     % job.getJobId()
                     % muserSessionInfo.userid
                     % muserSessionInfo.user_aclogin), LogInfo);
    } else {
      LOG((boost::str(boost::format("[WARN] submission error: %1% [%2%]")
                      % job.getJobId()
                      % job.getSubmitError())), LogWarning);
    }
  } else {
    throw TMSVishnuException(ERRCODE_INVALID_PARAM, "unknown batch action");
  }
}


/**
 * @brief Get the uid corresponding to given system user name
 * @param username
 * @return
 */
uid_t
JobServer::getSystemUid(const std::string& name)
{
  passwd* info = getpwnam(name.c_str());
  if (! info) {
    throw TMSVishnuException(ERRCODE_INVALID_PARAM,
                             "The user doesn't have a valid uid "+name);
  }
  return info->pw_uid;
}


void
JobServer::checkMachineId(std::string machineId) {
  std::string sqlMachineRequest = boost::str(
                                    boost::format("SELECT machineid"
                                                  " FROM machine"
                                                  " WHERE machineid='%1%'"
                                                  " AND status<>%2%")%mdatabaseInstance->escapeData(machineId) %vishnu::STATUS_DELETED);
  boost::scoped_ptr<DatabaseResult> machine(mdatabaseInstance->getResult(sqlMachineRequest.c_str()));
  if(machine->getNbTuples()==0) {
    throw UMSVishnuException(ERRCODE_UNKNOWN_MACHINE);
  }
}


/**
   * @brief create the executable for submit job script
   * @param content The script content
   * @param options The submit options
   * @param defaultBatchOption The default batch options
   * @return The script path
 */
std::string
JobServer::createJobScriptExecutaleFile(std::string& content,
                                        JsonObject* options,
                                        const std::vector<std::string>& defaultBatchOption)
{
  std::string path = options->getStringProperty("scriptpath");

  // Create the file on the file system
  vishnu::saveInFile(path,
                     processScript(content,
                                   options,
                                   defaultBatchOption,
                                   muserSessionInfo.machine_name));

  // Make the file executable
  if(0 != chmod(path.c_str(),
                S_IWUSR|S_IRUSR|S_IXUSR|
                S_IRGRP|S_IXGRP|
                S_IROTH|S_IXOTH)) {
    throw SystemException(ERRCODE_INVDATA, "Unable to make the script executable" + path) ;
  }

  return path;
}

/**
 * @brief Export environment variables used throughout the execution, notably in cloud mode
 * @param defaultJobInfo The default job info
 */
void
JobServer::exportJobEnvironments(const TMS_Data::Job& defaultJobInfo)
{
  setenv("VISHNU_JOB_ID", defaultJobInfo.getJobId().c_str(), 1);
  setenv("VISHNU_OUTPUT_DIR", defaultJobInfo.getOutputDir().c_str(), 1);
}
