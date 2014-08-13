// =============================================================================
//
// Copyright (c) 2014 Brannon Dorsey <http://christopherbaker.net>
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


#pragma once


#include <string>
#include <json/json.h>
#include "Poco/Pipe.h"
#include "Poco/UUID.h"
#include "Poco/Process.h"
#include "Poco/StreamCopier.h"
#include "Poco/PipeStream.h"
#include "Poco/RegularExpression.h"
#include "Poco/Path.h"
#include "ofTypes.h"
#include "Project.h"
#include "ofMain.h"
#include "ProcessTaskQueue.h"
#include "MakeTask.h"
#include "RunTask.h"


namespace of {
namespace Sketch {


class Compiler
{
public:
    Compiler(ProcessTaskQueue& taskQueue,
             const std::string& pathToTemplates,
             const std::string& openFrameworksDir);

    Poco::UUID compile(const Project& project);
    Poco::UUID run(const Project& project);

    void generateSourceFiles(const Project& project);
    Json::Value parseError(std::string message) const;
    
private:
    ProcessTaskQueue& _taskQueue;

    std::string _pathToTemplates;
    std::string _pathToSrc;
    std::string _projectFileTemplate;
    std::string _classTemplate;
    std::string _openFrameworksDir;
    
    void _parseAddons();
    void _getAddons();

    void _replaceIncludes(std::string& fileContents);
};


} } // namespace of::Sketch
