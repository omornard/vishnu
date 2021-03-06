#include "daemon_cleaner.hpp"

#include <boost/foreach.hpp>
#include <fstream>

#include <sys/types.h>                  // for pid_t
#include <syslog.h>                     // for syslog, LOG_ERR
#include <unistd.h>                     // for fork, getpid
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/filesystem/fstream.hpp>  // for ofstream
#include <boost/filesystem/operations.hpp>  // for directory_iterator, etc
#include <boost/filesystem/path.hpp>    // for path
#include <boost/interprocess/sync/file_lock.hpp>  // for file_lock
#include <boost/interprocess/sync/lock_options.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>  // for scoped_lock
#include <boost/iterator/iterator_facade.hpp>  // for iterator_facade, etc
#include <boost/mpl/bool.hpp>           // for bool_
#include <boost/mpl/bool_fwd.hpp>       // for false_
#include <csignal>                      // for kill
#include <cstdlib>                      // for exit, EXIT_SUCCESS, getenv, etc
#include <iostream>                     // for basic_ostream, etc
#include <list>                         // for list
#include "VishnuException.hpp"          // for VishnuException


#include "sessionUtils.hpp"
#include "utilVishnu.hpp"
#include "api_ums.hpp"

//{{RELAX<MISRA_0_1_3> Because these variables are used in this file or other files

/**
 * \brief The home directory
 */
bfs::path home_dir;

/**
 * \brief The session storage  directory
 */
bfs::path session_dir;

/**
 * \brief The VISHNU output files directories
 */

bfs::path vishnu_dir;

/**
 * \brief The daemon locked file
 */

bfs::path daemon_file;

//}}RELAX<MISRA_0_1_3>

using namespace std;
using namespace vishnu;

/**
 * \brief Check if the pid exists
 * \param pid The given pid
 * \return True if the pid exists
 */
bool
pid_exists(int pid) {
  return !kill(pid, 0);
}


/**
 * \brief Deletes the file corresponding to the terminal closed
 * \param configFile: The VISHNU configuration file
 * \param ac: The number of command parameters
 * \param av: The names of parameters
 */
void
deleter(char* configFile,int ac,char* av[]) {
  extern bfs::path daemon_file;
  extern bfs::path session_dir;

  bi::file_lock f_lock(daemon_file.string().c_str());

  {
    bi::scoped_lock<bi::file_lock> lock(f_lock,bi::try_to_lock_type());

    if (false == lock.owns()) {
      exit(EXIT_SUCCESS);
    }

    while(true) {
      bfs::directory_iterator it = bfs::directory_iterator(session_dir);

      for (; it != bfs::directory_iterator(); ++it) {// loop over session directory
        const bfs::path current_path = it->path();

        if (!pid_exists(convertToInt(it->path().native()))) {
          // close all sessions opened by disconnect mode before deleting file
          SessionContainer allSessions=getAllSessions(current_path.string());// get all sessions stored in file

          if (false == allSessions.empty()) { // is there a session?
            BOOST_FOREACH (SessionEntry session, allSessions) {
              if (session.getClosePolicy() == 2) {//that session is open by disconnect mode
                // TODO : a loop to handle that
                if (vishnuInitialize(configFile, ac, av)) {
                  if (vishnuInitialize(configFile, ac, av)) {
                    if (vishnuInitialize(configFile, ac, av)) {
                      syslog(LOG_ERR,"VISHNU initialization failed !");
                      exit (EXIT_FAILURE);
                    }
                  }
                }

                try {
                  close (session.getSessionKey()); // and need to be closed
                } catch(VishnuException & e) {  // if the close command fails
                  if (!checkBadSessionKeyError(e)) {  // check if we need to stop the daemon
                    syslog(LOG_ERR, "The file is corrupted");
                    exit(e.getMsgI());
                  }
                }

              }  // end of if (session.getClosePolicy() == 2)
            }  // end of looping over the file
          }  // end of if (false == allSessions.empty())

          bfs::remove(current_path);// remove the file
        }  // end of if (!pid_exists(convertToInt(it->path().native())))
      }  // end of for

      // take a pause
      btt::sleep(bpt::seconds(5));//%RELAX<MISRA_0_1_3> Because it used to take a pause
    }

    exit(EXIT_SUCCESS);
  }
}


/**
 *\brief Creates a child process and call the deleter() function
 * \param configFile: The VISHNU configuration file
 * \param ac: The number of command parameters
 * \param av: The names of parameters
 */



void
cleaner(char* configFile,int ac,char* av[]) {
  // declare all global variables
  extern bfs::path home_dir;
  extern bfs::path session_dir;
  extern bfs::path vishnu_dir;
  extern bfs::path daemon_file;

  home_dir = getenv("HOME"); // set the user home directory
  vishnu_dir = home_dir ;  // set the directory in which will be stored the daemon lock file

  vishnu_dir /= ".vishnu";
  daemon_file=vishnu_dir;
  session_dir /= vishnu_dir;   // set the directory in which will be stored all session files

  daemon_file/="daemon.pid";
  session_dir /= "sessions";

  if (false == bfs::exists(session_dir)) { // make sure session_dir exists
    bfs::create_directories(session_dir);
  }

  pid_t pid = fork();// create a process resident to delete and close  all sessions corresponding on close terminal

  if (pid < 0) {
    std::cerr << "cleaning process: fork() failed" << std::endl;
  } else if (0 == pid) {
    if (false == bfs::exists(daemon_file)) { // make sure daemon_file exists
      bfs::ofstream f(daemon_file);
      f << getpid();    // record daemon pid
      f.close();
    }

    deleter(configFile,ac,av);
  }

  return;
}
