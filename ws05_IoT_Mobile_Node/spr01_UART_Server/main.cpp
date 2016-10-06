
//for usleep
#include <unistd.h>
//for printf
#include <stdio.h>
//for stdout
#include <iostream>
//for file
#include <fstream>
//for abs
#include <cmath>

//for exit()
#include <cstdlib>

#include "serial.hpp"
#include "utils.hpp"

using namespace std;

void help_arguments()
{
	string line;
	ifstream helpFile("help.txt");
	if (helpFile.is_open())
	{
		while ( getline (helpFile,line) )
		{
		  cout << line << '\n';
		}
		helpFile.close();
	}
	else
	{
		std::cout << "help file missing reinstall SW" << std::endl;
	}
	
}


int main( int argc, char** argv )
{
	strmap conf;
	utl::args2map(argc,argv,conf);
	Serial 		ser;

	if(utl::exists(conf,"logfile"))
	{
		std::cout << "parsed logfile = " << conf["logfile"] << std::endl;
		ser.start_logfile(conf["logfile"]);
	}
	if(utl::exists(conf,"port"))
	{
		std::cout << "parsed port = " << conf["port"] << std::endl;
		ser.start(conf["port"]);
	}
	else
	{
		help_arguments();
		exit(1);
	}
	
	

	
	//std::cout << MAGENTA << "Colored " << CYAN << "Text" << RESET << std::endl;
	
	while (1) 
	{
		//display Received log
		if(ser.update())
		{
			ser.logBuffer();
		}
		usleep(500000);
		
	}

	return 0;
}
