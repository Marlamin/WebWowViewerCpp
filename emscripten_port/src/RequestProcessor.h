//
// Created by deamon on 06.09.17.
//

#ifndef WEBWOWVIEWERCPP_REQUESTPROCESSOR_H
#define WEBWOWVIEWERCPP_REQUESTPROCESSOR_H

#include "../../wowViewerLib/src/include/sharedFile.h"
#include "../../wowViewerLib/src/include/iostuff.h"
#include <thread>
#include <list>
#include <vector>
#include <forward_list>
#include <unordered_set>

class RequestProcessor : public IFileRequest {
protected:
    RequestProcessor() {

    }

protected:
    IFileRequester *m_fileRequester = nullptr;

    virtual void processFileRequest(std::string &fileName, CacheHolderType holderType) = 0;
public:
    void setFileRequester(IFileRequester *fileRequester) {
        m_fileRequester = fileRequester;
    }

private:
    class RequestStruct {
    public:
        std::string fileName;
        CacheHolderType holderType;
    };

    class ResultStruct {
    public:
        ResultStruct(){};
        ResultStruct (const ResultStruct &old_obj) {
            fileName = old_obj.fileName;
            holderType = old_obj.holderType;
            buffer = old_obj.buffer;
        }
        std::string fileName;
        CacheHolderType holderType;
        HFileContent buffer = nullptr;
    };

    std::thread *loaderThread;

    std::list<RequestStruct> m_requestQueue;
    std::list<ResultStruct> m_resultQueue;
    std::unordered_set<std::string> currentlyProcessingFnames;

    bool m_threaded = false;

public:
    void processResults(int limit);
    void processRequests(bool calledFromThread);

    void setThreaded(bool value) {
        m_threaded = value;
        if (value) {
            loaderThread = new std::thread(([&](){
                this->processRequests(true);
            }));
        }
    }
    int currentlyProcessing = 0;
    void provideResult(std::string &fileName, HFileContent content, CacheHolderType holderType);
protected:
    void addRequest (std::string &fileName, CacheHolderType holderType);


};

#endif //WEBWOWVIEWERCPP_REQUESTPROCESSOR_H
