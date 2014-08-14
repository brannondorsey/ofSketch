// =============================================================================
//
// Copyright (c) 2014 Brannon Dorsey <http://brannondorsey.com>
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


#include "Compiler.h"


namespace of {
namespace Sketch {


Compiler::Compiler(ProcessTaskQueue& taskQueue,
                   const std::string& pathToTemplates,
                   const std::string& openFrameworksDir):
    _taskQueue(taskQueue),
    _pathToTemplates(pathToTemplates),
    _projectFileTemplate(ofBufferFromFile(ofToDataPath(_pathToTemplates + "/main.tmpl")).getText()),
    _classTemplate(ofBufferFromFile(ofToDataPath(_pathToTemplates + "/class.tmpl")).getText()),
    _openFrameworksDir(openFrameworksDir)
{
}


Poco::UUID Compiler::compile(const Project& project)
{
    MakeTask::Settings settings;
    settings.ofRoot = _openFrameworksDir;
    return _taskQueue.start(new MakeTask(settings, project, "Release"));
}


Poco::UUID Compiler::run(const Project& project)
{
    return _taskQueue.start(new RunTask(project, RunTask::RELEASE));
}


void Compiler::generateSourceFiles(const Project& project)
{
    ofDirectory src(project.getPath() + "/src");
    src.remove(true);
    src.create(true);

    Json::Value projectData = project.getData();
    
    Poco::RegularExpression cppExpression(".+\\.cpp$", Poco::RegularExpression::RE_ANCHORED);
    std::string projectFile;
    
    if (cppExpression.match(projectData["projectFile"]["name"].asString()))
    {
        ofLogNotice("Compiler::generateSourceFiles") << "Project file " << projectData["projectFile"]["name"].asString() << " matches .cpp regex" << endl;
        projectFile = projectData["projectFile"]["fileContents"].asString();
    }
    else
    {
        projectFile = _projectFileTemplate;
        ofStringReplace(projectFile, "<projectfile>", projectData["projectFile"]["fileContents"].asString());
        ofStringReplace(projectFile, "<projectname>", projectData["projectFile"]["name"].asString());
        _replaceIncludes(projectFile);
    }
    
    ofBuffer sourceBuffer(projectFile);
    ofBufferToFile(src.getAbsolutePath() + "/main.cpp", sourceBuffer);

    if (project.hasClasses())
    {
        Poco::RegularExpression hExpression(".+\\.h$", Poco::RegularExpression::RE_ANCHORED);
        
        for (unsigned int i = 0; i < projectData["classes"].size(); ++i)
        {
            Json::Value c = projectData["classes"][i];
            std::string classFile;
            
            if (hExpression.match(c["name"].asString()))
            {
                classFile = c["fileContents"].asString();
                ofLogNotice("Compiler::generateSourceFiles") << "Class file " << c["name"] << " matches .h regex" << endl;
            }
            else
            {
                classFile = _classTemplate;
                ofStringReplace(classFile, "<classname>", c["name"].asString());
                ofStringReplace(classFile, "<classfile>", c["fileContents"].asString());
                _replaceIncludes(classFile);
            }
            
            
            ofBuffer sourceBuffer(classFile);
            ofBufferToFile(src.getAbsolutePath() + "/" + c["name"].asString() + ".h", sourceBuffer);
        }
    }
}


Json::Value Compiler::parseError(std::string message) const
{
    // i.e. Mike-Test:8:6: error: cannot initialize a variable of type 'int' with an lvalue of type 'const char [3]'

    Json::Value compileError;

    Poco::RegularExpression errorExpression(".+:[0-9]+:[0-9]+: (fatal error|error|warning|note): .+$", Poco::RegularExpression::RE_ANCHORED);

    if (errorExpression.match(message))
    {
        std::vector<std::string> vals = ofSplitString(message, ":");

        if (vals.size() == 5)
        {
            ofStringReplace(vals[3], " ", "");
            
            // ACE Editor refers to "note" as "info"
            if (vals[3] == "note") vals[3] = "info";
            else if (vals[3] == "fatalerror") vals[3] = "error";
            
            compileError["tabName"] = vals[0];
            compileError["annotation"]["row"] = ofToInt(vals[1]);
            compileError["annotation"]["column"] = ofToInt(vals[2]);
            compileError["annotation"]["type"] = vals[3];
            compileError["annotation"]["text"] = vals[4];
           
        }
    }

    return compileError;
}


void Compiler::_replaceIncludes(std::string& fileContents)
{
    Poco::RegularExpression includesExpression("#include .*\n");
    Poco::RegularExpression::Match match;
    
    std::vector<std::string> includes;
    
    int numMatches = 0;
    std::size_t matchOffset = 0;
    
    while (matchOffset < fileContents.size())
    {
        if (includesExpression.match(fileContents, matchOffset, match) == 0) break;
        std::string include;
        includesExpression.extract(fileContents, match.offset, include);
        includes.push_back(include);
        matchOffset = match.offset + match.length;
        numMatches++;
    }

    includesExpression.subst(fileContents, "", Poco::RegularExpression::RE_GLOBAL);
    ofStringReplace(fileContents, "<includes>", ofJoinString(includes, ""));
    ofStringReplace(fileContents, "<line>", ofToString(includes.size()));
}


void Compiler::_parseAddons()
{
}


void Compiler::_getAddons()
{
}


} } // namespace of::Sketch
