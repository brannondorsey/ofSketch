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


const std::string App::VERSION_SPECIAL = "";

App::App():
    DATA_PATH(ofToDataPath("", true)),
    _editorSettings(Poco::Path(DATA_PATH).append(Poco::Path("Resources/Settings/EditorSettings.json", Poco::Path::PATH_UNIX)).toString(Poco::Path::PATH_NATIVE)),
    _ofSketchSettings(),
    _threadPool("ofSketchThreadPool"),
    _taskQueue(ofx::TaskQueue_<std::string>::UNLIMITED_TASKS, _threadPool),
    _compiler(_taskQueue,
              Poco::Path(ofToDataPath("Resources/Templates/CompilerTemplates"), Poco::Path::PATH_UNIX).toString(Poco::Path::PATH_NATIVE),
              _ofSketchSettings.getOpenFrameworksDir()),
    _addonManager(Poco::Path(ofToDataPath(_ofSketchSettings.getOpenFrameworksDir())).append(Poco::Path("addons")).toString(Poco::Path::PATH_NATIVE)),
    _projectManager(Poco::Path(ofToDataPath(_ofSketchSettings.getProjectDir(), true)).toString(Poco::Path::PATH_NATIVE)),
    _uploadRouter(Poco::Path(ofToDataPath(_ofSketchSettings.getProjectDir(), true)).toString(Poco::Path::PATH_NATIVE))
{
    
    cout << "The data path is " << DATA_PATH << endl;

    ofLogNotice("App::App") << "Editor setting's projectDir: " << _ofSketchSettings.getProjectDir();
    _taskQueue.registerTaskEvents(this);

    ofLogNotice("App::App") << "Starting server on port: " << _ofSketchSettings.getPort() << " With Websocket Buffer Size: " << _ofSketchSettings.getBufferSize();

    ofx::HTTP::BasicJSONRPCServerSettings settings; // TODO: load from file.
    settings.setBufferSize(_ofSketchSettings.getBufferSize());
    settings.setPort(_ofSketchSettings.getPort());
    settings.setUploadRedirect("");
    settings.setMaximumFileUploadSize(5000000*1024); // 50 GB
    
    server = ofx::HTTP::BasicJSONRPCServer::makeShared(settings);

    // Must register for all events before initializing server.
    ofSSLManager::registerAllEvents(this);

    server->getPostRoute()->registerPostEvents(&_uploadRouter);
    server->getWebSocketRoute()->registerWebSocketEvents(this);

    // Set up websocket logger.
    // _loggerChannel = WebSocketLoggerChannel::makeShared();
    // _loggerChannel->setWebSocketRoute(server->getWebSocketRoute());
    // ofSetLoggerChannel(_loggerChannel);

    // _logo.loadImage("media/openFrameworks.png");
    _font.loadFont(OF_TTF_SANS, 20);
}


App::~App()
{
    _taskQueue.unregisterTaskEvents(this);

    server->getWebSocketRoute()->unregisterWebSocketEvents(this);
    server->getPostRoute()->unregisterPostEvents(&_uploadRouter);

    ofSSLManager::unregisterAllEvents(this);
}


void App::setup()
{
    ofSetFrameRate(30);

    ofSetLogLevel("ofThread", OF_LOG_ERROR);
    ofSetLogLevel(OF_LOG_VERBOSE);

    ofSSLManager::initializeServer(new Poco::Net::Context(Poco::Net::Context::SERVER_USE,
                                                          Poco::Path(ofToDataPath("ssl/privateKey.nopassword.pem"),
                                                                     Poco::Path::PATH_UNIX).toString(Poco::Path::PATH_NATIVE),
                                                          Poco::Path(ofToDataPath("ssl/selfSignedCertificate.nopassword.pem"),
                                                                     Poco::Path::PATH_UNIX).toString(Poco::Path::PATH_NATIVE),
                                                          Poco::Path(ofToDataPath("ssl/cacert.pem"), Poco::Path::PATH_UNIX).toString()));

    // TODO: configure these via settings files

    server->registerMethod("load-project",
                           "Load the requested project.",
                           this,
                           &App::loadProject);

    server->registerMethod("load-template-project",
                           "Load an anonymous project.",
                           this,
                           &App::loadTemplateProject);

    server->registerMethod("save-project",
                           "Save the current project.",
                           this,
                           &App::saveProject);

    server->registerMethod("create-project",
                           "Create a new project.",
                           this,
                           &App::createProject);

    server->registerMethod("delete-project",
                           "Delete the current project.",
                           this,
                           &App::deleteProject);

    server->registerMethod("rename-project",
                           "Rename the current project.",
                           this,
                           &App::renameProject);

    server->registerMethod("notify-project-closed",
                           "Notify the server that project was closed.",
                           this,
                           &App::notifyProjectClosed);

    server->registerMethod("request-project-closed",
                           "Broadcast a project close request to connected clients.",
                           this,
                           &App::requestProjectClosed);

    server->registerMethod("request-app-quit",
                           "Quit the app.",
                           this,
                           &App::requestAppQuit);

    server->registerMethod("create-class",
                           "Create a new class for the current project.",
                           this,
                           &App::createClass);

    server->registerMethod("delete-class",
                           "Delete a select class from for the current project.",
                           this,
                           &App::deleteClass);

    server->registerMethod("rename-class",
                           "Rename a select class from for the current project.",
                           this,
                           &App::renameClass);

    server->registerMethod("run-project",
                           "Run the requested project.",
                           this,
                           &App::runProject);

    server->registerMethod("compile-project",
                           "Run the requested project.",
                           this,
                           &App::compileProject);

    server->registerMethod("stop",
                           "Stop the requested project.",
                           this,
                           &App::stop);

    server->registerMethod("get-project-list",
                           "Get list of all projects in the Project directory.",
                           this,
                           &App::getProjectList);

    server->registerMethod("load-editor-settings",
                           "Get the editor settings.",
                           this,
                           &App::loadEditorSettings);

    server->registerMethod("save-editor-settings",
                           "Save the editor settings.",
                           this,
                           &App::saveEditorSettings);
    
    server->registerMethod("load-ofsketch-settings",
                           "Get ofSketch settings.",
                           this,
                           &App::loadOfSketchSettings);
    
    server->registerMethod("save-ofsketch-settings",
                           "Save ofSketch settings.",
                           this,
                           &App::saveOfSketchSettings);
    
    server->registerMethod("get-addon-list",
                           "Get a list of all addons.",
                           this,
                           &App::getAddonList);
    
    server->registerMethod("get-project-addon-list",
                           "Get a list of addons for a project.",
                           this,
                           &App::getProjectAddonList);
    
    server->registerMethod("add-project-addon",
                           "Add an addon to a project.",
                           this,
                           &App::addProjectAddon);
    
    server->registerMethod("remove-project-addon",
                           "Remove an addon from a project.",
                           this,
                           &App::removeProjectAddon);

    
    server->start();


    ofTargetPlatform arch = getTargetPlatform();

    if (arch != OF_TARGET_LINUXARMV6L && arch != OF_TARGET_LINUXARMV7L)
    {
        // Launch a browser with the address of the server.
        ofLaunchBrowser(server->getURL() + "/?project=HelloWorld");
    }
}


void App::update()
{
}


void App::draw()
{
    ofBackground(255);

    // _logo.draw(10, 0);

    ofSetColor(80);
    _font.drawString("Launch", 70, 30);
}

void App::exit()
{
    // broadcast requestProjectClosed settings to all connected clients
    Json::Value params;
    params["foo"] = "bar";
    Json::Value json = App::toJSONMethod("Server", "appExit", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
    ofLogNotice("App::exit") << "appExit frame broadcasted" << endl;

    // Reset default logger.
    ofLogToConsole();
}


void App::mousePressed(int x, int y, int button)
{
    ofLaunchBrowser(server->getURL());
}


void App::loadProject(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    if (args.params.isMember("projectName"))
    {
        std::string projectName = args.params["projectName"].asString();

        if (_projectManager.projectExists(projectName))
        {
            _projectManager.loadProject(pSender, args);
        }
        else args.error["message"] = "The requested project does not exist.";
    }
    else args.error["message"] = "Incorrect parameters sent to load-project method.";
}


void App::loadTemplateProject(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    _projectManager.loadTemplateProject(pSender, args);
}


void App::saveProject(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectData"]["projectFile"]["name"].asString();

    if (_projectManager.projectExists(projectName))
    {
        _projectManager.saveProject(pSender, args);
        const Project& project = _projectManager.getProject(projectName);
        _compiler.generateSourceFiles(project);
    }
    else args.error["message"] = "The requested project does not exist.";
}


void App::createProject(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    if (!_projectManager.projectExists(projectName))
    {
        _projectManager.createProject(pSender, args);
    }
    else args.error["message"] = "That project name already exists.";
}


void App::deleteProject(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    if (_projectManager.projectExists(projectName))
    {
        _projectManager.deleteProject(pSender, args);
        requestProjectClosed(pSender, args);
    }
    else args.error["message"] = "The project that you are trying to delete does not exist.";
}


void App::renameProject(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    if (_projectManager.projectExists(projectName))
    {
        _projectManager.renameProject(pSender, args);
        requestProjectClosed(pSender, args);
    }
    else args.error["message"] = "The project that you are trying to delete does not exist.";
}


void App::notifyProjectClosed(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    _projectManager.notifyProjectClosed(projectName);
    ofLogNotice("App::notifyProjectClosed") << projectName << " closed.";
}
    
void App::requestProjectClosed(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    std::string clientUUID = args.params["clientUUID"].asString();

    // broadcast requestProjectClosed settings to all connected clients
    Json::Value params;
    params["projectName"] = projectName;
    params["clientUUID"] = clientUUID;
    Json::Value json = App::toJSONMethod("Server", "requestProjectClosed", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
}


void App::requestAppQuit(const void *pSender, ofx::JSONRPC::MethodArgs &args)
{
//    args.result = "App quit";
//    ofExit();
}


void App::createClass(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();

    if (_projectManager.projectExists(projectName))
    {
        std::string className = args.params["className"].asString();
        Project& project = _projectManager.getProjectRef(projectName);
        args.result["classFile"] = project.createClass(className);

    }
    else args.error["message"] = "The requested project does not exist.";
}


void App::deleteClass(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    if (_projectManager.projectExists(projectName))
    {
        std::string className = args.params["className"].asString();
        Project& project = _projectManager.getProjectRef(projectName);
        if (project.deleteClass(className))
        {
            args.result["message"] = className + "class deleted.";
        }
        else args.error["message"] = "Error deleting the class.";
    }
    else args.error["message"] = "The requested project does not exist.";
}


void App::renameClass(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    if (_projectManager.projectExists(projectName))
    {
        std::string className = args.params["className"].asString();
        std::string newClassName = args.params["newClassName"].asString();
        Project& project = _projectManager.getProjectRef(projectName);
        if (project.renameClass(className, newClassName))
        {
            args.result["message"] = className + " class renamed to " + newClassName;
        }
        else args.error["message"] = "Error renaming " + className + " class.";
    }
    else args.error["message"] = "The requested project does not exist.";
}


void App::runProject(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    if (_projectManager.projectExists(projectName))
    {
        ofLogNotice("App::run") << "Running " << projectName << " project";
        const Project& project = _projectManager.getProject(projectName);
        Poco::UUID taskId = _compiler.run(project);
        ofLogNotice("APP::run") << "Task ID: " << taskId.toString();
        args.result = taskId.toString();
    }
    else args.error["message"] = "The requested project does not exist.";
}

void App::compileProject(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    if (_projectManager.projectExists(projectName))
    {
        ofLogNotice("App::compileProject") << "Compiling " << projectName << " project";
        const Project& project = _projectManager.getProject(projectName);
        Poco::UUID taskId = _compiler.compile(project);
        ofLogNotice("App::compileProject") << "Task ID: " << taskId.toString();
        args.result = taskId.toString();
    }
    else args.error["message"] = "The requested project does not exist.";
}

void App::stop(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    if (args.params.isMember("taskId"))
    {
        Poco::UUID taskId(args.params["taskId"].asString());
        _taskQueue.cancel(taskId);
        ofLogNotice("App::stop") << "Stopped task " << taskId.toString();
    }
    else
    {
        // Invalid.
        args.error = "No task id.";
    }
}
    
void App::getProjectList(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    _projectManager.getProjectList(pSender, args);
}


void App::getAddonList(const void *pSender, ofx::JSONRPC::MethodArgs &args)
{
    ofLogVerbose("App::getAddonList") << " sending addon list.";
    Json::Value addonsJSON;
    
    std::vector<Addon::SharedPtr> addons = _addonManager.getAddons();
    
    std::vector<Addon::SharedPtr>::const_iterator iter = addons.begin();
    
    while (iter != addons.end())
    {
        Json::Value addon;
        addon["name"] = (*iter)->getName();
        addon["path"] = Poco::Path((*iter)->getPath(), Poco::Path::PATH_UNIX).toString();
        addonsJSON.append(addon);
        ++iter;
    }
    
    args.result = addonsJSON;
}
    
void App::getProjectAddonList(const void *pSender, ofx::JSONRPC::MethodArgs &args)
{
    std::string projectName = args.params["projectName"].asString();
    if (_projectManager.projectExists(projectName))
    {
        const Project& project = _projectManager.getProject(projectName);

        if (project.hasAddons()) {
            
            std::vector<std::string> addons = project.getAddons();

            for (unsigned int i = 0; i < addons.size(); i++) {
                args.result["addons"][i] = addons[i];
            }
            
            args.result["hasAddons"] = true;
        }
        else
        {
            args.result["hasAddons"] = false;
        }
        
    }
    else args.error["message"] = "The requested project does not exist.";
}
    
void App::addProjectAddon(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    std::string addon = args.params["addon"].asString();
    
    if (_projectManager.projectExists(projectName))
    {
        Project& project = _projectManager.getProjectRef(projectName);
        project.addAddon(addon);
    }
    else args.error["message"] = "The requested project does not exist.";
}
    
void App::removeProjectAddon(const void* pSender, ofx::JSONRPC::MethodArgs& args)
{
    std::string projectName = args.params["projectName"].asString();
    std::string addon = args.params["addon"].asString();
    
    if (_projectManager.projectExists(projectName))
    {
        Project& project = _projectManager.getProjectRef(projectName);
        project.removeAddon(addon);
    }
    else args.error["message"] = "The requested project does not exist.";
}


void App::loadEditorSettings(const void *pSender, ofx::JSONRPC::MethodArgs &args)
{
    args.result = _editorSettings.getData();
}


void App::saveEditorSettings(const void *pSender, ofx::JSONRPC::MethodArgs &args)
{
    ofLogVerbose("App::saveEditorSettings") << "Saving editor settings" << endl;
    Json::Value settings = args.params["data"]; // must make a copy
    _editorSettings.update(settings);
    _editorSettings.save();

    // broadcast new editor settings to all connected clients
    Json::Value params;
    params["data"] = settings;
    params["clientUUID"] = args.params["clientUUID"];
    ofLogNotice("App::saveEditorSettings") << "clientUUID: " << params["clientUUID"] << endl;
    Json::Value json = App::toJSONMethod("Server", "updateEditorSettings", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
}
    
void App::loadOfSketchSettings(const void *pSender, ofx::JSONRPC::MethodArgs &args)
{
    args.result = _ofSketchSettings.getData();
}

void App::saveOfSketchSettings(const void *pSender, ofx::JSONRPC::MethodArgs &args)
{
    ofLogVerbose("App::saveOfSketchSettings") << "Saving ofSketch settings" << endl;
    
    Json::Value settings = args.params["data"]; // must make a copy
    
    _ofSketchSettings.update(settings);
    _ofSketchSettings.save();
    
    // broadcast new editor settings to all connected clients
    Json::Value params;
    params["data"] = settings;
    params["clientUUID"] = args.params["clientUUID"];
    
    Json::Value json = App::toJSONMethod("Server", "updateOfSketchSettings", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
}

bool App::onWebSocketOpenEvent(ofx::HTTP::WebSocketOpenEventArgs& args)
{
    ofLogVerbose("App::onWebSocketOpenEvent") << "Connection opened from: " << args.getConnectionRef().getClientAddress().toString();

    // Here, we need to send all initial values, settings, etc to the
    // client before any other messages arrive.

    Json::Value params = _taskQueue.toJson();

    Json::Value json = App::toJSONMethod("TaskQueue", "taskList", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));

    // Send the update to the client that just connected.
    args.getConnectionRef().sendFrame(frame);

    // Send version info.
    params.clear();
    params["version"] = getVersion();
    params["major"] = getVersionMajor();
    params["minor"] = getVersionMinor();
    params["patch"] = getVersionPatch();
    params["special"] = getVersionSpecial();
    params["target"] = toString(getTargetPlatform());

    json = App::toJSONMethod("Server", "version", params);
    frame = ofx::HTTP::WebSocketFrame(App::toJSONString(json));

    args.getConnectionRef().sendFrame(frame);

//    params.clear();
//    
//    params["editorSettings"] = _editorSettings.getData();
//    json = App::toJSONMethod("Server", "editorSettings", params);
//    frame = ofx::HTTP::WebSocketFrame(App::toJSONString(json));
//    
//    args.getConnectionRef().sendFrame(frame);
    
    return false; // did not handle it
}


bool App::onWebSocketCloseEvent(ofx::HTTP::WebSocketCloseEventArgs& args)
{
    std::stringstream ss;

    ss << "Connection closed from: " << args.getConnectionRef().getClientAddress().toString() << " ";
    ss << "Code: " << args.getCode() << " Reason: " << args.getReason();

    ofLogVerbose("App::onWebSocketCloseEvent") << ss.str();

    return false; // did not handle it
}


bool App::onWebSocketFrameReceivedEvent(ofx::HTTP::WebSocketFrameEventArgs& args)
{
    ofLogVerbose("App::onWebSocketFrameReceivedEvent") << "Frame received from: " << args.getConnectionRef().getClientAddress().toString();
    return false; // did not handle it
}


bool App::onWebSocketFrameSentEvent(ofx::HTTP::WebSocketFrameEventArgs& args)
{
//    ofLogVerbose("App::onWebSocketFrameSentEvent") << "Frame sent to: " << args.getConnectionRef().getClientAddress().toString();
    return false; // did not handle it
}


bool App::onWebSocketErrorEvent(ofx::HTTP::WebSocketErrorEventArgs& args)
{
    ofLogError("App::onWebSocketErrorEvent") << "Stop: " << args.getError();
//    ofLogVerbose("App::onWebSocketErrorEvent") << "Error on: " << args.getConnectionRef().getClientAddress().toString();
    return false; // did not handle it
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


bool App::onTaskQueued(const ofx::TaskQueuedEventArgs& args)
{
    Json::Value params;
    params["name"] = args.getTaskName();
    params["uuid"] = args.getTaskId().toString();
    Json::Value json = App::toJSONMethod("TaskQueue", "taskQueued", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
    return false;
}


bool App::onTaskStarted(const ofx::TaskStartedEventArgs& args)
{
    Json::Value params;
    params["name"] = args.getTaskName();
    params["uuid"] = args.getTaskId().toString();
    Json::Value json = App::toJSONMethod("TaskQueue", "taskStarted", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
    return false;
}


bool App::onTaskCancelled(const ofx::TaskCancelledEventArgs& args)
{
    Json::Value params;
    params["name"] = args.getTaskName();
    params["uuid"] = args.getTaskId().toString();
    Json::Value json = App::toJSONMethod("TaskQueue", "taskCancelled", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
    return false;
}


bool App::onTaskFinished(const ofx::TaskFinishedEventArgs& args)
{
    Json::Value params;
    params["name"] = args.getTaskName();
    params["uuid"] = args.getTaskId().toString();
    Json::Value json = App::toJSONMethod("TaskQueue", "taskFinished", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
    return false;
}


bool App::onTaskFailed(const ofx::TaskFailedEventArgs& args)
{
    Json::Value params;
    params["name"] = args.getTaskName();
    params["uuid"] = args.getTaskId().toString();
    params["exception"] = args.getException().displayText();
    Json::Value json = App::toJSONMethod("TaskQueue", "taskFailed", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
    return false;
}


bool App::onTaskProgress(const ofx::TaskProgressEventArgs& args)
{
    Json::Value params;
    params["name"] = args.getTaskName();
    params["uuid"] = args.getTaskId().toString();
    params["progress"] = args.getProgress();
    Json::Value json = App::toJSONMethod("TaskQueue", "taskProgress", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
    return false;
}


bool App::onTaskData(const ofx::TaskDataEventArgs<std::string>& args)
{
    // We use the custom events to send status messages, and other custom
    // events for custom data specific events.
    Json::Value params;
    params["name"] = args.getTaskName();
    params["uuid"] = args.getTaskId().toString();
    params["message"] = args.getData();
    
    Json::Value error = _compiler.parseError(args.getData());

    if (!error.empty())
    {
        params["compileError"] = error;
    }
    
    Json::Value json = App::toJSONMethod("TaskQueue", "taskMessage", params);
    ofx::HTTP::WebSocketFrame frame(App::toJSONString(json));
    server->getWebSocketRoute()->broadcast(frame);
    return false;
}


Json::Value App::toJSONMethod(const std::string& module,
                              const std::string& method,
                              const Json::Value& params)
{
    Json::Value json;
    json["ofSketch"] = "1.0";
    json["module"] = module;
    json["method"] = method;
    json["params"] = params;
    return json;
}


std::string App::toJSONString(const Json::Value& json)
{
    Json::FastWriter writer;
    return writer.write(json);
}


std::string App::getVersion()
{
    std::stringstream ss;

    ss << VERSION_MAJOR << ".";
    ss << VERSION_MINOR << ".";
    ss << VERSION_PATCH;

    if (!VERSION_SPECIAL.empty())
    {
        ss << "-" << VERSION_SPECIAL;
    }

    return ss.str();
}

int App::getVersionMajor()
{
    return VERSION_MAJOR;
}


int App::getVersionMinor()
{
    return VERSION_MINOR;
}


int App::getVersionPatch()
{
    return VERSION_PATCH;
}


std::string App::getVersionSpecial()
{
    return VERSION_SPECIAL;
}


////////////////////////////////////////////////////////////////////////////////


#include "Poco/Process.h"
#include "Poco/PipeStream.h"
#include "Poco/StreamCopier.h"

ofTargetPlatform App::getTargetPlatform()
{
#ifdef TARGET_LINUX
    std::string cmd("uname");
    std::vector<std::string> args;
    args.push_back("-m");
    Poco::Pipe outPipe;
    Poco::ProcessHandle ph = Poco::Process::launch(cmd, args, 0, &outPipe, 0);
    Poco::PipeInputStream istr(outPipe);

    std::stringstream ostr;

    Poco::StreamCopier::copyStream(istr, ostr);

    std::string arch = ostr.str();

    if(ofIsStringInString(arch,"x86_64"))
    {
        return OF_TARGET_LINUX64;
    }
    else if(ofIsStringInString(arch,"armv6l"))
    {
        return OF_TARGET_LINUXARMV6L;
    }
    else if(ofIsStringInString(arch,"armv7l"))
    {
        return OF_TARGET_LINUXARMV7L;
    }
    else
    {
        return OF_TARGET_LINUX;
    }

#elif defined(TARGET_OSX)
        return OF_TARGET_OSX;
#elif defined(TARGET_WIN32)
#if (_MSC_VER)
        return OF_TARGET_WINVS;
#else
        return OF_TARGET_WINGCC;
#endif
#elif defined(TARGET_ANDROID)
        return OF_TARGET_ANDROID;
#elif defined(TARGET_OF_IOS)
        return OF_TARGET_IOS;
#elif defined(TARGET_EMSCRIPTEN)
        return OF_TARGET_EMSCRIPTEN;
#endif
}


std::string App::toString(ofTargetPlatform targetPlatform)
{
    switch (targetPlatform)
    {
        case OF_TARGET_OSX:
            return "OSX";
        case OF_TARGET_WINGCC:
            return "WINGCC";
        case OF_TARGET_WINVS:
            return "WINVS";
        case OF_TARGET_IOS:
            return "IOS";
        case OF_TARGET_ANDROID:
            return "ANDROID";
        case OF_TARGET_LINUX:
            return "LINUX";
        case OF_TARGET_LINUX64:
            return "LINUX64";
        case OF_TARGET_LINUXARMV6L: // arm v6 little endian
            return "LINUXARMV6L";
        case OF_TARGET_LINUXARMV7L: // arm v7 little endian
            return "LINUXARMV7L";
        default:
            return "UNKNOWN";
    }
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace of::Sketch
