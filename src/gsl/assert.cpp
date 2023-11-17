///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023 The Dash Core developers
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#include <gsl/assert.h>
#include <logging.h>

#include <sstream>
namespace gsl
{
namespace details
{
[[noreturn]] void terminate(nostd::source_location loc) noexcept
{
#if defined(GSL_MSVC_USE_STL_NOEXCEPTION_WORKAROUND)
    (*gsl::details::get_terminate_handler())();
#else
    std::ostringstream s;
    s << "ERROR: error detected null not_null detected at " << loc.file_name() << ":" << loc.line() << ":"
                                                            << loc.column() << ":" << loc.function_name()
                                                            << std::endl;
    std::cerr << s.str() << std::flush;
    LogPrintf("%s", s.str()); /* Continued */
    std::terminate();
#endif // defined(GSL_MSVC_USE_STL_NOEXCEPTION_WORKAROUND)
}

} // namespace details
} // namespace gsl

