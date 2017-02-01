//
//  MathLine
//
//  Created by Robert Jacobson on 12/14/14.
//  Copyright (c) 2014 Robert Jacobson. All rights reserved.
//

#include <iostream>
#include <boost/program_options.hpp>
#include "mlbridge.h"

//Shortened for convenience.
namespace opt = boost::program_options;

#define CONTINUE 0
#define QUIT_WITH_SUCCESS 1
#define QUIT_WITH_ERROR 2

//Why is this not part of std::string?!
char *copyDataFromString(const std::string str){
    char * newstring = new char[str.size() + 1];
    std::copy(str.begin(), str.end(), newstring);
    newstring[str.size()] = '\0';
    return newstring;
}

int ParseProgramOptions(MLBridge &bridge, int argc, const char * argv[]){
    //Parse options using boost::program_options. Much of this is boilerplate.
    opt::options_description description("All options");
    description.add_options()
    ("mainloop",
     opt::value<bool>(&bridge.useMainLoop)->default_value(true),
     "Boolean. Whether or not to use the kernel's Main Loop which keeps track of session history with In[#] and Out[#] variables. Defaults to true.")
    ("prompt",
     opt::value<std::string>(),
     "String. The prompt presented to the user for input. When inoutstrings is true, this is typically the empty string.")
    ("inoutstrings",
     opt::value<bool>(&bridge.showInOutStrings)->default_value(true),
     "Boolean. Whether or not to print the \"In[#]:=\" and \"Out[#]=\" strings. When mainloop is false this option does nothing. Defaults to true.")
    /* This is silly. The user can just set $PrePrint herself.
     ("preprint",
     opt::value<std::string>(),
     "String. This option sets $PrePrint to the given value which affects how the output is displayed. Defaults to InputForm.")
     */
    ("linkname",
     opt::value<std::string>(),
     "String. The call string to set up the link. The default works on *nix systems on which math is in the path and runnable. Defaults to \"math -mathlink\".")
    ("linkmode",
     opt::value<std::string>(),
     "String. The MathLink link mode. The default launches a new kernel which is almost certainly what you want. It should be possible, however, to take over an already existing kernel, though this has not been tested. Defaults to \"launch\".")
    ("usegetline",
     opt::value<bool>(&bridge.useGetline)->default_value(false),
     "Boolean. If set to false, we use readline-like input with command history and emacs-style editing capability. If set to true, we use a simplified getline input with limited editing capability. Defaults to false.")
    ("maxhistory",
     opt::value<int>()->default_value(10),
     "Integer (nonnegative). The maximum number of lines to keep in the input history. Defaults to 10.")
    ("help", "Produce help message.")
    ;
    opt::variables_map optionsMap;
    try{
        opt::store(opt::parse_command_line(argc, argv, description), optionsMap);
        
        if ( optionsMap.count("help")){
            std::cout << "MathLine Usage.\n" << description << std::endl;
            return QUIT_WITH_SUCCESS;
        }
        //Since notify can throw an exception, we call it AFTER possibly displaying the help message.
        opt::notify(optionsMap);
    }
    catch(opt::error &e){
        std::cerr << "ERROR: " << e.what() << "\n\n";
        std::cerr << description << std::endl;
        return QUIT_WITH_ERROR;
    }
    
    //Now apply the options to our MLBridge instance.
    //(We are able to apply some automatically above.)
    if(optionsMap.count("prompt")){
        bridge.prompt = optionsMap["prompt"].as<std::string>();
    }
    if(optionsMap.count("linkname")){
        std::string str = optionsMap["linkname"].as<std::string>();
        if(str.empty()){
            //Empty string. Ignore this option.
            std::cout << "Option linkname cannot be empty. Ignoring." << std::endl;
        } else{
            //Make a copy, because optionsMap will go out of scope and free the linkname before we can connect with it.
            bridge.argv[1] = copyDataFromString(str);
        }
    }
    if(optionsMap.count("linkmode")){
        std::string str = optionsMap["linkmode"].as<std::string>();
        if(str.empty()){
            //Empty string. Ignore this option.
            std::cout << "Option linkmode cannot be empty. Ignoring." << std::endl;
        } else{
            //Make a copy, because optionsMap will go out of scope and free the linkname before we can connect with it.
            bridge.argv[3] = copyDataFromString(str);
        }
    }
    if(optionsMap.count("maxhistory")){
        int max = optionsMap["maxhistory"].as<int>();
        if (max >= 0) {
            bridge.SetMaxHistory(max);
        } else{
            std::cout << "Option maxhistory must be nonnegative." << std::endl;
        }
    }

    return CONTINUE;
}

int main(int argc, const char * argv[]) {
    //Banner
    std::cout << "MathLine: A free and open source textual interface to Mathematica." << std::endl;
    
    //Passing false tells the MLBridge that we are going to set up the link options and connect manually. Otherwise it would choose reasonable defaults for us.
    MLBridge bridge(false);
    
    //Parse the command line arguments.
    int parseFailed = ParseProgramOptions(bridge, argc, argv);
    if (parseFailed) {
        return parseFailed;
    }
    
    //Attempt to establish the MathLink connection using the options we've set.
    try{
        bridge.Connect();
    } catch(MLBridgeException e){
        std::cerr << e.ToString() << "\n";
        std::cerr << "Could not connect to Mathematica. Check that \"" << bridge.argv[1] << "\" works from a command line." << std::endl;
        return 1;
    }
    if(bridge.isConnected()){
        //Let's print the kernel version.
        std::cout << "Mathematica " << bridge.GetKernelVersion() << "\n" << std::endl;
        bridge.REPL();
    } else{
        std::cout << "MLBridge failed to connect.";
    }
        
    return 0;
}
