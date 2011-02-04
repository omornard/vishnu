

#include "addVishnuUser.hh"

namespace po = boost::program_options;

using namespace std;

int main (int ac, char* av[]){
     
       

	try {


		Configuration config(av[0]);// take the command line name

		string defaultConfig = "VishnuConfig.cfg";

		/******* Parsed value containers ****************/
       
		string dietConfig;

		std::string userId;
        
		std::string firstname;

		std::string lastname;

		int privilege; 
		
		std::string email; 

		/********** EMF data ************/

		UMS_Data::User newUser;




/***************  Default configuration file ***************** */

		{
    
			ifstream f(defaultConfig.c_str());
   
			if (f.is_open()){
				config.setConfigFile(defaultConfig);
			}
   
			f.close();
		}
/**************** Describe options *************/



		Options opt(&config );
  
		opt.add("version,v",
				"print version message",
				GENERIC );
		
        opt.add<string>("dietConfig,c", 
						            "The diet config file",
												ENV,
												dietConfig);
				
				opt.add<int>("privilege",
		                 "the privilege of the user (admin or simple user)",
										 HIDDEN,
										 privilege);

				opt.setPosition("privilege",1);


				opt.add<string>("userId,u",
									    	"represents the VISHNU user identifier",
												HIDDEN,
												userId);

				opt.setPosition("userId",1);

				opt.add<string>("firstname",
												"The firstname of the user",
												HIDDEN,
												firstname);

				opt.setPosition("firstname",1);


				opt.add<string>("lastname",
												"The lastname of the user",
												HIDDEN,
												lastname);

				opt.setPosition("lastname",1);


				opt.add<string>("email",
												"The email of the user",
												HIDDEN,
												email);
										
				opt.setPosition("email",1);

/**************  Parse to retrieve option values  ********************/
 
		opt.parse_cli(ac,av);

		opt.parse_cfile();
		
		opt.parse_env(env_name_mapper());

		opt.notify();





/********  Process **************************/

		if ((ac < 6) || (opt.count("help"))){

			cout << "Usage: " << av[0] <<" privilege userId firstname lastname email"<<endl;
			
			cout << opt << endl;
                 
			return 0;

		}

		
		if (opt.count("userId")){
			
			cout <<"The user identifier is " << userId << endl;
			
			newUser.setUserId(userId);
		}
		
		if(opt.count("privilege")){
			
			cout <<"the privilege is : " << privilege << endl;
			
			newUser.setPrivilege(privilege);
		}
		
		if(opt.count("firstname")){

			cout << "The firstname is " << firstname << endl;
			
			newUser.setFirstname(firstname);
                        
		}


		if (opt.count("lastname")){
			
			cout <<"The lastname is " << lastname << endl;
			
			newUser.setLastname(lastname);

		}
  
		if (opt.count("email")){
	
			cout <<"The email is " << email << endl;
			
		newUser.setEmail(email);
		}

            
		if (opt.count("dietConfig")){
           
			cout <<"The diet config file " << dietConfig << endl;
                        
                }

/************** Call UMS connect service *******************************/

               // initializing DIET
							 
							  if (diet_initialize(dietConfig.c_str(), ac, av)) {
                    cerr << "DIET initialization failed !" << endl;
               return 1;
              }

    
							std::string sessionKey;
		
							int res = addVishnuUser(sessionKey,newUser);


		

	}// End of try bloc

	catch(std::exception& e){
		cout << e.what() <<endl;
		return 1;
	}
	
	return 0;

}// end of main


