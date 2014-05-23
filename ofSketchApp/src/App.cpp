// =============================================================================
//
// Copyright (c) 2013-2014 Christopher Baker <http://christopherbaker.net>
//               2014 Brannon Dorsey <http://brannondorsey.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =============================================================================


#include "App.h"


namespace of {
namespace Sketch {


App::App()
{
    // Must register for all events before initializing server.
    ofSSLManager::registerAllEvents(this);

    server.getPostRoute()->registerPostEvents(this);
    server.getWebSocketRoute()->registerWebSocketEvents(this);
}


App::~App()
{
    server.getWebSocketRoute()->unregisterWebSocketEvents(this);
    server.getPostRoute()->unregisterPostEvents(this);

    ofSSLManager::unregisterAllEvents(this);
}


void App::setup()
{
    ofSetFrameRate(30);

    ofSetLogLevel(OF_LOG_VERBOSE);

    ofSSLManager::initializeServer(new Poco::Net::Context(Poco::Net::Context::SERVER_USE,
                                                          ofToDataPath("ssl/privateKey.nopassword.pem"),
                                                          ofToDataPath("ssl/selfSignedCertificate.nopassword.pem"),
                                                          ofToDataPath("ssl/cacert.pem")));

    // TODO: configure these via settings files
    _projectManager = ProjectManager::makeShared(ofToDataPath("Projects"));
    _addonManager = AddonManager::makeShared(ofToDataPath("openFrameworks/addons"));
    _compiler = Compiler(ofToDataPath("Resources/Templates/CompilerTemplates"));

    HTTP::BasicJSONRPCServerSettings settings; // TODO: load from file.

    // settings.setUseSSL(true);

    server.setup(settings);

    server.registerMethod("load-project",
                          "Load the requested project.",
                          this,
                          &App::loadProject);
    
    server.registerMethod("save-project",
                          "Save the current project.",
                          this,
                          &App::saveProject);

    server.registerMethod("run",
                          "Run the requested project.",
                          this,
                          &App::run);

    server.registerMethod("stop",
                          "Stop the requested project.",
                          this,
                          &App::stop);
    
    server.registerMethod("get-project-list",
                          "Get list of all projects in the Project directory.",
                          this,
                          &App::getProjectList);

    // start the server
    server.start();

    // Launch a browser with the address of the server.
    ofLaunchBrowser(server.getURL());
}

void App::update()
{
    
}

void App::draw()
{
    ofBackground(0);
}

void App::loadProject(const void* pSender, JSONRPC::MethodArgs& args)
{
    _projectManager->loadProject(pSender, args);
}
    
void App::saveProject(const void* pSender, JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectData"]["projectFile"]["name"].asString();
    
    if (_projectManager->projectExists(projectName)) {
    
        cout<<"Project Saved!"<<endl;
        _projectManager->saveProject(pSender, args);
        const Project& project = _projectManager->getProject(projectName);
        _compiler.generateSourceFiles(project);
    }
}

void App::run(const void* pSender, JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectData"]["projectFile"]["name"].asString();
    
    if (_projectManager->projectExists(projectName)) {
        
        cout<<"Running project..."<<endl;
        const Project& project = _projectManager->getProject(projectName);
        _compiler.run(project);
        
    }
}

void App::stop(const void* pSender, JSONRPC::MethodArgs& args)
{
    std::cout << "stop: " << std::endl;
    std::cout << args.params.toStyledString() << std::endl;
}

void App::getProjectList(const void* pSender, JSONRPC::MethodArgs& args)
{
    _projectManager->getProjectList(pSender, args);
};

bool App::onWebSocketOpenEvent(HTTP::WebSocketEventArgs& args)
{
//    ofLogVerbose("App::onWebSocketOpenEvent") << "Connection opened from: " << args.getConnectionRef().getClientAddress().toString();
    return false; // did not handle it
}


bool App::onWebSocketCloseEvent(HTTP::WebSocketEventArgs& args)
{
//    ofLogVerbose("App::onWebSocketCloseEvent") << "Connection closed from: " << args.getConnectionRef().getClientAddress().toString();
    return false; // did not handle it
}


bool App::onWebSocketFrameReceivedEvent(HTTP::WebSocketFrameEventArgs& args)
{
//    ofLogVerbose("App::onWebSocketFrameReceivedEvent") << "Frame received from: " << args.getConnectionRef().getClientAddress().toString();
    return false; // did not handle it
}


bool App::onWebSocketFrameSentEvent(HTTP::WebSocketFrameEventArgs& args)
{
//    ofLogVerbose("App::onWebSocketFrameSentEvent") << "Frame sent to: " << args.getConnectionRef().getClientAddress().toString();
    return false; // did not handle it
}


bool App::onWebSocketErrorEvent(HTTP::WebSocketEventArgs& args)
{
//    ofLogVerbose("App::onWebSocketErrorEvent") << "Error on: " << args.getConnectionRef().getClientAddress().toString();
    return false; // did not handle it
}


bool App::onHTTPPostEvent(HTTP::PostEventArgs& args)
{
//    ofLogNotice("ofApp::onHTTPPostEvent") << "Data: " << args.getBuffer().getText();
    return false;
}


bool App::onHTTPFormEvent(HTTP::PostFormEventArgs& args)
{
//    ofLogNotice("ofApp::onHTTPFormEvent") << "";
//    HTTP::Utils::dumpNameValueCollection(args.getForm(), ofGetLogLevel());
    return false;
}


bool App::onHTTPUploadEvent(HTTP::PostUploadEventArgs& args)
{
//    std::string stateString = "";
//
//    switch (args.getState())
//    {
//        case HTTP::PostUploadEventArgs::UPLOAD_STARTING:
//            stateString = "STARTING";
//            break;
//        case HTTP::PostUploadEventArgs::UPLOAD_PROGRESS:
//            stateString = "PROGRESS";
//            break;
//        case HTTP::PostUploadEventArgs::UPLOAD_FINISHED:
//            stateString = "FINISHED";
//            break;
//    }
//
//    ofLogNotice("ofApp::onHTTPUploadEvent") << "";
//    ofLogNotice("ofApp::onHTTPUploadEvent") << "         state: " << stateString;
//    ofLogNotice("ofApp::onHTTPUploadEvent") << " formFieldName: " << args.getFormFieldName();
//    ofLogNotice("ofApp::onHTTPUploadEvent") << "orig. filename: " << args.getOriginalFilename();
//    ofLogNotice("ofApp::onHTTPUploadEvent") <<  "      filename: " << args.getFilename();
//    ofLogNotice("ofApp::onHTTPUploadEvent") <<  "      fileType: " << args.getFileType().toString();
//    ofLogNotice("ofApp::onHTTPUploadEvent") << "# bytes xfer'd: " << args.getNumBytesTransferred();
    return false;
}


void App::onSSLServerVerificationError(Poco::Net::VerificationErrorArgs& args)
{
    ofLogVerbose("ofApp::onServerVerificationError") << args.errorMessage();

    // if you want to proceed, you should allow your user to inspect
    // the certificate and set:
    //     args.setIgnoreError(true);
    // if they approve
}


void App::onSSLClientVerificationError(Poco::Net::VerificationErrorArgs& args)
{
    ofLogVerbose("ofApp::onClientVerificationError") << args.errorMessage();

    std::stringstream ss;

    ss << "Error: " << args.errorMessage() << " #" << args.errorNumber() << " depth: " << args.errorDepth() << std::endl;

    ss << "Certificate: " << std::endl;

    ss << "Issued By: " << args.certificate().issuerName() << std::endl;
    ss << "Subject Name: " << args.certificate().issuerName() << std::endl;
    ss << "Common Name: " << args.certificate().issuerName() << std::endl;

    Poco::DateTime ldtValidFrom = args.certificate().validFrom();
    Poco::DateTime ldtExpiresOn = args.certificate().expiresOn();

    ss << "Valid From: " << Poco::DateTimeFormatter::format(ldtValidFrom, "%dd %H:%M:%S.%i") << std::endl;
    ss << "Expires On: " << Poco::DateTimeFormatter::format(ldtExpiresOn, "%dd %H:%M:%S.%i") << std::endl;

    ofLogVerbose("ofApp::onServerVerificationError") << ss.str();

    // if you want to proceed, you should allow your user to inspect
    // the certificate and set:
    //     args.setIgnoreError(true);
    // if they approve
}


void App::onSSLPrivateKeyPassphraseRequired(std::string& args)
{
    ofLogVerbose("ofApp::onPrivateKeyPassphraseRequired") << args;
    
    // if you want to proceed, you should allow your user set the
    // the certificate and set:
    args = "password";
}


} } // namespace of::Sketch
