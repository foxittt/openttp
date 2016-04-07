//
//
// The MIT License (MIT)
//
// Copyright (c) 2016  Michael J. Wouters
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
// Modification history
//


#ifndef DEBUG_H
#define DEBUG_H

#ifndef CWDEBUG

#include <iostream>     // std::cerr
#include <cstdlib>      // std::exit, EXIT_FAILURE

#define AllocTag1(p)
#define AllocTag2(p, desc)
#define AllocTag_dynamic_description(p, data)
#define AllocTag(p, data)
#define Debug(STATEMENT...)
#define Dout(cntrl, data)
#define DoutFatal(cntrl, data) LibcwDoutFatal(, , cntrl, data)
#define ForAllDebugChannels(STATEMENT...)
#define ForAllDebugObjects(STATEMENT...)
#define LibcwDebug(dc_namespace, STATEMENT...)
#define LibcwDout(dc_namespace, d, cntrl, data)
#define LibcwDoutFatal(dc_namespace, d, cntrl, data) do { ::std::cerr << data << ::std::endl; ::std::exit(EXIT_FAILURE); } while(1)
#define LibcwdForAllDebugChannels(dc_namespace, STATEMENT...)
#define LibcwdForAllDebugObjects(dc_namespace, STATEMENT...)
#define NEW(x) new x
#define CWDEBUG_ALLOC 0
#define CWDEBUG_MAGIC 0
#define CWDEBUG_LOCATION 0
#define CWDEBUG_LIBBFD 0
#define CWDEBUG_DEBUG 0
#define CWDEBUG_DEBUGOUTPUT 0
#define CWDEBUG_DEBUGM 0
#define CWDEBUG_DEBUGT 0
#define CWDEBUG_MARKER 0

#else // CWDEBUG

// This must be defined before <libcwd/debug.h> is included and must be the
// name of the namespace containing your `dc' (Debug Channels) namespace
// (see below).  You can use any namespace(s) you like, except existing
// namespaces (like ::, ::std and ::libcwd).
#define DEBUGCHANNELS ::lcdmonitor::debug::channels
#include <libcwd/debug.h>

namespace lcdmonitor {
  namespace debug {

    void init(void);		// Initialize debugging code from main().
    void init_thread(void);	// Initialize debugging code from new threads.

    namespace channels {	// This is the DEBUGCHANNELS namespace, see above.
      namespace dc {		// 'dc' is defined inside DEBUGCHANNELS.

			using namespace libcwd::channels::dc;
			using libcwd::channel_ct;

			// Add the declaration of new debug channels here
			// and add their definition in a custom debug.cc file.
				extern channel_ct trace;
				extern channel_ct benchmark;
				extern channel_ct fixme;
      } // namespace dc
    } // namespace DEBUGCHANNELS
  }
}

#endif // CWDEBUG

#endif // DEBUG_H