/* boost random/normal_distribution.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Copyright Steven Watanabe 2010-2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_NORMAL_DISTRIBUTION_HPP
#define BOOST_RANDOM_NORMAL_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <istream>
#include <iosfwd>
#include <boost/assert.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/int_float_pair.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/exponential_distribution.hpp>

namespace boost {
namespace random {

namespace detail {

// tables for the ziggurat algorithm
template<class RealType>
struct normal_table {
    static const RealType table_x[129];
    static const RealType table_y[129];
};

template<class RealType>
const RealType normal_table<RealType>::table_x[129] = {
    3.7130862467403632609, 3.4426198558966521214, 3.2230849845786185446, 3.0832288582142137009,
    2.9786962526450169606, 2.8943440070186706210, 2.8231253505459664379, 2.7611693723841538514,
    2.7061135731187223371, 2.6564064112581924999, 2.6109722484286132035, 2.5690336259216391328,
    2.5300096723854666170, 2.4934545220919507609, 2.4590181774083500943, 2.4264206455302115930,
    2.3954342780074673425, 2.3658713701139875435, 2.3375752413355307354, 2.3104136836950021558,
    2.2842740596736568056, 2.2590595738653295251, 2.2346863955870569803, 2.2110814088747278106,
    2.1881804320720206093, 2.1659267937448407377, 2.1442701823562613518, 2.1231657086697899595,
    2.1025731351849988838, 2.0824562379877246441, 2.0627822745039633575, 2.0435215366506694976,
    2.0246469733729338782, 2.0061338699589668403, 1.9879595741230607243, 1.9701032608497132242,
    1.9525457295488889058, 1.9352692282919002011, 1.9182573008597320303, 1.9014946531003176140,
    1.8849670357028692380, 1.8686611409895420085, 1.8525645117230870617, 1.8366654602533840447,
    1.8209529965910050740, 1.8054167642140487420, 1.7900469825946189862, 1.7748343955807692457,
    1.7597702248942318749, 1.7448461281083765085, 1.7300541605582435350, 1.7153867407081165482,
    1.7008366185643009437, 1.6863968467734863258, 1.6720607540918522072, 1.6578219209482075462,
    1.6436741568569826489, 1.6296114794646783962, 1.6156280950371329644, 1.6017183802152770587,
    1.5878768648844007019, 1.5740982160167497219, 1.5603772223598406870, 1.5467087798535034608,
    1.5330878776675560787, 1.5195095847593707806, 1.5059690368565502602, 1.4924614237746154081,
    1.4789819769830978546, 1.4655259573357946276, 1.4520886428822164926, 1.4386653166774613138,
    1.4252512545068615734, 1.4118417124397602509, 1.3984319141236063517, 1.3850170377251486449,
    1.3715922024197322698, 1.3581524543224228739, 1.3446927517457130432, 1.3312079496576765017,
    1.3176927832013429910, 1.3041418501204215390, 1.2905495919178731508, 1.2769102735516997175,
    1.2632179614460282310, 1.2494664995643337480, 1.2356494832544811749, 1.2217602305309625678,
    1.2077917504067576028, 1.1937367078237721994, 1.1795873846544607035, 1.1653356361550469083,
    1.1509728421389760651, 1.1364898520030755352, 1.1218769225722540661, 1.1071236475235353980,
    1.0922188768965537614, 1.0771506248819376573, 1.0619059636836193998, 1.0464709007525802629,
    1.0308302360564555907, 1.0149673952392994716, 0.99886423348064351303, 0.98250080350276038481,
    0.96585507938813059489, 0.94890262549791195381, 0.93161619660135381056, 0.91396525100880177644,
    0.89591535256623852894, 0.87742742909771569142, 0.85845684317805086354, 0.83895221428120745572,
    0.81885390668331772331, 0.79809206062627480454, 0.77658398787614838598, 0.75423066443451007146,
    0.73091191062188128150, 0.70647961131360803456, 0.68074791864590421664, 0.65347863871504238702,
    0.62435859730908822111, 0.59296294244197797913, 0.55869217837551797140, 0.52065603872514491759,
    0.47743783725378787681, 0.42654798630330512490, 0.36287143102841830424, 0.27232086470466385065,
    0
};

template<class RealType>
const RealType normal_table<RealType>::table_y[129] = {
    0, 0.0026696290839025035092, 0.0055489952208164705392, 0.0086244844129304709682,
    0.011839478657982313715, 0.015167298010672042468, 0.018592102737165812650, 0.022103304616111592615,
    0.025693291936149616572, 0.029356317440253829618, 0.033087886146505155566, 0.036884388786968774128,
    0.040742868074790604632, 0.044660862200872429800, 0.048636295860284051878, 0.052667401903503169793,
    0.056752663481538584188, 0.060890770348566375972, 0.065080585213631873753, 0.069321117394180252601,
    0.073611501884754893389, 0.077950982514654714188, 0.082338898242957408243, 0.086774671895542968998,
    0.091257800827634710201, 0.09578784912257815216, 0.10036444102954554013, 0.10498725541035453978,
    0.10965602101581776100, 0.11437051244988827452, 0.11913054670871858767, 0.12393598020398174246,
    0.12878670619710396109, 0.13368265258464764118, 0.13862377998585103702, 0.14361008009193299469,
    0.14864157424369696566, 0.15371831220958657066, 0.15884037114093507813, 0.16400785468492774791,
    0.16922089223892475176, 0.17447963833240232295, 0.17978427212496211424, 0.18513499701071343216,
    0.19053204032091372112, 0.19597565311811041399, 0.20146611007620324118, 0.20700370944187380064,
    0.21258877307373610060, 0.21822164655637059599, 0.22390269938713388747, 0.22963232523430270355,
    0.23541094226572765600, 0.24123899354775131610, 0.24711694751469673582, 0.25304529850976585934,
    0.25902456739871074263, 0.26505530225816194029, 0.27113807914102527343, 0.27727350292189771153,
    0.28346220822601251779, 0.28970486044581049771, 0.29600215684985583659, 0.30235482778947976274,
    0.30876363800925192282, 0.31522938806815752222, 0.32175291587920862031, 0.32833509837615239609,
    0.33497685331697116147, 0.34167914123501368412, 0.34844296754987246935, 0.35526938485154714435,
    0.36215949537303321162, 0.36911445366827513952, 0.37613546951445442947, 0.38322381105988364587,
    0.39038080824138948916, 0.39760785649804255208, 0.40490642081148835099, 0.41227804010702462062,
    0.41972433205403823467, 0.42724699830956239880, 0.43484783025466189638, 0.44252871528024661483,
    0.45029164368692696086, 0.45813871627287196483, 0.46607215269457097924, 0.47409430069824960453,
    0.48220764633483869062, 0.49041482528932163741, 0.49871863547658432422, 0.50712205108130458951,
    0.51562823824987205196, 0.52424057267899279809, 0.53296265938998758838, 0.54179835503172412311,
    0.55075179312105527738, 0.55982741271069481791, 0.56902999107472161225, 0.57836468112670231279,
    0.58783705444182052571, 0.59745315095181228217, 0.60721953663260488551, 0.61714337082656248870,
    0.62723248525781456578, 0.63749547734314487428, 0.64794182111855080873, 0.65858200005865368016,
    0.66942766735770616891, 0.68049184100641433355, 0.69178914344603585279, 0.70333609902581741633,
    0.71515150742047704368, 0.72725691835450587793, 0.73967724368333814856, 0.75244155918570380145,
    0.76558417390923599480, 0.77914608594170316563, 0.79317701178385921053, 0.80773829469612111340,
    0.82290721139526200050, 0.83878360531064722379, 0.85550060788506428418, 0.87324304892685358879,
    0.89228165080230272301, 0.91304364799203805999, 0.93628268170837107547, 0.96359969315576759960,
    1
};


template<class RealType = double>
struct unit_normal_distribution
{
    template<class Engine>
    RealType operator()(Engine& eng) {
        const double * const table_x = normal_table<double>::table_x;
        const double * const table_y = normal_table<double>::table_y;
        for(;;) {
            std::pair<RealType, int> vals = generate_int_float_pair<RealType, 8>(eng);
            int i = vals.second;
            int sign = (i & 1) * 2 - 1;
            i = i >> 1;
            RealType x = vals.first * RealType(table_x[i]);
            if(x < table_x[i + 1]) return x * sign;
            if(i == 0) return generate_tail(eng) * sign;

            RealType y01 = uniform_01<RealType>()(eng);
            RealType y = RealType(table_y[i]) + y01 * RealType(table_y[i + 1] - table_y[i]);

            // These store the value y - bound, or something proportional to that difference:
            RealType y_above_ubound, y_above_lbound;

            // There are three cases to consider:
            // - convex regions (where x[i] > x[j] >= 1)
            // - concave regions (where 1 <= x[i] < x[j])
            // - region containing the inflection point (where x[i] > 1 > x[j])
            // For convex (concave), exp^(-x^2/2) is bounded below (above) by the tangent at
            // (x[i],y[i]) and is bounded above (below) by the diagonal line from (x[i+1],y[i+1]) to
            // (x[i],y[i]).
            //
            // *If* the inflection point region satisfies slope(x[i+1]) < slope(diagonal), then we
            // can treat the inflection region as a convex region: this condition is necessary and
            // sufficient to ensure that the curve lies entirely below the diagonal (that, in turn,
            // also implies that it will be above the tangent at x[i]).
            //
            // For the current table size (128), this is satisfied: slope(x[i+1]) = -0.60653 <
            // slope(diag) = -0.60649, and so we have only two cases below instead of three.

            if (table_x[i] >= 1) { // convex (incl. inflection)
                y_above_ubound = RealType(table_x[i] - table_x[i+1]) * y01 - (RealType(table_x[i]) - x);
                y_above_lbound = y - (RealType(table_y[i]) + (RealType(table_x[i]) - x) * RealType(table_y[i]) * RealType(table_x[i]));
            }
            else { // concave
                y_above_lbound = RealType(table_x[i] - table_x[i+1]) * y01 - (RealType(table_x[i]) - x);
                y_above_ubound = y - (RealType(table_y[i]) + (RealType(table_x[i]) - x) * RealType(table_y[i]) * RealType(table_x[i]));
            }

            if (y_above_ubound < 0 // if above the upper bound reject immediately
                    &&
                    (
                     y_above_lbound < 0 // If below the lower bound accept immediately
                     ||
                     y < f(x) // Otherwise it's between the bounds and we need a full check
                    )
               ) {
                return x * sign;
            }
        }
    }
    static RealType f(RealType x) {
        using std::exp;
        return exp(-(x*x/2));
    }
    // Generate from the tail using rejection sampling from the exponential(x_1) distribution,
    // shifted by x_1.  This looks a little different from the usual rejection sampling because it
    // transforms the condition by taking the log of both sides, thus avoiding the costly exp() call
    // on the RHS, then takes advantage of the fact that -log(unif01) is simply generating an
    // exponential (by inverse cdf sampling) by replacing the log(unif01) on the LHS with a
    // exponential(1) draw, y.
    template<class Engine>
    RealType generate_tail(Engine& eng) {
        const RealType tail_start = RealType(normal_table<double>::table_x[1]);
        boost::random::exponential_distribution<RealType> exp_x(tail_start);
        boost::random::exponential_distribution<RealType> exp_y;
        for(;;) {
            RealType x = exp_x(eng);
            RealType y = exp_y(eng);
            // If we were doing non-transformed rejection sampling, this condition would be:
            // if (unif01 < exp(-.5*x*x)) return x + tail_start;
            if(2*y > x*x) return x + tail_start;
        }
    }
};

} // namespace detail


/**
 * Instantiations of class template normal_distribution model a
 * \random_distribution. Such a distribution produces random numbers
 * @c x distributed with probability density function
 * \f$\displaystyle p(x) =
 *   \frac{1}{\sqrt{2\pi}\sigma} e^{-\frac{(x-\mu)^2}{2\sigma^2}}
 * \f$,
 * where mean and sigma are the parameters of the distribution.
 *
 * The implementation uses the "ziggurat" algorithm, as described in
 *
 *  @blockquote
 *  "The Ziggurat Method for Generating Random Variables",
 *  George Marsaglia and Wai Wan Tsang, Journal of Statistical Software,
 *  Volume 5, Number 8 (2000), 1-7.
 *  @endblockquote
 */
template<class RealType = double>
class normal_distribution
{
public:
    typedef RealType input_type;
    typedef RealType result_type;

    class param_type {
    public:
        typedef normal_distribution distribution_type;

        /**
         * Constructs a @c param_type with a given mean and
         * standard deviation.
         *
         * Requires: sigma >= 0
         */
        explicit param_type(RealType mean_arg = RealType(0.0),
                            RealType sigma_arg = RealType(1.0))
          : _mean(mean_arg),
            _sigma(sigma_arg)
        {}

        /** Returns the mean of the distribution. */
        RealType mean() const { return _mean; }

        /** Returns the standand deviation of the distribution. */
        RealType sigma() const { return _sigma; }

        /** Writes a @c param_type to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        { os << parm._mean << " " << parm._sigma ; return os; }

        /** Reads a @c param_type from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        { is >> parm._mean >> std::ws >> parm._sigma; return is; }

        /** Returns true if the two sets of parameters are the same. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._mean == rhs._mean && lhs._sigma == rhs._sigma; }
        
        /** Returns true if the two sets of parameters are the different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        RealType _mean;
        RealType _sigma;
    };

    /**
     * Constructs a @c normal_distribution object. @c mean and @c sigma are
     * the parameters for the distribution.
     *
     * Requires: sigma >= 0
     */
    explicit normal_distribution(const RealType& mean_arg = RealType(0.0),
                                 const RealType& sigma_arg = RealType(1.0))
      : _mean(mean_arg), _sigma(sigma_arg)
    {
        BOOST_ASSERT(_sigma >= RealType(0));
    }

    /**
     * Constructs a @c normal_distribution object from its parameters.
     */
    explicit normal_distribution(const param_type& parm)
      : _mean(parm.mean()), _sigma(parm.sigma())
    {}

    /**  Returns the mean of the distribution. */
    RealType mean() const { return _mean; }
    /** Returns the standard deviation of the distribution. */
    RealType sigma() const { return _sigma; }

    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return -std::numeric_limits<RealType>::infinity(); }
    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return std::numeric_limits<RealType>::infinity(); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_mean, _sigma); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        _mean = parm.mean();
        _sigma = parm.sigma();
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /**  Returns a normal variate. */
    template<class Engine>
    result_type operator()(Engine& eng)
    {
        detail::unit_normal_distribution<RealType> impl;
        return impl(eng) * _sigma + _mean;
    }

    /** Returns a normal variate with parameters specified by @c param. */
    template<class URNG>
    result_type operator()(URNG& urng, const param_type& parm)
    {
        return normal_distribution(parm)(urng);
    }

    /** Writes a @c normal_distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, normal_distribution, nd)
    {
        os << nd._mean << " " << nd._sigma;
        return os;
    }

    /** Reads a @c normal_distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, normal_distribution, nd)
    {
        is >> std::ws >> nd._mean >> std::ws >> nd._sigma;
        return is;
    }

    /**
     * Returns true if the two instances of @c normal_distribution will
     * return identical sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(normal_distribution, lhs, rhs)
    {
        return lhs._mean == rhs._mean && lhs._sigma == rhs._sigma;
    }

    /**
     * Returns true if the two instances of @c normal_distribution will
     * return different sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(normal_distribution)

private:
    RealType _mean, _sigma;

};

} // namespace random

using random::normal_distribution;

} // namespace boost

#endif // BOOST_RANDOM_NORMAL_DISTRIBUTION_HPP
