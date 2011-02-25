/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Anton Deguet
  Created on: 2005-08-14

  (C) Copyright 2005-2007 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

%module cisstNumericalPython

%include <std_string.i>
%include <std_except.i>

%import "cisstCommon/cisstCommon.i"
%import "cisstVector/cisstVector.i"

%header %{
#include <cisstNumerical/nmrPython.h>
%}

// Generate parameter documentation for IRE
%feature("autodoc", "1");

%include "cisstNumerical/nmrNetlib.h"

#if CISST_HAS_CISSTNETLIB
%include "cisstNumerical/nmrSVD.i"
#endif
