/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: mtsTaskManagerProxyServer.cpp 145 2009-03-18 23:32:40Z mjung5 $

  Author(s):  Min Yang Jung
  Created on: 2009-03-17

  (C) Copyright 2009 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstMultiTask/mtsTaskManagerProxyServer.h>

CMN_IMPLEMENT_SERVICES(mtsTaskManagerProxyServer);

void mtsTaskManagerProxyServer::StartProxy(mtsTaskManager * callingTaskManager)
{
    // Initialize Ice object.
    // Notice that a worker thread is not created right now.
    Init();

    if (InitSuccessFlag) {
        // Create a worker thread here and returns immediately.
        ThreadArgumentsInfo.argument = callingTaskManager;
        ThreadArgumentsInfo.proxy = this;
        ThreadArgumentsInfo.Runner = mtsTaskManagerProxyServer::Runner;

        WorkerThread.Create<ProxyWorker<mtsTaskManager>, ThreadArguments<mtsTaskManager>*>(
            &ProxyWorkerInfo, &ProxyWorker<mtsTaskManager>::Run, &ThreadArgumentsInfo, "C-PRX");
    }
}

void mtsTaskManagerProxyServer::Runner(ThreadArguments<mtsTaskManager> * arguments)
{
    mtsTaskManagerProxyServer * ProxyServer = 
        dynamic_cast<mtsTaskManagerProxyServer*>(arguments->proxy);
    
    try {
        ProxyServer->StartServer();
    } catch (...) {
        ProxyServer->EndServant();
    }
    ProxyServer->EndServant();
    //catch (const Ice::Exception& e) {
    //    ProxyServer->EndServant();
    //    CMN_LOG_CLASS_AUX(ProxyServer, 3) << "Proxy initialization error: " << e << std::endl;
    //} catch (const char * msg) {
    //    ProxyServer->OnThreadEnd();
    //    CMN_LOG_CLASS_AUX(ProxyServer, 3) << "Proxy initialization error: " << msg << std::endl;        
    //}

    ProxyServer->OnThreadEnd();
}

//-----------------------------------------------------------------------------
// From SLICE definition
//-----------------------------------------------------------------------------
mtsTaskManagerProxyServer::TaskManagerServerI::TaskManagerServerI(
    const Ice::CommunicatorPtr& communicator) 
    : _communicator(communicator),
      _destroy(false),
      _TaskManagerServer(new TaskManagerServerThread(this))
{
}

void mtsTaskManagerProxyServer::TaskManagerServerI::Start()
{
    std::cout << "============================ TaskManagerServerI::Start" << std::endl;

    _TaskManagerServer->start();
}

void mtsTaskManagerProxyServer::TaskManagerServerI::Run()
{
    int num = 0;
    while(true)
    {
        std::set<mtsTaskManagerProxy::TaskManagerClientPrx> clients;
        {
            IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);
            timedWait(IceUtil::Time::seconds(2));

            if(_destroy)
            {
                break;
            }

            clients = _clients;
        }

        if(!clients.empty())
        {
            ++num;
            for(std::set<mtsTaskManagerProxy::TaskManagerClientPrx>::iterator p 
                = clients.begin(); p != clients.end(); ++p)
            {
                try
                {
                    (*p)->ReceiveData(num);
                }
                catch(const IceUtil::Exception& ex)
                {
                    std::cerr << "removing client `" << _communicator->identityToString((*p)->ice_getIdentity()) << "':\n"
                        << ex << std::endl;

                    IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);
                    _clients.erase(*p);
                }
            }
        }
    }
}

void mtsTaskManagerProxyServer::TaskManagerServerI::Destroy()
{
    std::cout << "============================ TaskManagerServerI::Destroy" << std::endl;

    IceUtil::ThreadPtr callbackSenderThread;

    {
        IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);

        std::cout << "destroying callback sender" << std::endl;
        _destroy = true;

        notify();

        callbackSenderThread = _TaskManagerServer;
        _TaskManagerServer = 0; // Resolve cyclic dependency.
    }

    callbackSenderThread->getThreadControl().join();
}

void mtsTaskManagerProxyServer::TaskManagerServerI::AddClient(
    const ::Ice::Identity& ident, const ::Ice::Current& current)
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);

    std::cout << "adding client: " << _communicator->identityToString(ident) << "'"<< std::endl;

    mtsTaskManagerProxy::TaskManagerClientPrx client = 
        mtsTaskManagerProxy::TaskManagerClientPrx::uncheckedCast(current.con->createProxy(ident));
    _clients.insert(client);
}

void mtsTaskManagerProxyServer::TaskManagerServerI::SendCurrentTaskInfo(
    const ::Ice::Current&)
{
    std::cout << "============================ TaskManagerServerI::SendCurrentTaskInfo" << std::endl;
}

/*
void mtsTaskManagerProxyServer::TaskManagerChannelI::ShareTaskInfo(
    const ::mtsTaskManagerProxy::TaskInfo& clientTaskInfo,
    ::mtsTaskManagerProxy::TaskInfo& serverTaskInfo, 
    const ::Ice::Current&)
{
    //
    // 4/13 TODO:
    // - Task Name, Interface Name should be received.
    // - How to define the id of TaskManager instance? (need to define the name
    //   of cmnGenericObject?)
    //

    // Get the names of tasks' being managed by the peer TaskManager.
    mtsTaskManagerProxy::TaskNameSeq::const_iterator it = clientTaskInfo.taskNames.begin();
    for (; it != clientTaskInfo.taskNames.end(); ++it) {
        CMN_LOG_CLASS_AUX(mtsTaskManager::GetInstance(), 5) << 
            "CLIENT TASK NAME: " << *it << std::endl;
    }

    // Send my information to the peer ('peers' in the future).
    mtsTaskManager::GetInstance()->GetNamesOfTasks(serverTaskInfo.taskNames);
}
*/