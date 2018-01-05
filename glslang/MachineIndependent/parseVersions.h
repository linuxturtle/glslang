//
// Copyright (C) 2016 Google, Inc.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//    Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

// This is implemented in Versions.cpp

#ifndef _PARSE_VERSIONS_INCLUDED_
#define _PARSE_VERSIONS_INCLUDED_

#include "../Public/ShaderLang.h"
#include "../Include/InfoSink.h"
#include "Scan.h"

#include <map>
#include <cstdint>

namespace glslang {

//
// Base class for parse helpers.
// This just has version-related information and checking.
// This class should be sufficient for preprocessing.
//
class TParseVersions {
public:
    TParseVersions(TIntermediate& interm, int version, EProfile profile,
                   const SpvVersion& spvVersion, EShLanguage language, TInfoSink& infoSink,
                   bool forwardCompatible, EShMessages messages)
        : infoSink(infoSink), version(version), profile(profile), language(language),
          spvVersion(spvVersion), forwardCompatible(forwardCompatible),
          intermediate(interm), messages(messages), numErrors(0), currentScanner(0) { }
    virtual ~TParseVersions() { }
    virtual void initializeExtensionBehavior();
    virtual void requireProfile(const TSourceLoc&, int queryProfiles, const char* featureDesc);
    virtual void profileRequires(const TSourceLoc&, int queryProfiles, int minVersion, int numExtensions, const char* const extensions[], const char* featureDesc);
    virtual void profileRequires(const TSourceLoc&, int queryProfiles, int minVersion, const char* const extension, const char* featureDesc);
    virtual void requireStage(const TSourceLoc&, EShLanguageMask, const char* featureDesc);
    virtual void requireStage(const TSourceLoc&, EShLanguage, const char* featureDesc);
    virtual void checkDeprecated(const TSourceLoc&, int queryProfiles, int depVersion, const char* featureDesc);
    virtual void requireNotRemoved(const TSourceLoc&, int queryProfiles, int removedVersion, const char* featureDesc);
    virtual void unimplemented(const TSourceLoc&, const char* featureDesc);
    virtual void requireExtensions(const TSourceLoc&, int numExtensions, const char* const extensions[], const char* featureDesc);
    virtual void ppRequireExtensions(const TSourceLoc&, int numExtensions, const char* const extensions[], const char* featureDesc);
    virtual TExtensionBehavior getExtensionBehavior(const char*);
    virtual bool extensionTurnedOn(const char* const extension);
    virtual bool extensionsTurnedOn(int numExtensions, const char* const extensions[]);
    virtual void updateExtensionBehavior(int line, const char* const extension, const char* behavior);
    virtual void updateExtensionBehaviorRaw(int line, const char* const extension, const char* behavior);
    virtual void fullIntegerCheck(const TSourceLoc&, const char* op);
    virtual void doubleCheck(const TSourceLoc&, const char* op);
#ifdef AMD_EXTENSIONS
    virtual void int16Check(const TSourceLoc& loc, const char* op, bool builtIn = false);
    virtual void float16Check(const TSourceLoc&, const char* op, bool builtIn = false);
#endif
    virtual void int64Check(const TSourceLoc&, const char* op, bool builtIn = false);
    virtual void spvRemoved(const TSourceLoc&, const char* op);
    virtual void vulkanRemoved(const TSourceLoc&, const char* op);
    virtual void requireVulkan(const TSourceLoc&, const char* op);
    virtual void requireSpv(const TSourceLoc&, const char* op);
    virtual bool checkExtensionsRequested(const TSourceLoc&, int numExtensions, const char* const extensions[], const char* featureDesc);
    virtual void updateExtensionBehavior(const char* const extension, TExtensionBehavior);

    virtual void C_DECL error(const TSourceLoc&, const char* szReason, const char* szToken,
        const char* szExtraInfoFormat, ...) = 0;
    virtual void C_DECL  warn(const TSourceLoc&, const char* szReason, const char* szToken,
        const char* szExtraInfoFormat, ...) = 0;
    virtual void C_DECL ppError(const TSourceLoc&, const char* szReason, const char* szToken,
        const char* szExtraInfoFormat, ...) = 0;
    virtual void C_DECL ppWarn(const TSourceLoc&, const char* szReason, const char* szToken,
        const char* szExtraInfoFormat, ...) = 0;

    void addError() { ++numErrors; }
    int getNumErrors() const { return numErrors; }

    void setScanner(TInputScanner* scanner) { currentScanner = scanner; }
    TInputScanner* getScanner() const { return currentScanner; }
    const TSourceLoc& getCurrentLoc() const { return currentScanner->getSourceLoc(); }
    void setCurrentLine(int line) { currentScanner->setLine(line); }
    void setCurrentColumn(int col) { currentScanner->setColumn(col); }
    void setCurrentSourceName(const char* name) { currentScanner->setFile(name); }
    void setCurrentString(int string) { currentScanner->setString(string); }

    void getPreamble(std::string&);
    bool relaxedErrors()    const { return (messages & EShMsgRelaxedErrors) != 0; }
    bool suppressWarnings() const { return (messages & EShMsgSuppressWarnings) != 0; }
    bool isReadingHLSL()    const { return (messages & EShMsgReadHlsl) == EShMsgReadHlsl; }

    TInfoSink& infoSink;

    // compilation mode
    int version;                 // version, updated by #version in the shader
    EProfile profile;            // the declared profile in the shader (core by default)
    EShLanguage language;        // really the stage
    SpvVersion spvVersion;
    bool forwardCompatible;      // true if errors are to be given for use of deprecated features
    TIntermediate& intermediate; // helper for making and hooking up pieces of the parse tree

protected:
    // Track for each extension string, what its current behavior is set to.
    // Because keyword scanning needs to check this, it needs to run at high
    // performance, and hence the frequently checked strings need to be the
    // canonical ones from Versions.h.
    // Only a raw string from shader source should need a slow operation to
    // figure out what extension it is. Otherwise, the mapping is integer based,
    // not string based.
    class tExtensionBehaviorMap {
    public:
        // Translate from an unknown char* string to one of the canonical ones
        // from Versions.h. Keep the original raw string if no mapping is found.
        const char* mapRaw(const char* extensionString)
        {
            // This is expected to happen infrequenly; only when coming
            // from shader source. Otherwise, the code should start with
            // the canonical one from Versions.h.
            for (auto iter = map.begin(); iter != map.end(); ++iter) {
                // translate from integer back to canonical string to find a match
                const char* canonical = reinterpret_cast<const char*>(iter->first);
                if (strcmp(canonical, extensionString) == 0)
                    return canonical;
            }
            return extensionString;
        }
        // Use only canonical strings from Versions.h.
        // Returns writable l-value so the mapping can be set by the caller.
        TExtensionBehavior& operator[](const char* extensionString)
        {
            // translate from canonical string to integer, then
            // map to resulting behavior
            return map[reinterpret_cast<uintptr_t>(extensionString)];
        }
        // Use only canonical strings from Versions.h.
        TExtensionBehavior find(const char* extensionString)
        {
            // translate from canonical string to integer
            auto iter = map.find(reinterpret_cast<uintptr_t>(extensionString));
            if (iter == map.end())
                return EBhMissing;
            else
                return iter->second;
        }
        // Set all current extensions' behavior to 'behavior'
        void setAll(TExtensionBehavior behavior)
        {
            for (auto iter = map.begin(); iter != map.end(); ++iter)
                iter->second = behavior;
        }
    protected:
        // Use an integer-based mapped, no string manipulation.
        TMap<uintptr_t, TExtensionBehavior> map;
    };
    tExtensionBehaviorMap extensionBehavior; // the actual map object to use for the above

    EShMessages messages;        // errors/warnings/rule-sets
    int numErrors;               // number of compile-time errors encountered
    TInputScanner* currentScanner;

private:
    explicit TParseVersions(const TParseVersions&);
    TParseVersions& operator=(const TParseVersions&);
};

} // end namespace glslang

#endif // _PARSE_VERSIONS_INCLUDED_
