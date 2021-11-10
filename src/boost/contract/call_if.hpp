
#ifndef BOOST_CONTRACT_CALL_IF_HPP_
#define BOOST_CONTRACT_CALL_IF_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Statically disable compilation and execution of functor calls.

@note   These facilities allow to emulate C++17 <c>if constexpr</c> statements
        when used together with functor templates (and C++14 generic lambdas).
        Therefore, they are not useful on C++17 compilers where
        <c> if constexpr</c> can be directly used instead.
*/

#include <boost/contract/detail/none.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/config.hpp>

/* PRIVATE */

/** @cond */

// Boost.ResultOf not always able to deduce lambda result type (on MSVC).
#ifndef BOOST_NO_CXX11_DECLTYPE
    #include <boost/utility/declval.hpp>
    #define BOOST_CONTRACT_CALL_IF_RESULT_OF_(F) \
        decltype(boost::declval<F>()())
#else
    #include <boost/utility/result_of.hpp>
    #define BOOST_CONTRACT_CALL_IF_RESULT_OF_(F) \
        typename boost::result_of<F()>::type
#endif

/** @endcond */

/* CODE */

namespace boost { namespace contract {

/**
Select compilation and execution of functor template calls using a static
boolean predicate (not needed on C++17 compilers, use <c>if constexpr</c>
instead).

This class template has no members because it is never used directly, it is only
used via its specializations.
Usually this class template is instantiated only via the return value of
@RefFunc{boost::contract::call_if} and @RefFunc{boost::contract::call_if_c}.

@see    @RefSect{extras.assertion_requirements__templates_,
        Assertion Requirements}

@tparam Pred    Static boolean predicate that selects which functor template
                call to compile and execute.
@tparam Then Type of the functor template to call if the static predicate
        @c Pred is @c true.
@tparam ThenResult Return type of then-branch functor template call (this is
        usually automatically deduced by this library so it is never explicitly
        specified by the user, and that is why it is often marked as
        @c internal_type in this documentation).
*/
template<bool Pred, typename Then, typename ThenResult =
    #ifndef BOOST_CONTRACT_DETAIL_DOXYGEN
        boost::contract::detail::none
    #else
        internal_type
    #endif
>
struct call_if_statement {}; // Empty so cannot be used (but copyable).

/**
Template specialization to dispatch between then-branch functor template calls
that return void and the ones that return non-void (not needed on C++17
compilers, use <c>if constexpr</c> instead).


The base class is a call-if statement so the else and else-if statements can be
specified if needed.
Usually this class template is instantiated only via the return value of
@RefFunc{boost::contract::call_if} and @RefFunc{boost::contract::call_if_c}.

@note   The <c>result_of<Then()>::type</c> expression needs be evaluated only
        when the static predicate is already checked to be @c true (because
        @c Then() is required to compile only in that case).
        Thus, this template specialization introduces an extra level of
        indirection necessary for proper lazy evaluation of this result-of
        expression.

@see    @RefSect{extras.assertion_requirements__templates_,
        Assertion Requirements}

@tparam Then Type of functor template to call when the static predicate is
        @c true (as it is for this template specialization).
*/
template<typename Then>
struct call_if_statement<true, Then,
    #ifndef BOOST_CONTRACT_DETAIL_DOXYGEN
        boost::contract::detail::none
    #else
        internal_type
    #endif
> :
    call_if_statement<true, Then,
        #ifndef BOOST_CONTRACT_DETAIL_DOXYGEN
            BOOST_CONTRACT_CALL_IF_RESULT_OF_(Then)
        #else
            typename result_of<Then()>::type
        #endif
    >
{ // Copyable (as its base).
    /**
    Construct this object with the then-branch functor template.

    @param f    Then-branch nullary functor template.
                The functor template call @c f() is compiled and called for this
                template specialization (because the if-statement static
                predicate is @c true).
                The return type of @c f() must be the same as (or implicitly
                convertible to) the return type of all other functor template
                calls specified for this call-if object.
    */
    explicit call_if_statement(Then f) : call_if_statement<true, Then,
            BOOST_CONTRACT_CALL_IF_RESULT_OF_(Then)>(f) {}
};

/**
Template specialization to handle static predicates that are @c true for
then-branch functor template calls that do not return void (not needed on C++17
compilers, use <c>if constexpr</c> instead).


Usually this class template is instantiated only via the return value of
@RefFunc{boost::contract::call_if} and @RefFunc{boost::contract::call_if_c}.

@see    @RefSect{extras.assertion_requirements__templates_,
        Assertion Requirements}

@tparam Then Type of functor template to call when the static predicate is
        @c true (as it is for this template specialization).
@tparam ThenResult Non-void return type of the then-branch functor template
        call.
*/
template<typename Then, typename ThenResult>
struct call_if_statement<true, Then, ThenResult> { // Copyable (as *).
    /**
    Construct this object with the then-branch functor template.

    @param f    Then-branch nullary functor template.
                The functor template call @c f() is actually compiled and
                executed for this template specialization (because the
                if-statement static predicate is @c true).
                The return type of @c f() must be the same as (or implicitly
                convertible to) the @p ThenResult type.
    */
    explicit call_if_statement(Then f) :
            r_(boost::make_shared<ThenResult>(f())) {}

    /**
    This implicit type conversion returns a copy of the value returned by the
    call to the then-branch functor template.
    */
    operator ThenResult() const { return *r_; }

    /**
    Specify the else-branch functor template.

    @param f    Else-branch nullary functor template.
                The functor template call @c f() is never compiled or executed
                for this template specialization (because the if-statement
                static predicate is @c true).
                The return type of @c f() must be the same as (or implicitly
                convertible to) the @p ThenResult type.
    
    @return A copy of the value returned by the call to the then-branch functor
            template (because the else-branch functor template call is not
            executed).
    */
    template<typename Else>
    ThenResult else_(Else const&
        #ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
            f
        #endif
    ) const { return *r_; }
    
    /**
    Specify an else-if-branch functor template (using a static boolean
    predicate).

    @param f    Else-if-branch nullary functor template.
                The functor template call @c f() is never compiled or executed
                for this template specialization (because the if-statement
                static predicate is @c true).
                The return type of @c f() must be the same as (or implicitly
                convertible to) the @p ThenResult type.
    
    @tparam ElseIfPred  Static boolean predicate selecting which functor
                        template call to compile and execute.
    
    @return A call-if statement so the else statement and additional else-if
            statements can be specified if needed.
            Eventually, it will be the return value of the then-branch functor
            template call for this template specialization (because the
            if-statement static predicate is @c true).
    */
    template<bool ElseIfPred, typename ElseIfThen>
    call_if_statement<true, Then, ThenResult> else_if_c(
        ElseIfThen const&
        #ifdef BOOST_CONTRACT_DETAIL_DOXYGEN // Avoid unused param warning.
            f
        #endif
    ) const { return *this; }

    /**
    Specify an else-if-branch functor template (using a nullary boolean
    meta-function).

    @param f    Else-if-branch nullary functor template.
                The functor template call @c f() is never compiled or executed
                for this template specialization (because the if-statement
                static predicate is @c true).
                The return type of @c f() must be the same as (or implicitly
                convertible to) the @p ThenResult type.
    
    @tparam ElseIfPred  Nullary boolean meta-function selecting which functor
                        template call to compile and execute.
    
    @return A call-if statement so the else statement and additional else-if
            statements can be specified if needed.
            Eventually, it will be the return value of the then-branch functor
            template call for this template specialization (because the
            if-statement static predicate is @c true).
    */
    template<class ElseIfPred, typename ElseIfThen>
    call_if_statement<true, Then, ThenResult> else_if(
        ElseIfThen const&
        #ifdef BOOST_CONTRACT_DETAIL_DOXYGEN // Avoid unused param warning.
            f
        #endif
    ) const { return *this; }
    
private:
    boost::shared_ptr<ThenResult> r_;
};

/**
Template specialization to handle static predicates that are @c true for
then-branch functor template calls that return void (not needed on C++17
compilers, use <c>if constexpr</c> instead).


Usually this class template is instantiated only via the return value of
@RefFunc{boost::contract::call_if} and @RefFunc{boost::contract::call_if_c}.

@see    @RefSect{extras.assertion_requirements__templates_,
        Assertion Requirements}

@tparam Then Type of functor template to call when the static predicate if
        @c true (as it is for this template specialization).
*/
template<typename Then>
struct call_if_statement<true, Then, void> { // Copyable (no data).
    /**
    Construct this object with the then-branch functor template.

    @param f    Then-branch nullary functor template.
                The functor template call @c f() is actually compiled and
                executed for this template specialization (because the
                if-statement static predicate is @c true).
                The return type of @c f() must be @c void for this template
                specialization (because the then-branch functor template calls
                return void).
    */
    explicit call_if_statement(Then f) { f(); }
    
    // Cannot provide `operator ThenResult()` here, because ThenResult is void.

    /**
    Specify the else-branch functor template.

    @param f    Else-branch nullary functor template.
                The functor template call @c f() is never compiled or executed
                for this template specialization (because the if-statement
                static predicate is @c true).
                The return type of @c f() must be @c void for this template
                specialization (because the then-branch functor template calls
                return void).
    */
    template<typename Else>
    void else_(Else const&
        #ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
            f
        #endif
    ) const {}
    
    /**
    Specify an else-if-branch functor template (using a static boolean
    predicate).

    @param f    Else-if-branch nullary functor template.
                The functor template call @c f() is never compiled or executed
                for this template specialization (because the if-statement
                static predicate is @c true).
                The return type of @c f() must be @c void for this template
                specialization (because the then-branch functor template calls
                return void).
    
    @tparam ElseIfPred  Static boolean predicate selecting which functor
                        template call to compile and execute.
    
    @return A call-if statement so the else statement and additional else-if
            statements can be specified if needed.
            Eventually, it will return void for this template specialization
            (because the then-branch functor template calls return void).
    */
    template<bool ElseIfPred, typename ElseIfThen>
    call_if_statement<true, Then, void> else_if_c(
        ElseIfThen const&
        #ifdef BOOST_CONTRACT_DETAIL_DOXYGEN // Avoid unused param warning.
            f
        #endif
    ) const { return *this; }

    /**
    Specify an else-if-branch functor template (using a nullary boolean
    meta-function).

    @param f    Else-if-branch nullary functor template.
                The functor template call @c f() is never compiled or executed
                for this template specialization (because the if-statement
                static predicate is @c true).
                The return type of @c f() must be @c void for this template
                specialization (because the then-branch functor template calls
                return void).

    @tparam ElseIfPred  Nullary boolean meta-function selecting which functor
                        template call to compile and execute.

    @return A call-if statement so the else statement and additional else-if
            statements can be specified if needed.
            Eventually, it will return void for this template specialization
            (because the then-branch functor template calls return void).
    */
    template<class ElseIfPred, typename ElseIfThen>
    call_if_statement<true, Then, void> else_if(
        ElseIfThen const&
        #ifdef BOOST_CONTRACT_DETAIL_DOXYGEN // Avoid unused param warning.
            f
        #endif
    ) const { return *this; }
};

/**
Template specialization to handle static predicates that are @c false (not
needed on C++17 compilers, use <c>if constexpr</c> instead).

This template specialization handles all else-branch functor template calls
(whether they return void or not).
Usually this class template is instantiated only via the return value of
@RefFunc{boost::contract::call_if} and @RefFunc{boost::contract::call_if_c}.

@see    @RefSect{extras.assertion_requirements__templates_,
        Assertion Requirements}

@tparam Then Type of functor template to call when the static predicate is
        @c true (never the case for this template specialization).
*/
template<typename Then> // Copyable (no data).
struct call_if_statement<false, Then,
    #ifndef BOOST_CONTRACT_DETAIL_DOXYGEN
        boost::contract::detail::none
    #else
        internal_type
    #endif
> {
    /**
    Construct this object with the then-branch functor template.

    @param f    Then-branch nullary functor template.
                The functor template call @c f() is never compiled or executed
                for this template specialization (because the if-statement
                static predicate is @c false).
                The return type of @c f() must be the same as (or implicitly
                convertible to) the return type of all other functor template
                calls specified for this call-if object.
    */
    explicit call_if_statement(Then const&
        #ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
            f
        #endif
    ) {}

    // Do not provide `operator result_type()` here, require else_ instead.

    /**
    Specify the else-branch functor template.

    @note   The <c>result_of<Else()>::type</c> expression needs be evaluated
            only when the static predicate is already checked to be @c false
            (because @c Else() is required to compile only in that case).
            Thus, this result-of expression is evaluated lazily and only in
            instantiations of this template specialization.
    
    @param f    Else-branch nullary functor template.
                The functor template call @c f() is actually compiled and
                executed for this template specialization (because the
                if-statement static predicate is @c false).
                The return type of @c f() must be the same as (or implicitly
                convertible to) the return type of all other functor template
                calls specified for this call-if object.
    
    @return A copy of the value returned by the call to the else-branch functor
            template @c f().
    */
    template<typename Else>
    #ifndef BOOST_CONTRACT_DETAIL_DOXYGEN
        BOOST_CONTRACT_CALL_IF_RESULT_OF_(Else)
    #else
        typename result_of<Else()>::type
    #endif
    else_(Else f) const { return f(); }
    
    /**
    Specify an else-if-branch functor template (using a static boolean
    predicate).

    @param f    Else-if-branch nullary functor template.
                The functor template call @c f() is actually compiled and
                executed if and only if @c ElseIfPred is @c true (because the
                if-statement static predicate is already @c false for this
                template specialization).
                The return type of @c f() must be the same as (or implicitly
                convertible to) the return type of all other functor template
                calls specified for this call-if object.
    
    @tparam ElseIfPred  Static boolean predicate selecting which functor
                        template call to compile and execute.

    @return A call-if statement so the else statement and additional else-if
            statements can be specified if needed.
            Eventually, this will be the return value of the one functor
            template call being compiled and executed.
    */
    template<bool ElseIfPred, typename ElseIfThen>
    call_if_statement<ElseIfPred, ElseIfThen> else_if_c(ElseIfThen f) const {
        return call_if_statement<ElseIfPred, ElseIfThen>(f);
    }
    
    /**
    Specify an else-if-branch functor template (using a nullary boolen
    meta-function).

    @param f    Else-if-branch nullary functor template.
                The functor template call @c f() is actually compiled and
                executed if and only if @c ElseIfPred::value is @c true (because
                the if-statement static predicate is already @c false for this
                template specialization).
                The return type of @c f() must be the same as (or implicitly
                convertible to) the return type of all other functor template
                calls specified for this call-if object.

    @tparam ElseIfPred  Nullary boolean meta-function selecting which functor
                        template call to compile and execute.

    @return A call-if statement so the else statement and additional else-if
            statements can be specified if needed.
            Eventually, this will be the return value of the one functor
            template call being compiled and executed.
    */
    template<class ElseIfPred, typename ElseIfThen>
    call_if_statement<ElseIfPred::value, ElseIfThen> else_if(ElseIfThen f)
            const {
        return call_if_statement<ElseIfPred::value, ElseIfThen>(f);
    }
};

/**
Select compilation and execution of functor template calls using a static
boolean predicate (not needed on C++17 compilers, use <c>if constexpr</c>
instead).

Create a call-if object with the specified then-branch functor template:

@code
boost::contract::call_if_c<Pred1>(
    then_functor_template1
).template else_if_c<Pred2>(            // Optional.
    then_functor_template2
)                                       // Optionally, other `else_if_c` or
...                                     // `else_if`.
.else_(                                 // Optional for `void` functors,
    else_functor_template               // but required for non `void`.
)
@endcode

Optional functor templates for else-if-branches and the else-branch can be
specified as needed (the else-branch function template is required if @c f
returns non-void).

@see    @RefSect{extras.assertion_requirements__templates_,
        Assertion Requirements}

@param f    Then-branch nullary functor template.
            The functor template call @c f() is compiled and executed if and
            only if @c Pred is @c true.
            The return type of other functor template calls specified for this
            call-if statement (else-branch, else-if-branches, etc.) must be the
            same as (or implicitly convertible to) the return type of
            then-branch functor call @c f().

@tparam Pred    Static boolean predicate selecting which functor template call
                to compile and execute.

@return A call-if statement so else and else-if statements can be specified if
        needed.
        Eventually, this will be the return value of the one functor template
        call being compiled and executed (which could also be @c void).
*/
template<bool Pred, typename Then>
call_if_statement<Pred, Then> call_if_c(Then f) {
    return call_if_statement<Pred, Then>(f);
}

/**
Select compilation and execution of functor template calls using a nullary
boolean meta-function (not needed on C++17 compilers, use <c>if constexpr</c>
instead).

This is equivalent to <c>boost::contract::call_if_c<Pred::value>(f)</c>.
Create a call-if object with the specified then-branch functor template:

@code
boost::contract::call_if<Pred1>(
    then_functor_template1
).template else_if<Pred2>(              // Optional.
    then_functor_template2
)                                       // Optionally, other `else_if` or
...                                     // `else_if_c`.
.else_(                                 // Optional for `void` functors,
    else_functor_template               // but required for non `void`.
)
@endcode

Optional functor templates for else-if-branches and the else-branch can be
specified as needed (the else-branch functor template is required if @c f
returns non-void).

@see    @RefSect{extras.assertion_requirements__templates_,
        Assertion Requirements}

@param f    Then-branch nullary functor template.
            The functor template call @c f() is compiled and executed if and
            only if @c Pred::value is @c true.
            The return type of other functor template calls specified for this
            call-if statement (else-branch, else-if-branches, etc.) must be the
            same as (or implicitly convertible to) the return type of
            then-branch functor template call @c f().

@tparam Pred    Nullary boolean meta-function selecting which functor template
                call to compile and execute.

@return A call-if statement so else and else-if statements can be specified if
        needed.
        Eventually, this will be the return value of the one functor template
        call being compiled and executed (which could also be @c void).
*/
template<class Pred, typename Then>
call_if_statement<Pred::value, Then> call_if(Then f) {
    return call_if_statement<Pred::value, Then>(f);
}

/**
Select compilation and execution of a boolean functor template condition using a
static boolean predicate (not needed on C++17 compilers, use
<c>if constexpr</c> instead).

Compile and execute the nullary boolean functor template call @c f() if and only
if the specified static boolean predicate @p Pred is @c true, otherwise
trivially return @p else_ (@c true by default) at run-time.

A call to <c>boost::contract::condition_if_c<Pred>(f, else_)</c> is logically
equivalent to <c>boost::contract::call_if_c<Pred>(f, [] { return else_; })</c>
(but its internal implementation is optimized and it does not actually use
@c call_if_c).

@see    @RefSect{extras.assertion_requirements__templates_,
        Assertion Requirements}

@param f    Nullary boolean functor template.
            The functor template call @c f() is compiled and executed if and
            only if @c Pred is @c true.

@tparam Pred    Static boolean predicate selecting when the functor template
                call @c f() should be compiled and executed.
@param else_    Boolean value to return when @c Pred is @c false (instead of
                compiling and executing the functor template call @c f()).

@return Boolean value returned by @c f() if the static predicate @c Pred is
        @c true. Otherwise, trivially return @p else_.
*/
#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    template<bool Pred, typename Then>
    bool condition_if_c(Then f, bool else_ = true);
#else
    // NOTE: condition_if is a very simple special case of call_if so it can be
    // trivially implemented using enable_if instead of call_if as done below.

    template<bool Pred, typename Then>
    typename boost::enable_if_c<Pred, bool>::type
    condition_if_c(Then f, bool /* else_ */ = true) { return f(); }

    template<bool Pred, typename Then>
    typename boost::disable_if_c<Pred, bool>::type
    condition_if_c(Then /* f */, bool else_ = true) { return else_; }
#endif

/**
Select compilation and execution of a boolean functor template condition using a
nullary boolean meta-function (not needed on C++17 compilers, use
<c>if constexpr</c> instead).

This is equivalent to
<c>boost::contract::condition_if_c<Pred::value>(f, else_)</c>.
Compile and execute the nullary boolean functor template call @c f() if and only
if the specified nullary boolean meta-function @p Pred::value is @c true,
otherwise trivially return @p else_ (@c true by default) at run-time.

@see    @RefSect{extras.assertion_requirements__templates_,
        Assertion Requirements}

@param f    Nullary boolean functor template.
            The functor template call @c f() is compiled and executed if and
            only if @c Pred::value is @c true.
@param else_    Boolean value to return when @c Pred::value is @c false (instead
                of compiling and executing the functor template call @c f()).

@tparam Pred    Nullary boolean meta-function selecting when the functor
                template call @c f() should be compiled and executed.

@return Boolean value returned by @c f() if the static predicate @c Pred::value
        is @c true. Otherwise, trivially return @p else_.
*/
template<class Pred, typename Then>
bool condition_if(Then f, bool else_ = true) {
    return condition_if_c<Pred::value>(f, else_);
}

} } // namespace

#endif // #include guard

