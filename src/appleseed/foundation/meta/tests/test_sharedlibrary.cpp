
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2014-2015 Esteban Tovagliari, The appleseedhq Organization
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

// appleseed.foundation headers.
#include "foundation/platform/sharedlibrary.h"
#include "foundation/utility/test.h"

using namespace foundation;

TEST_SUITE(Foundation_Platform_SharedLibrary)
{
#ifdef _WIN32

    // Not sure how to test this...

#else

    TEST_CASE(LoadSystemLibAndGetSymbol)
    {
        SharedLibrary sh_lib("libdl.so");
        void* symbol = sh_lib.get_symbol("dlopen", false);

        EXPECT_NEQ(0, symbol);
    }

    TEST_CASE(CannotLoadSharedLibrary)
    {
        EXPECT_EXCEPTION(
            ExceptionCannotLoadSharedLib,
            SharedLibrary("libdlXX.so"));
    }

    TEST_CASE(CannotGetSymbol_ThrowExceptionIsTrue)
    {
        SharedLibrary sh_lib("libdl.so");

        EXPECT_EXCEPTION(
            ExceptionSharedLibCannotGetSymbol,
            sh_lib.get_symbol("XdlopenXX", false));
    }

    TEST_CASE(CannotGetSymbol_ThrowExceptionIsFalse)
    {
        SharedLibrary sh_lib("libdl.so");
        void* symbol = sh_lib.get_symbol("XdlopenXX");

        EXPECT_EQ(0, symbol);
    }

#endif
}
