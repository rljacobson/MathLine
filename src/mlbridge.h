#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-avoid-c-arrays"
//
//  mlbridge.h
//  MathLinkBridge
//
//  Created by Robert Jacobson on 12/14/14.
//  Copyright (c) 2014 Robert Jacobson. All rights reserved.
//

#pragma once

#include <iostream>
#include <string>
#include <queue>
#include <exception>

#include "config.h"

struct MLBridgeException: public std::exception{
    MLBridgeException(std::string errorMsg, int errorCode = 0);
    int errorCode;
    std::string errorMsg;
    std::string ToString();
};

struct MLBridgeMessage{
    std::string name;
    std::string tag;
    std::string message;
    int position;
};

class MLBridge {
public:
    //Parameters affecting how to communicate with the user and kernel.
    std::string prompt{""}; //Supplied by the user of this MLBridge object.
    std::string kernelPrompt{""}; //Supplied by the kernel.
    bool useMainLoop = true;
    bool showInOutStrings = true;
    bool useGetline = false;
    
    int argc = 4;
    const char *argvdefaults[4] = {"MathLine",
        "-linklaunch",
        "-linkname",
        "math -" MMANAME_LOWER};
    const char **argv = nullptr;
    //Streams to use for io.
    std::ostream *pcout = &std::cout;
    std::istream *pcin = &std::cin;
    /*
     The kernel may return postscript code. This vector of postscript strings contains the postscript images returned by the kernel. It is up to the user of MLBridge to remove items from this vector.
     
     To use this feature you must evaluate '$Display = "stdout"'.
     
     */
    std::queue<std::string> images;
    
    
    MLBridge();
    MLBridge(int argc, const char *argv[]);
    ~MLBridge();

    void Connect(int argc, const char *argv[]);
    void Connect();
    bool IsConnected(){ return connected; }
    void Disconnect();

    bool IsRunning();
    void REPL();
    void SetMaxHistory(int max = 10);
    void SetPrePrint(const std::string &preprintfunction);
    std::string GetKernelVersion();
    std::string GetEvaluated(const std::string &expression);
    
private:
    //Variables to keep track of state.
    bool connected = false;
    bool running = false;
    //The following is set to true when an "incomplete expression" syntax error occurs, as it indicates that more input is needed.
    bool continueInput = false;
    enum {ExpressionMode, TextMode} inputMode = ExpressionMode;
    //Holds the image data (postscript code) as we receive it from the kernel until we have it all.
    std::string *image = nullptr;
    bool makeNewImage = true;
    //The last input string we sent to the kernel.
    std::string inputString;
    std::string outputPrompt;
    //Syntax messages are cached. 
    std::queue<MLBridgeMessage*> messages;
    
    MMALINK link = nullptr;
    MMAEnvironment environment = nullptr;
    
    void ErrorCheck();
    std::string ReadInput();
    void PrintMessages();
    void InitializeKernel();

    // Evaluation with REPL.
    void Evaluate(const std::string &input);
    //Skips the Main Loop regardless of the state of useMainLoop.
    void EvaluateWithoutMainLoop(const std::string &input, bool eatReturnPacket = true);

    //Convenience wrapper for MLGetUTF8String, etc..
    enum GetFunctionType {GetString, GetFunction, GetSymbol, GetCharacters};
    std::string GetUTF8String(GetFunctionType func = GetString);
    int GetNextPacket();
    
    //These are the packets this code knows how to handle. Each returns whether no more packets are expected from the kernel.
    bool ReceivedInputNamePacket();
    bool ReceivedInputPacket();
    bool ReceivedOutputNamePacket();
    bool ReceivedReturnTextPacket();
    //bool ReceivedReturnPacket(); //We just use the code for ReturnExpressionPacket for this case.
    bool ReceivedReturnExpressionPacket();
    bool ReceivedTextPacket();
    bool ReceivedDisplayPacket();
    bool ReceivedDisplayEndPacket();
    bool ReceivedSyntaxPacket();
    bool ReceivedInputStringPacket();
    bool ReceivedMenuPacket();
    bool ReceivedMessagePacket();
    bool ReceivedSuspendPacket();
    bool ReceivedResumePacket();
    bool ReceivedBeginDialogPacket();
    bool ReceivedEndDialogPacket();

    void ProcessKernelResponse();
};

#pragma clang diagnostic pop
