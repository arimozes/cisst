/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */
/* $Id: main.cpp,v 1.4 2008/09/24 19:46:56 anton Exp $ */

#include <cisstVector.h>
#include <cisstOSAbstraction.h>
#include <cisstDevices.h>

#include "displayTask.h"
#include "displayUI.h"

using namespace std;

int main(void)
{
    // log configuration
    cmnLogger::SetLoD(10);
    cmnLogger::GetMultiplexer()->AddChannel(cout, 10);
    // add a log per thread
    osaThreadedLogFile threadedLog("example1-");
    cmnLogger::GetMultiplexer()->AddChannel(threadedLog, 10);
    // specify a higher, more verbose log level for these classes
    cmnClassRegister::SetLoD("mtsTaskInterface", 10);
    cmnClassRegister::SetLoD("mtsTaskManager", 10);
    cmnClassRegister::SetLoD("devSensableHD", 10);

    // create our two tasks
    const long PeriodDisplay = 10; // in milliseconds
    mtsTaskManager * taskManager = mtsTaskManager::GetInstance();
    displayTask * displayTaskObject =
        new displayTask("DISP", PeriodDisplay * cmn_ms);
    displayTaskObject->Configure();

	devSensableHD * robotObject = new devSensableHD("Omni");
    // add the tasks to the task manager

    taskManager->AddTask(displayTaskObject);
	taskManager->AddTask(robotObject);
    // connect the tasks
    taskManager->Connect("DISP", "Robot", "Omni", "DefaultArm");
    // generate a nice tasks diagram
    std::ofstream dotFile("example1.dot"); 
    taskManager->ToStreamDot(dotFile);
    dotFile.close();

    // create the tasks, i.e. find the commands
    taskManager->CreateAll();
    // start the periodic Run
    taskManager->StartAll();

    // wait until the close button of the UI is pressed
    while (1) {
        if (displayTaskObject->GetExitFlag()) {
            break;
        }
    }
    // cleanup
    taskManager->KillAll();

    osaSleep(PeriodDisplay * 2);
    while (!displayTaskObject->IsTerminated()) osaSleep(PeriodDisplay);

    return 0;
}

/*
  Author(s):  Ankur Kapoor, Peter Kazanzides, Anton Deguet
  Created on: 2004-04-30

  (C) Copyright 2004-2008 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/
