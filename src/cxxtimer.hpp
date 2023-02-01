/*

MIT License

Copyright (c) 2017 André L. Maravilha

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef CXX_TIMER_HPP
#define CXX_TIMER_HPP

#include <chrono>


namespace cxxtimer {

/**
 * This class works as a stopwatch.
 */
class Timer {

public:

    /**
     * Constructor.
     *
     * @param   start
     *          If true, the timer is started just after construction.
     *          Otherwise, it will not be automatically started.
     */
    Timer(bool start = false);

    /**
     * Copy constructor.
     *
     * @param   other
     *          The object to be copied.
     */
    Timer(const Timer& other) = default;

    /**
     * Transfer constructor.
     *
     * @param   other
     *          The object to be transferred.
     */
    Timer(Timer&& other) = default;

    /**
     * Destructor.
     */
    virtual ~Timer() = default;

    /**
     * Assignment operator by copy.
     *
     * @param   other
     *          The object to be copied.
     *
     * @return  A reference to this object.
     */
    Timer& operator=(const Timer& other) = default;

    /**
     * Assignment operator by transfer.
     *
     * @param   other
     *          The object to be transferred.
     *
     * @return  A reference to this object.
     */
    Timer& operator=(Timer&& other) = default;

    /**
     * Start/resume the timer.
     */
    void start();

    /**
     * Stop/pause the timer.
     */
    void stop();

    /**
     * Reset the timer.
     */
    void reset();

    /**
     * Return the elapsed time.
     *
     *
     * @return  The elapsed time.
     */
    template <class duration_t = std::chrono::milliseconds>
    typename duration_t::rep count() const;

private:

    bool started_{false};
    bool paused_{false};
    std::chrono::steady_clock::time_point reference_;
    std::chrono::duration<long double> accumulated_;
};

}


inline cxxtimer::Timer::Timer(bool start) :
        reference_(std::chrono::steady_clock::now()),
        accumulated_(std::chrono::duration<long double>(0)) {
    if (start) {
        this->start();
    }
}

inline void cxxtimer::Timer::start() {
    if (!started_) {
        started_ = true;
        paused_ = false;
        accumulated_ = std::chrono::duration<long double>(0);
        reference_ = std::chrono::steady_clock::now();
    } else if (paused_) {
        reference_ = std::chrono::steady_clock::now();
        paused_ = false;
    }
}

inline void cxxtimer::Timer::stop() {
    if (started_ && !paused_) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        accumulated_ = accumulated_ + std::chrono::duration_cast< std::chrono::duration<long double> >(now - reference_);
        paused_ = true;
    }
}

inline void cxxtimer::Timer::reset() {
    if (started_) {
        started_ = false;
        paused_ = false;
        reference_ = std::chrono::steady_clock::now();
        accumulated_ = std::chrono::duration<long double>(0);
    }
}

template <class duration_t>
typename duration_t::rep cxxtimer::Timer::count() const {
    if (started_) {
        if (paused_) {
            return std::chrono::duration_cast<duration_t>(accumulated_).count();
        } else {
            return std::chrono::duration_cast<duration_t>(
                    accumulated_ + (std::chrono::steady_clock::now() - reference_)).count();
        }
    } else {
        return duration_t(0).count();
    }
}


#endif