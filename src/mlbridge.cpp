//
//  mlbridge.cpp
//  MathLinkBridge
//
//  Created by Robert Jacobson on 12/14/14.
//  Copyright (c) 2014 Robert Jacobson. All rights reserved.
//
//  Note: Functions with an MMA- prefix are really MathLink/WSTP functions.
//        The correct prefix (ML- or WS-) is determined at compile time with
//        a macro. See config.h for details.

#include <iostream>
#include <string>
#include <signal.h>
#include <stdlib.h> //Needed to free() memory linenoise allocat with malloc().
#include "linenoise.h"
#include "mlbridge.h"

//Used for printf() debugging.
bool debug = false;
void DebugPrint(std::string msg){
    //std::ostream &cout = *pcout;
    if(debug) std::cout << msg << "\n";
}


MLBridgeException::MLBridgeException(std::string error, int errorCode): errorMsg(error), errorCode(errorCode){
    //Pass.
}

std::string MLBridgeException::ToString(){
    return std::string(MMANAME " Error " + std::to_string(errorCode) + ": " + errorMsg);
}


MLBridge::MLBridge(){
    SetMaxHistory();
    argv = argvdefaults;
}

MLBridge::MLBridge(int newArgc, const char *newArgv[]){
    SetMaxHistory();
    argc = newArgc;
    argv = newArgv;
}

MLBridge::~MLBridge(){
    Disconnect();
}

void MLBridge::Connect(int newArgc, const char *newArgv[]){
    argc = newArgc;
    argv = newArgv;
    Connect();
}

void MLBridge::Connect(){
    int error = MMAEOK;
    connected = false;

    //If no parameters are specified and this has no default parameters, bail.
    if(argc==0 || argv==NULL){
        throw MLBridgeException("No " MMANAME " parameters specified for the connection.");
    }
    
    //Initialize the link to Mathematica.
    environment = MMAInitialize(NULL);
    if(environment == NULL){
        //Failed to initialize the link.
        throw MLBridgeException("Cannot initialize " MMANAME ".");
    }
    
    //Open the link to Mathematica.
    //link = MMAOpen(argc, (char **)argv);
    DebugPrint("Calling WSOpenArgcArgv...");
    link = MMAOpenArgcArgv( environment, argc, (char **)argv, &error);
    DebugPrint("WSOpenArgcArgv returned: " + std::to_string(error));
    if (link == NULL || error != MMAEOK) {
        DebugPrint("Link is null or error.");
        //The link failed to open.
        connected = false;
        MMADeinitialize(environment);
        throw MLBridgeException("Cannot open " MMANAME " link.", error);
    }

    //Activate the link. This blocks until the kernel is ready.
    /*
         TODO: MLActivate can block indefinitely. We should poll MLReady until it returns true or until we timeout before trying to call MLActivate.
     */
    if(!MMAActivate(link)) ErrorCheck();

    /*
    //Make sure we can connect through the link.
    if (MMAReady(link) != true ){
        //We're not connected.
        connected = false;
        MMAClose(link);
        MMADeinitialize(environment);
        throw MLBridgeException("Cannot connect to " MMANAME ".");
    }
    */

    //Link is successful.
    connected = true;

    //Initialize the kernel (sets $PrePrint, etc.).
    InitializeKernel();
}

void MLBridge::Disconnect(){
    if(connected){
        MMAClose(link);
        connected = false;
    }
    if(environment) MMADeinitialize(environment);
}

void MLBridge::InitializeKernel() {
    int packet;

    //The first thing the kernel does is send us an InputNamePacket.
    packet = GetNextPacket();
    if(packet == INPUTNAMEPKT){
        kernelPrompt = GetUTF8String();
    } else{
        //Error condition, but not ILLEGALPKT or other link/kernel error.
        throw MLBridgeException("Kernel sent an unexpected packet (" + std::to_string(packet) + ") during initial startup.");
    }
    SetPrePrint("InputForm");
}

std::string MLBridge::ReadInput(){
    std::string inputString;
    std::string promptToUser = prompt + kernelPrompt;
    if(continueInput){
        promptToUser.replace(0, promptToUser.length()-1, promptToUser.length(), ' ');
    }

    /*
     The user of this class may either use std::getline() or linenoise. The advantage of getline is that it can be used with something other than std::cin, while linenoise ignores pcin and always uses std::cin.
     */
    if(useGetline){
        std::istream &cin = *pcin;
        std::ostream &cout = *pcout;
        
        cout << promptToUser;
        
        std::getline(cin, inputString);
        //Check the status of cin.
        if (!cin.good()){
            //A ctrl+c event inside of getline introduces an internal error in cin. We attempt to clear the error.
            /*
             TODO: Generally cin is std::cin (it's the default), but it need not be. We should have a more robust way of dealing with ctrl+c while blocking in getline().
             */
            cin.clear();
        }
    } else {
        char *line;
        line = linenoise(promptToUser.data());
        linenoiseHistoryAdd(line);
        inputString = std::string(line);
        free(line);
    }
    
    kernelPrompt = "";
    return inputString;
}

void MLBridge::SetMaxHistory(int max){
    //We pass max+1 because apparently 1 means zero history for linenoise.
    if(!linenoiseHistorySetMaxLen(max+1))
        throw MLBridgeException("Invalid maximum history length.");
}

void MLBridge::REPL(){
    std::ostream &cout = *pcout;
    std::string inputString;
    
    //For convenience we wrap everything in a try-block. However, some errors are recoverable. Though we do not do this, one could attempt to clear the error and restart the REPL.
    try {
        while(true){
            inputString = ReadInput();
            if( inputString == "Exit" || inputString == "Exit[]" || inputString == "Quit" ) break;
            Evaluate(inputString);
            //Read and act on response from the kernel.
            ProcessKernelResponse();
        }
    } catch (MLBridgeException e) {
        cout << e.ToString() << std::endl;
    }
}

std::string MLBridge::GetUTF8String(GetFunctionType func){
    //func defaults to GetString.
    //MLGetUTF8String does NOT null terminate the string.
    const unsigned char *stringBuffer = NULL;
    int bytes = 0;
    int characters;
    int success = 0;
    std::string output;

    //Wait until the kernel is ready.
    MMAWaitForLinkActivity(link);
    
    switch(func){
        case GetString:
            success = MMAGetUTF8String(link, &stringBuffer, &bytes, &characters);
            break;
            
        case GetFunction:
            //success = MLGetUTF8Function(link, &stringBuffer, &bytes, &characters);
            //Link error? At least we never use MLGetUTF8Function.
            break;
            
        case GetSymbol:
            success = MMAGetUTF8Symbol(link, &stringBuffer, &bytes, &characters);
            break;
            
        case GetCharacters:
            //We don't ever use MLGetUTF8Characters.
            break;
    }
    
    if(!success )
    {
        //No string to read.
        ErrorCheck(); //Disconnects on error.
        throw MLBridgeException("String expected but not read from" MMANAME ".");
    }

    //Copy byte-for-byte into the output string buffer.
    output.assign((char *)stringBuffer, bytes);
    MMAReleaseUTF8String(link, stringBuffer, bytes);
    
    return output;
}

int MLBridge::GetNextPacket(){
    int packet;

    //Wait until the kernel is ready.
    MMAWaitForLinkActivity(link);

    //MLNewPacket skips to the end of the current packet even if we are already at the end. It's never an error to call MLNewPacket(), but it is an error to call MLNextPacket if we aren't finished with the previous packet.
    if(!MMANewPacket(link)) ErrorCheck();
    packet = MMANextPacket(link);
    if(packet == ILLEGALPKT) ErrorCheck();
    
    return packet;
}

void MLBridge::ErrorCheck(){
    int errorCode;
    std::string error;
    
    if(link == NULL){
        throw MLBridgeException("The " MMANAME " connection has been severed.", MMAEDEAD);
    }
    errorCode = MMAError(link);
    if(errorCode != MMAEOK){
        const char *errormsg = MMAErrorMessage(link);
        if(errormsg){
            error = std::string(errormsg);
            MMAReleaseErrorMessage(link, errormsg);
        }else{
            error = "Kernel Error, but " MMANAME " did not return an error description.";
        }
        /*
          TO DO: Don't disconnect on error. Allow caller to attempt to recover based on error state instead.
        */
        Disconnect();
        throw MLBridgeException(error, errorCode);
    }
}

void MLBridge::Evaluate(std::string inputString){
    //We record the inputString so that we can reference it later, for example, in the event of a syntax error.
    if(continueInput){
        inputString = this->inputString.append(inputString);
        continueInput = false;
    } else{
        this->inputString = inputString;
    }
    
    /*
     There are two kinds of strings we can send to the kernel: strings of Mathematica code (the typical case) and strings of arbitrary text (in the case of the kernel requesting user input). In addition, there are two ways to ask the kernel to process Mathematica code: as part of the "Main Loop" in which In[#] and Out[#] variables are set, etc., which is typical of a human-usable REPL, or as NOT part of the "Main Loop," which is more appropriate in cases where session history need not be accessed or retained.
     */
    if(inputMode == ExpressionMode){
        //The user has input Mathematica code.
        if(useMainLoop){
            //Maintain session history for this evaluation.
            MMAPutFunction(link, "EnterTextPacket", 1);
        }else{
            //Bypass the kernel's Main Loop.
            MMAPutFunction(link, "EvaluatePacket", 1);
            MMAPutFunction(link, "ToString", 1);
            MMAPutFunction(link, "ToExpression", 1);
        }
        
    } else if(inputMode == TextMode){
        //The user has input arbitrary text, from example in response to an InputString[] call.
        MMAPutFunction(link, "TextPacket", 1);
        //Turn off TextMode
        inputMode = ExpressionMode;
    }
    MMAPutUTF8String(link, (const unsigned char *)inputString.data(), (int)inputString.size());
    MMAEndPacket(link);
    //We check for errors after sending a packet.
    ErrorCheck();
    running = true;
}

void MLBridge::EvaluateWithoutMainLoop(std::string inputString, bool eatReturnPacket){
    // This function always circumvents the kernel's Main Loop. Use it for setting $PrePrint for example.
    
    
    //We record the inputString so that we can reference it later, for example, in the event of a syntax error.
    if(continueInput){
        inputString = this->inputString.append(inputString);
        continueInput = false;
    } else{
        this->inputString = inputString;
    }
    
    //Bypass the kernel's Main Loop.
    MMAPutFunction(link, "EvaluatePacket", 1);
    MMAPutFunction(link, "ToString", 1);
    MMAPutFunction(link, "ToExpression", 1);
    MMAPutUTF8String(link, (const unsigned char *)inputString.data(), (int)inputString.size());
    MMAEndPacket(link);
    //We check for errors after sending a packet.
    ErrorCheck();
    
    //The default is to discard the result.
    if(eatReturnPacket){
        GetNextPacket();
    } else{
        running = true;
    }
}

void MLBridge::SetPrePrint(std::string preprintfunction){
    EvaluateWithoutMainLoop("$PrePrint = " + preprintfunction);
}

std::string MLBridge::GetKernelVersion() {
    return GetEvaluated("$Version");
}

std::string MLBridge::GetEvaluated(std::string expression){
    
    EvaluateWithoutMainLoop(expression, false);
    //EvaluateWithoutMainLoop sets running to true, but we get the return string ourselves, so we leaving running = false;
    running = false;


    //Fetch the string returned by the kernel.
    if(GetNextPacket() != RETURNPKT){
        //Return an error.
        throw MLBridgeException("Kernel sent an unexpected packet in response to a request to evaluate $Version.");
    }
    
    return GetUTF8String();
}

bool MLBridge::IsRunning(){
    //If we never started, there's nothing to do!
    if(!running) return false;

    if(!MMAFlush(link) || !MMAReady(link)) {
        //Check if an error has occurred.
        ErrorCheck();
        running = true;
    } else{
        running = false;
    }
    return running;
}

void MLBridge::PrintMessages(){
    std::ostream &cout = *pcout;
    MLBridgeMessage *m;
    
    while(!messages.empty()){
        //Only print if we aren't continuing previous input.
        if(!continueInput){
            m = messages.front();
            cout << "\n" << m->message << std::endl;
            if(m->position > -1){
                cout << inputString << "\n";
                cout << std::string(m->position, '.');
                cout << "^ Syntax Error.\n" << std::endl;
            }
            
            delete m;
        }
        messages.pop();
    }
}

//The following represent In[#]:= strings and text that the kernel prints to the console respectively.
bool MLBridge::ReceivedInputNamePacket(){
    
    DebugPrint("<INPUTNAMEPKT>");
    if(!continueInput) *pcout << "\n\n";
    if(showInOutStrings && useMainLoop){
        kernelPrompt = GetUTF8String();
    }
    
    return true;
}

//Represents a prompt for user input.
bool MLBridge::ReceivedInputPacket(){

    DebugPrint("<INPUTPKT>");
    kernelPrompt = GetUTF8String();
    
    return true;
}

//The following represent Out[#]= strings and text that the kernel prints to the console respectively.
bool MLBridge::ReceivedOutputNamePacket(){
    std::ostream &cout = *pcout;
    std::string output;

    DebugPrint("<OUTPUTNAMEPKT>");
    
    //Print any cached messages.
    PrintMessages();
    
    cout << "\n";
    if(showInOutStrings){
        outputPrompt = GetUTF8String();
        cout << outputPrompt;
    }
    
    return false;
}

//Contains the text returned from the evaluation.
bool MLBridge::ReceivedReturnTextPacket(){
    std::ostream &cout = *pcout;

    DebugPrint("<RETURNTEXTPKT>");
    
    //Frankly, I'm not sure how to correctly format the output without starting to print it on a new line. There must be a way because Wolfram's interface does it.
    cout << "\n" << GetUTF8String();
    
    return false;
}

//We receive these packets when useMainLoop is false.
bool MLBridge::ReceivedReturnExpressionPacket(){
    std::ostream &cout = *pcout;

    DebugPrint("<RETURNPKT/RETURNEXPRPKT>");
    
    //Print any cached messages.
    PrintMessages();
    
    cout << GetUTF8String() << std::endl;

    //If we are using the Main Loop, we expect more packets from the kernel, so we keep done=false.
    return !useMainLoop;
}

//This packet typically contains the message (string) describing a syntax error.
bool MLBridge::ReceivedTextPacket(){
    std::ostream &cout = *pcout;

    DebugPrint("<TEXTPKT>");
    
    //We don't print if this text packet is for incomplete input syntax error.
    if(!continueInput){
        cout << GetUTF8String();
    }

    return false;
}

//This packet contains postscript code, i.e., the kernel is sending an image.
bool MLBridge::ReceivedDisplayPacket(){
    DebugPrint("<DISPLAYPKT>");

    if(makeNewImage){
        //If this is a new image (i.e. postscript string), create a new string to store the code.
        makeNewImage = false;
        image = new std::string();
        
        //If we want to include our own postscript preamble, this is where it would go.
    }
    
    image->append(GetUTF8String());
    
    return false;
}

//This packet is received when the kernel is done sending postscript code.
bool MLBridge::ReceivedDisplayEndPacket(){
    DebugPrint("<DISPLAYENDPKT>");
    
    image->append(GetUTF8String());

    //If we want to include our own postscript "post-amble", this is where it would go.
    images.push(*image);
    image = NULL;
    makeNewImage = true;
    
    //It is unclear if we still return false when useMainLoop is false.
    return false;
}

//The kernel reports a syntax error. This packet contains the position of the error.
bool MLBridge::ReceivedSyntaxPacket(){
    DebugPrint("<SYNTAXPKT>");

    //We cache syntax messages. This syntax packet must be associated to the last message cached. Record the position in that message's cache entry.
    MLBridgeMessage *m = messages.back();
    MMAGetInteger(link, &m->position);
    
    /*
     We don't throw an MLBridgeException because it's for errors associated to the link to the kernel, not for every possible error. Thus we do not throw an exception here. In fact, doing so would disrupt the internal state of the REPL. If one wishes to catch syntax errors, the best way is probably to implement a call-back function to handle them and call the function from here.
     */
    return false;
}

//This packet represents a request from the kernel for a plaintext string input from the user.
bool MLBridge::ReceivedInputStringPacket(){
    std::ostream &cout = *pcout;

    DebugPrint("<INPUTSTRPKT>");
    
    cout << GetUTF8String();
    
    inputMode = TextMode;
    return true;
}

//Sending an interrupt signal (i.e. ctrl+c) brings up the Interrupt> menu which expects textual user input.
bool MLBridge::ReceivedMenuPacket(){
    std::ostream &cout = *pcout;
    DebugPrint("<MENUPKT>");
    
    //What is this number? It seems to indicate that the kernel will subsequently output additional menu text, so we should expect it. (I think.) This happens when the user enters an invalid option at the Interrupt> menu.
    int interruptMenuNumber = 0;
    
    MMAGetInteger(link, &interruptMenuNumber);

    kernelPrompt = GetUTF8String();

    //The Interrupt> menu wants text input.
    inputMode = TextMode;

    //If we are getting a text menu...
    if(interruptMenuNumber == 0){
        //We are about to receive additional menu text from the kernel.
        if(GetNextPacket() != TEXTPKT){
            //We received a packet we didn't expect.
            throw MLBridgeException("Menu text expected but not received.");
        }
        DebugPrint("<TEXTPKT>");
        
        //Get the menu text from the text packet and print it.
        cout << GetUTF8String();
        
    } else{
        //Start on a new line.
        cout << "\n";
    }
    
    return true;
}

//Message from the kernel, for example, to indicate a runtime error.
bool MLBridge::ReceivedMessagePacket(){
    std::ostream &cout = *pcout;

    DebugPrint("<MESSAGEPKT>");

    /*
     The problem we need to solve here is that we want to silently continue reading input from the user if we receive "Syntax::sntxi", but otherwise we want to output the message. Problem is, we might receive other messages BEFORE "Syntax::sntxi". So we cache syntax messsages until they are all read from the kernel, check for the presence of "Syntax::sntxi", and print them or not.
     
     Is it true that we only ever get "Syntax" messages prior to "Syntax::sntxi"? If not, then the following code needs to be adjusted.
     */
    MLBridgeMessage *message;
    std::string symbolName = GetUTF8String(GetSymbol);
    std::string tag = GetUTF8String();
    
    //Syntax::sntxi:
    if(symbolName == "Syntax"){
        if(tag == "sntxi"){
            //Keep reading input.
            continueInput = true;
        }
        //Make a new message to stash.
        message = new MLBridgeMessage;
        message->name = symbolName;
        message->tag = tag;
        //Setting position = -1 indicates that there is no associated error position with this message.
        message->position = -1;
        
        //Get the text of the message from the kernel.
        GetNextPacket();
        
        message->message = GetUTF8String();
        
        //Stash the message.
        messages.push(message);
    } else if(!continueInput){
        //It's not a "Syntax::" message, and we haven't gotten a "Syntax::sntxi:" message, so go ahead and print any messages we've cached.
        PrintMessages();
        
        //Now get the text of this message from the kernel and print it.
        GetNextPacket();
        cout << "\n" << GetUTF8String() << std::endl;
    }
    return false;
}

//The SuspendPacket tells this link that something else, perhaps another front end, has taken control, and thus the kernel will start ignoring us.
bool MLBridge::ReceivedSuspendPacket(){
    DebugPrint("<SUSPENDPKT>");
    
    MMANewPacket(link); //Do I need this line?
    
    *pcout << "--suspended--" << std::endl;
    
    return true;
}

//The ResumePacket tells this link that something else, perhaps another front end, has given us back control, and thus the kernel will start paying attention to us again.
bool MLBridge::ReceivedResumePacket(){
    DebugPrint("<RESUMEPKT>");
    
    *pcout << "--resumed--" << std::endl;
    
    MMANewPacket(link); //Do I need this line?
    
    return false;
}

//A dialog is entered when a computation is interrupted and the user enters debug mode. Debug modes/dialogs can be nested.
bool MLBridge::ReceivedBeginDialogPacket(){
    DebugPrint("<BEGINDLGPKT>");
    
    int dialogLevel;
    MMAGetInteger(link, &dialogLevel);
    *pcout << "entering dialog:" << dialogLevel << std::endl;
    
    return false;
}

bool MLBridge::ReceivedEndDialogPacket(){
    DebugPrint("<ENDDLGPKT>");
    
    int dialogLevel;
    MMAGetInteger(link, &dialogLevel);
    dialogLevel--;
    *pcout << "leaving dialog:" << dialogLevel << std::endl;
    
    return false;
}

void MLBridge::ProcessKernelResponse() {
    bool done = false;
    std::string output;

    //Keep fetching packets until the kernel is finished responding.
    do {
        //We poll to see if MLNextPacket will block. If it will, just return true. We don't want to spend time blocking in MLNextPacket because we want to be able to send an MLInterruptMessage if we need to.
        while(IsRunning() );

        //Get the next packet.
        int packet = GetNextPacket();

        switch (packet) {
            case INPUTNAMEPKT:
                done = ReceivedInputNamePacket();
                break;
            case INPUTPKT:
                done = ReceivedInputPacket();
                break;
            case OUTPUTNAMEPKT:
                done = ReceivedOutputNamePacket();
                break;
            case RETURNTEXTPKT:
                done = ReceivedReturnTextPacket();
                break;
            case RETURNPKT:
                //Handled by RETURNEXPRPKT.
            case RETURNEXPRPKT:
                done = ReceivedReturnExpressionPacket();
                break;
            case TEXTPKT:
                done = ReceivedTextPacket();
                break;
            case MESSAGEPKT:
                done = ReceivedMessagePacket();
                break;
            case SYNTAXPKT:
                done = ReceivedSyntaxPacket();
                break;
            case INPUTSTRPKT:
                done = ReceivedInputStringPacket();
                break;
            case MENUPKT:
                done = ReceivedMenuPacket();
                break;
            case DISPLAYPKT:
                done = ReceivedDisplayPacket();
                break;
            case DISPLAYENDPKT:
                done = ReceivedDisplayEndPacket();
                break;
            case SUSPENDPKT:
                done = ReceivedSuspendPacket();
                break;
            case RESUMEPKT:
                done = ReceivedResumePacket();
                break;
            case BEGINDLGPKT:
                done = ReceivedBeginDialogPacket();
                break;
            case ENDDLGPKT:
                done = ReceivedEndDialogPacket();
                break;
            case ILLEGALPKT:
                DebugPrint("<ILLEGALPKT>");
                //We should attempt recovery.
                done = true;
                break;
            default: //Unhandled packet.
                DebugPrint("<unknown?>");
                break;
        } //End switch.

        //Print any cached messages we haven't printed yet.
        if (done) PrintMessages();

        running = !done;
    }while(!done);

    return;
}
