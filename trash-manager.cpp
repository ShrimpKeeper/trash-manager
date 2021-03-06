/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>//outputting via CL
#include <fstream>//interacting with log files
#include <time.h>//logging time placed and checking current time
#include <boost/filesystem.hpp>//interacting with directories 

bool log(std::string, std::fstream&, bool, bool);

bool isLogged(std::string, std::fstream&);

int  manage_trash(std::string, bool, bool);

int main(int argc, const char *argv[])
{
	bool verbose = false, quiet = false, help = false;
	if (argc > 1) {//check for flags 
		verbose = (strncmp(argv[1], "-v", 2) == 0);//check for verbose flag
		quiet = (strncmp(argv[1], "-q", 2) == 0);//check for quiet flag
		help = ((strncmp(argv[1], "-h", 2) == 0) || (strncmp(argv[1], "--help", 6) == 0));//check for help flag
	}

	if (help) {//check the help flag
		std::cout<<"USAGE: trash-manager [-v|-q] <trash-directory>\n";
		return -1;
	}

	if (argc - verbose - quiet) {
		boost::filesystem::path homes("/Users/");
		for (boost::filesystem::directory_entry& home : boost::filesystem::directory_iterator(homes)) {//for each in homes
			manage_trash((home.path().string() + "/.Trash/"), verbose, quiet);
		}
	}
	else {
		manage_trash((std::string)argv[argc - 1], verbose, quiet);
	}

	return 0;
}

//add an entry to the log
bool log(std::string file, std::fstream& logfile, bool verbose, bool quiet)
{
	bool successful;

	if (logfile<<file<<'\n'<<time(NULL)<<'\n')//if it successfully wrote to the file
	successful = true;
	else 
	successful = false;

	if (!successful && !quiet) 
	std::cerr<<"failed logging entry \""<<file<<"\"\n";
	else if (verbose) 
	std::cout<<"successfully logged entry \""<<file<<"\"\n";
		
	return successful;
}

//check if an entry is logged
bool isLogged(std::string entry, std::fstream& logfile) {
	logfile.seekg(std::ios_base::beg);//start search from beggining of the file
	std::string testString;
	for (int i = 0; logfile.peek() != EOF; i++) {
		getline(logfile, testString);
		if (entry == testString) {//if it's what we're looking for
			logfile.clear();
			return true;
		}
		logfile.ignore(2048, '\n');//drop a line for the time logged 
	}
	logfile.clear();
	return false;
}

int  manage_trash(std::string trash_location, bool verbose, bool quiet)
{
	///////////////////////////////////////////////
	//              Opening logfile              //
	///////////////////////////////////////////////
	boost::filesystem::path trash(trash_location);//open trashcan using last argument

	if (exists(trash)) {//check if trashcan exists
		if (!is_directory(trash)) {//and is also a directory
			if (!quiet)
			std::cerr<<"fatal error: specified "<<trash_location<<" exists but isn't a directory\n";
			return -1;
		}
	}
	else {
		if (!quiet)
		std::cerr<<"fatal error: "<<trash_location<<" does not exist\n";
		return -1;
	}
	#define LOGFILE (trash.string()+".trash-log")
	bool newLogfile = false;
	std::fstream logfile;
	logfile.open(LOGFILE, std::ios_base::in | std::ios_base::out | std::ios_base::app);

	if (!logfile.is_open()) {//if there is no logfile then create one
		if (verbose)
		std::cout<<"logfile does not exist, creating a new logfile\n";
		logfile.clear();//clear iostate flags from original opening attempt 
		logfile.open(LOGFILE, std::ios_base::out);//create a new file (no need for input)
		newLogfile = true;
		if (!logfile.is_open()) {//check if it created a logfile successfully 
			if (!quiet)
			std::cerr<<"fatal error: could not open logfile at "<<LOGFILE<<'\n';
			return -1;
		}
		else if (verbose)
		std::cout<<"successfully created a logfile at "<<LOGFILE<<'\n';;
	}
	else if (verbose)
	std::cout<<"successfully opened the existing logfile at "<<LOGFILE<<'\n';;
	

	///////////////////////////////////////////////////
	//            Logging/deleting files             //
	///////////////////////////////////////////////////
	int age = -1;
	int oldPosition;
	for (boost::filesystem::directory_entry& entry : boost::filesystem::directory_iterator(trash)) {//for each file in the trashcan
		oldPosition = logfile.tellg();
		if (!newLogfile && isLogged(entry.path().string(), logfile)) {//if it's an existing log file and the file is already logged 
			logfile>>age;
			if (verbose) {
				std::cout<<"entry \""<<entry.path().string()<<"\" is already logged\n";
				std::cout<<"\tdate logged: "<<age<<std::endl;
			}
			if (time(NULL) - age > 1210000) {//if it was logged more than 2 weeks ago
				if (verbose)
				std::cout<<'\t'<<entry.path().string()<<" is was logged more than 2 weeks ago. This file will be deleted\n";
				remove(entry);//delete file
			}
		}
		else if (entry.path().filename().string() != ".trash-log" && entry.path().filename().string() != ".DS_Store" ) {
			logfile.seekg(oldPosition);//return to the position from before the search
			log (entry.path().string(), logfile, verbose, quiet);
		}
		if (verbose)
		std::cout<<std::endl;
	}

	////////////////////////////////////////
	//     Remove unneccesary entries     //
	////////////////////////////////////////
	std::string fileEntry;
	bool iterOnAge;
	std::fstream cleanLogfile;
	cleanLogfile.open(trash.string() + ".new_trash-log", std::ios_base::out );

	logfile.seekg(std::ios_base::beg);
	while(logfile.peek() != EOF) {
		getline(logfile, fileEntry);
		iterOnAge = true;
		for (boost::filesystem::directory_entry& dirEntry : boost::filesystem::directory_iterator(trash)) {//for each file in the trashcan
			if (fileEntry == dirEntry) {//if it's a match and needed then add it and it's age to the new file
				cleanLogfile<<fileEntry<<std::endl;
				getline(logfile, fileEntry);//get age
				cleanLogfile<<fileEntry<<std::endl;
				iterOnAge = false;
				break;
			}
		}
		if (iterOnAge) {//skip age if needed
			logfile.ignore(2048, '\n');
		}
	}
	
	logfile.close();
	cleanLogfile.close();
	boost::filesystem::path dirtyLogfilePath(LOGFILE);//create path for the dirty logfile
	boost::filesystem::path cleanLogfilePath(trash.string() + ".new_trash-log");//create path for the clean logfile
	remove(dirtyLogfilePath);//delete old logfile
	rename(cleanLogfilePath, LOGFILE);//replace it with cleanLogfile
	
	return 0;
}
