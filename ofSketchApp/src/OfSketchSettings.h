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


#pragma once


#include <string>
#include <json/json.h>
#include "ofxJSONElement.h"


namespace of {
namespace Sketch {


class OfSketchSettings
{
public:
    OfSketchSettings(const std::string& path);

    bool load();
    bool save();

    const Json::Value& getData() const;

    int getPort() const;
    int getBufferSize() const;
    std::string getProjectDir() const;
    std::string getSketchDir() const;
    std::string getOpenFrameworksDir() const;
    std::string getOpenFrameworksVersion() const;
    std::string getAddonsDir() const;
    std::string getProjectSettingsFilename() const;
    std::string getProjectExtension() const;
    std::string getClassExtension() const;

private:
    std::string _path;
    ofxJSONElement _data; //ofxJSONElement for load functionality

};


} } // namespace of::Sketch
