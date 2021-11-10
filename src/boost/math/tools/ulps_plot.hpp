//  (C) Copyright Nick Thompson 2020.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_MATH_TOOLS_ULP_PLOT_HPP
#define BOOST_MATH_TOOLS_ULP_PLOT_HPP
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <vector>
#include <utility>
#include <fstream>
#include <string>
#include <list>
#include <random>
#include <limits>
#include <stdexcept>
#include <boost/math/tools/is_standalone.hpp>
#include <boost/math/tools/condition_numbers.hpp>

#ifndef BOOST_MATH_STANDALONE
#include <boost/random/uniform_real_distribution.hpp>
#endif

// Design of this function comes from:
// https://blogs.mathworks.com/cleve/2017/01/23/ulps-plots-reveal-math-function-accurary/

// The envelope is the maximum of 1/2 and half the condition number of function evaluation.

namespace boost::math::tools {

namespace detail {
template<class F1, class F2, class CoarseReal, class PreciseReal>
void write_gridlines(std::ostream& fs, int horizontal_lines, int vertical_lines,
                     F1 x_scale, F2 y_scale, CoarseReal min_x, CoarseReal max_x, PreciseReal min_y, PreciseReal max_y,
                     int graph_width, int graph_height, int margin_left, std::string const & font_color)
{
  // Make a grid:
  for (int i = 1; i <= horizontal_lines; ++i) {
      PreciseReal y_cord_dataspace = min_y +  ((max_y - min_y)*i)/horizontal_lines;
      auto y = y_scale(y_cord_dataspace);
      fs << "<line x1='0' y1='" << y << "' x2='" << graph_width
         << "' y2='" << y
         << "' stroke='gray' stroke-width='1' opacity='0.5' stroke-dasharray='4' />\n";

      fs << "<text x='" <<  -margin_left/4 + 5 << "' y='" << y - 3
         << "' font-family='times' font-size='10' fill='" << font_color << "' transform='rotate(-90 "
         << -margin_left/4 + 8 << " " << y + 5 << ")'>"
         << std::setprecision(4) << y_cord_dataspace << "</text>\n";
   }

    for (int i = 1; i <= vertical_lines; ++i) {
        CoarseReal x_cord_dataspace = min_x +  ((max_x - min_x)*i)/vertical_lines;
        CoarseReal x = x_scale(x_cord_dataspace);
        fs << "<line x1='" << x << "' y1='0' x2='" << x
           << "' y2='" << graph_height
           << "' stroke='gray' stroke-width='1' opacity='0.5' stroke-dasharray='4' />\n";

        fs << "<text x='" <<  x - 10  << "' y='" << graph_height + 10
           << "' font-family='times' font-size='10' fill='" << font_color << "'>"
           << std::setprecision(4) << x_cord_dataspace << "</text>\n";
    }
}
}

template<class F, typename PreciseReal, typename CoarseReal>
class ulps_plot {
public:
    ulps_plot(F hi_acc_impl, CoarseReal a, CoarseReal b,
             size_t samples = 1000, bool perturb_abscissas = false, int random_seed = -1);

    ulps_plot& clip(PreciseReal clip);

    ulps_plot& width(int width);

    ulps_plot& envelope_color(std::string const & color);

    ulps_plot& title(std::string const & title);

    ulps_plot& background_color(std::string const & background_color);

    ulps_plot& font_color(std::string const & font_color);

    ulps_plot& crop_color(std::string const & color);

    ulps_plot& nan_color(std::string const & color);

    ulps_plot& ulp_envelope(bool write_ulp);

    template<class G>
    ulps_plot& add_fn(G g, std::string const & color = "steelblue");

    ulps_plot& horizontal_lines(int horizontal_lines);

    ulps_plot& vertical_lines(int vertical_lines);

    void write(std::string const & filename) const;

    friend std::ostream& operator<<(std::ostream& fs, ulps_plot const & plot)
    {
        using std::abs;
        using std::floor;
        using std::isnan;
        if (plot.ulp_list_.size() == 0)
        {
            throw std::domain_error("No functions added for comparison.");
        }
        if (plot.width_ <= 1)
        {
            throw std::domain_error("Width = " + std::to_string(plot.width_) + ", which is too small.");
        }

        PreciseReal worst_ulp_distance = 0;
        PreciseReal min_y = (std::numeric_limits<PreciseReal>::max)();
        PreciseReal max_y = std::numeric_limits<PreciseReal>::lowest();
        for (auto const & ulp_vec : plot.ulp_list_)
        {
            for (auto const & ulp : ulp_vec)
            {
                if (static_cast<PreciseReal>(abs(ulp)) > worst_ulp_distance)
                {
                    worst_ulp_distance = static_cast<PreciseReal>(abs(ulp));
                }
                if (static_cast<PreciseReal>(ulp) < min_y)
                {
                    min_y = static_cast<PreciseReal>(ulp);
                }
                if (static_cast<PreciseReal>(ulp) > max_y)
                {
                    max_y = static_cast<PreciseReal>(ulp);
                }
            }
        }

        // half-ulp accuracy is the best that can be expected; sometimes we can get less, but barely less.
        // then the axes don't show up; painful!
        if (max_y < 0.5) {
            max_y = 0.5;
        }
        if (min_y > -0.5) {
            min_y = -0.5;
        }

        if (plot.clip_ > 0)
        {
            if (max_y > plot.clip_)
            {
                max_y = plot.clip_;
            }
            if (min_y < -plot.clip_)
            {
                min_y = -plot.clip_;
            }
        }

        int height = static_cast<int>(floor(double(plot.width_)/1.61803));
        int margin_top = 40;
        int margin_left = 25;
        if (plot.title_.size() == 0)
        {
            margin_top = 10;
            margin_left = 15;
        }
        int margin_bottom = 20;
        int margin_right = 20;
        int graph_height = height - margin_bottom - margin_top;
        int graph_width = plot.width_ - margin_left - margin_right;

        // Maps [a,b] to [0, graph_width]
        auto x_scale = [&](CoarseReal x)->CoarseReal
        {
            return ((x-plot.a_)/(plot.b_ - plot.a_))*static_cast<CoarseReal>(graph_width);
        };

        auto y_scale = [&](PreciseReal y)->PreciseReal
        {
            return ((max_y - y)/(max_y - min_y) )*static_cast<PreciseReal>(graph_height);
        };

        fs << "<?xml version=\"1.0\" encoding='UTF-8' ?>\n"
           << "<svg xmlns='http://www.w3.org/2000/svg' width='"
           << plot.width_ << "' height='"
           << height << "'>\n"
           << "<style>\nsvg { background-color:" << plot.background_color_ << "; }\n"
           << "</style>\n";
        if (plot.title_.size() > 0)
        {
            fs << "<text x='" << floor(plot.width_/2)
               << "' y='" << floor(margin_top/2)
               << "' font-family='Palatino' font-size='25' fill='"
               << plot.font_color_  << "'  alignment-baseline='middle' text-anchor='middle'>"
               << plot.title_
               << "</text>\n";
        }

        // Construct SVG group to simplify the calculations slightly:
        fs << "<g transform='translate(" << margin_left << ", " << margin_top << ")'>\n";
            // y-axis:
        fs  << "<line x1='0' y1='0' x2='0' y2='" << graph_height
            << "' stroke='gray' stroke-width='1'/>\n";
        PreciseReal x_axis_loc = y_scale(static_cast<PreciseReal>(0));
        fs << "<line x1='0' y1='" << x_axis_loc
            << "' x2='" << graph_width << "' y2='" << x_axis_loc
            << "' stroke='gray' stroke-width='1'/>\n";

        if (worst_ulp_distance > 3)
        {
            detail::write_gridlines(fs, plot.horizontal_lines_, plot.vertical_lines_, x_scale, y_scale, plot.a_, plot.b_,
                                    min_y, max_y, graph_width, graph_height, margin_left, plot.font_color_);
        }
        else
        {
            std::vector<double> ys{-3.0, -2.5, -2.0, -1.5, -1.0, -0.5, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0};
            for (size_t i = 0; i < ys.size(); ++i)
            {
                if (min_y <= ys[i] && ys[i] <= max_y)
                {
                    PreciseReal y_cord_dataspace = ys[i];
                    PreciseReal y = y_scale(y_cord_dataspace);
                    fs << "<line x1='0' y1='" << y << "' x2='" << graph_width
                       << "' y2='" << y
                       << "' stroke='gray' stroke-width='1' opacity='0.5' stroke-dasharray='4' />\n";

                    fs << "<text x='" <<  -margin_left/2 << "' y='" << y - 3
                       << "' font-family='times' font-size='10' fill='" << plot.font_color_ << "' transform='rotate(-90 "
                       << -margin_left/2 + 7 << " " << y << ")'>"
                       <<  std::setprecision(4) << y_cord_dataspace << "</text>\n";
                }
            }
            for (int i = 1; i <= plot.vertical_lines_; ++i)
            {
                CoarseReal x_cord_dataspace = plot.a_ +  ((plot.b_ - plot.a_)*i)/plot.vertical_lines_;
                CoarseReal x = x_scale(x_cord_dataspace);
                fs << "<line x1='" << x << "' y1='0' x2='" << x
                   << "' y2='" << graph_height
                   << "' stroke='gray' stroke-width='1' opacity='0.5' stroke-dasharray='4' />\n";

                fs << "<text x='" <<  x - 10  << "' y='" << graph_height + 10
                   << "' font-family='times' font-size='10' fill='" << plot.font_color_ << "'>"
                   << std::setprecision(4) << x_cord_dataspace << "</text>\n";
            }
        }

        int color_idx = 0;
        for (auto const & ulp : plot.ulp_list_)
        {
            std::string color = plot.colors_[color_idx++];
            for (size_t j = 0; j < ulp.size(); ++j)
            {
                if (isnan(ulp[j]))
                {
                    if(plot.nan_color_ == "")
                        continue;
                    CoarseReal x = x_scale(plot.coarse_abscissas_[j]);
                    PreciseReal y = y_scale(static_cast<PreciseReal>(plot.clip_));
                    fs << "<circle cx='" << x << "' cy='" << y << "' r='1' fill='" << plot.nan_color_ << "'/>\n";
                    y = y_scale(static_cast<PreciseReal>(-plot.clip_));
                    fs << "<circle cx='" << x << "' cy='" << y << "' r='1' fill='" << plot.nan_color_ << "'/>\n";
                }
                if (plot.clip_ > 0 && static_cast<PreciseReal>(abs(ulp[j])) > plot.clip_)
                {
                   if (plot.crop_color_ == "")
                      continue;
                   CoarseReal x = x_scale(plot.coarse_abscissas_[j]);
                   PreciseReal y = y_scale(static_cast<PreciseReal>(ulp[j] < 0 ? -plot.clip_ : plot.clip_));
                   fs << "<circle cx='" << x << "' cy='" << y << "' r='1' fill='" << plot.crop_color_ << "'/>\n";
                }
                else
                {
                   CoarseReal x = x_scale(plot.coarse_abscissas_[j]);
                   PreciseReal y = y_scale(static_cast<PreciseReal>(ulp[j]));
                   fs << "<circle cx='" << x << "' cy='" << y << "' r='1' fill='" << color << "'/>\n";
                }
            }
        }

        if (plot.ulp_envelope_)
        {
            std::string close_path = "' stroke='"  + plot.envelope_color_ + "' stroke-width='1' fill='none'></path>\n";
            size_t jstart = 0;
            while (plot.cond_[jstart] > max_y)
            {
                ++jstart;
                if (jstart >= plot.cond_.size())
                {
                    goto done;
                }
            }

            size_t jmin = jstart;
        new_top_path:
            if (jmin >= plot.cond_.size())
            {
                goto start_bottom_paths;
            }
            fs << "<path d='M" << x_scale(plot.coarse_abscissas_[jmin]) << " " << y_scale(plot.cond_[jmin]);

            for (size_t j = jmin + 1; j < plot.coarse_abscissas_.size(); ++j)
            {
                bool bad = isnan(plot.cond_[j]) || (plot.cond_[j] > max_y);
                if (bad)
                {
                    ++j;
                    while ( (j < plot.coarse_abscissas_.size() - 2) && bad)
                    {
                        bad = isnan(plot.cond_[j]) || (plot.cond_[j] > max_y);
                        ++j;
                    }
                    jmin = j;
                    fs << close_path;
                    goto new_top_path;
                }

                CoarseReal t = x_scale(plot.coarse_abscissas_[j]);
                PreciseReal y = y_scale(plot.cond_[j]);
                fs << " L" << t << " " << y;
            }
            fs << close_path;
        start_bottom_paths:
            jmin = jstart;
        new_bottom_path:
            if (jmin >= plot.cond_.size())
            {
                goto done;
            }
            fs << "<path d='M" << x_scale(plot.coarse_abscissas_[jmin]) << " " << y_scale(-plot.cond_[jmin]);

            for (size_t j = jmin + 1; j < plot.coarse_abscissas_.size(); ++j)
            {
                bool bad = isnan(plot.cond_[j]) || (-plot.cond_[j] < min_y);
                if (bad)
                {
                    ++j;
                    while ( (j < plot.coarse_abscissas_.size() - 2) && bad)
                    {
                        bad = isnan(plot.cond_[j]) || (-plot.cond_[j] < min_y);
                        ++j;
                    }
                    jmin = j;
                    fs << close_path;
                    goto new_bottom_path;
                }
                CoarseReal t = x_scale(plot.coarse_abscissas_[j]);
                PreciseReal y = y_scale(-plot.cond_[j]);
                fs << " L" << t << " " << y;
            }
            fs << close_path;
        }
    done:
        fs << "</g>\n"
           << "</svg>\n";
        return fs;
    }

private:
    std::vector<PreciseReal> precise_abscissas_;
    std::vector<CoarseReal> coarse_abscissas_;
    std::vector<PreciseReal> precise_ordinates_;
    std::vector<PreciseReal> cond_;
    std::list<std::vector<CoarseReal>> ulp_list_;
    std::vector<std::string> colors_;
    CoarseReal a_;
    CoarseReal b_;
    PreciseReal clip_;
    int width_;
    std::string envelope_color_;
    bool ulp_envelope_;
    int horizontal_lines_;
    int vertical_lines_;
    std::string title_;
    std::string background_color_;
    std::string font_color_;
    std::string crop_color_;
    std::string nan_color_;
};

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::envelope_color(std::string const & color)
{
    envelope_color_ = color;
    return *this;
}

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::clip(PreciseReal clip)
{
    clip_ = clip;
    return *this;
}

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::width(int width)
{
    width_ = width;
    return *this;
}

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::horizontal_lines(int horizontal_lines)
{
    horizontal_lines_ = horizontal_lines;
    return *this;
}

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::vertical_lines(int vertical_lines)
{
    vertical_lines_ = vertical_lines;
    return *this;
}

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::title(std::string const & title)
{
    title_ = title;
    return *this;
}

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::background_color(std::string const & background_color)
{
    background_color_ = background_color;
    return *this;
}

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::font_color(std::string const & font_color)
{
    font_color_ = font_color;
    return *this;
}

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::crop_color(std::string const & color)
{
    crop_color_ = color;
    return *this;
}

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::nan_color(std::string const & color)
{
    nan_color_ = color;
    return *this;
}

template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::ulp_envelope(bool write_ulp_envelope)
{
    ulp_envelope_ = write_ulp_envelope;
    return *this;
}

namespace detail{
bool ends_with(std::string const& filename, std::string const& suffix)
{
    if(filename.size() < suffix.size())
    {
        return false;
    }

    return std::equal(std::begin(suffix), std::end(suffix), std::end(filename) - suffix.size());
}
}

template<class F, typename PreciseReal, typename CoarseReal>
void ulps_plot<F, PreciseReal, CoarseReal>::write(std::string const & filename) const
{
    if(!boost::math::tools::detail::ends_with(filename, ".svg"))
    {
        throw std::logic_error("Only svg files are supported at this time.");
    }
    std::ofstream fs(filename);
    fs << *this;
    fs.close();
}


template<class F, typename PreciseReal, typename CoarseReal>
ulps_plot<F, PreciseReal, CoarseReal>::ulps_plot(F hi_acc_impl, CoarseReal a, CoarseReal b,
             size_t samples, bool perturb_abscissas, int random_seed) : crop_color_("red")
{
    // Use digits10 for this comparison in case the two types have differeing radixes:
    static_assert(std::numeric_limits<PreciseReal>::digits10 >= std::numeric_limits<CoarseReal>::digits10, "PreciseReal must have higher precision that CoarseReal");
    if (samples < 10)
    {
        throw std::domain_error("Must have at least 10 samples, samples = " + std::to_string(samples));
    }
    if (b <= a)
    {
        throw std::domain_error("On interval [a,b], b > a is required.");
    }
    a_ = a;
    b_ = b;

    std::mt19937_64 gen;
    if (random_seed == -1)
    {
        std::random_device rd;
        gen.seed(rd());
    }

    // Boost's uniform_real_distribution can generate quad and multiprecision random numbers; std's cannot:
    #ifndef BOOST_MATH_STANDALONE
    boost::random::uniform_real_distribution<PreciseReal> dis(static_cast<PreciseReal>(a), static_cast<PreciseReal>(b));
    #else
    // Use std::random in standalone mode if it is a type that the standard library can support (float, double, or long double)
    static_assert(std::numeric_limits<PreciseReal>::digits10 <= std::numeric_limits<long double>::digits10, "Standalone mode does not support types with precision that exceeds long double");
    std::uniform_real_distribution<PreciseReal> dis(static_cast<PreciseReal>(a), static_cast<PreciseReal>(b));
    #endif
    
    precise_abscissas_.resize(samples);
    coarse_abscissas_.resize(samples);

    if (perturb_abscissas)
    {
        for(size_t i = 0; i < samples; ++i)
        {
            precise_abscissas_[i] = dis(gen);
        }
        std::sort(precise_abscissas_.begin(), precise_abscissas_.end());
        for (size_t i = 0; i < samples; ++i)
        {
            coarse_abscissas_[i] = static_cast<CoarseReal>(precise_abscissas_[i]);
        }
    }
    else
    {
        for(size_t i = 0; i < samples; ++i)
        {
            coarse_abscissas_[i] = static_cast<CoarseReal>(dis(gen));
        }
        std::sort(coarse_abscissas_.begin(), coarse_abscissas_.end());
        for (size_t i = 0; i < samples; ++i)
        {
            precise_abscissas_[i] = static_cast<PreciseReal>(coarse_abscissas_[i]);
        }
    }

    precise_ordinates_.resize(samples);
    for (size_t i = 0; i < samples; ++i)
    {
        precise_ordinates_[i] = hi_acc_impl(precise_abscissas_[i]);
    }

    cond_.resize(samples, std::numeric_limits<PreciseReal>::quiet_NaN());
    for (size_t i = 0 ; i < samples; ++i)
    {
        PreciseReal y = precise_ordinates_[i];
        if (y != 0)
        {
            // Maybe cond_ is badly names; should it be half_cond_?
            cond_[i] = boost::math::tools::evaluation_condition_number(hi_acc_impl, precise_abscissas_[i])/2;
            // Half-ULP accuracy is the correctly rounded result, so make sure the envelop doesn't go below this:
            if (cond_[i] < 0.5)
            {
                cond_[i] = 0.5;
            }
        }
        // else leave it as nan.
    }
    clip_ = -1;
    width_ = 1100;
    envelope_color_ = "chartreuse";
    ulp_envelope_ = true;
    horizontal_lines_ = 8;
    vertical_lines_ = 10;
    title_ = "";
    background_color_ = "black";
    font_color_ = "white";
}

template<class F, typename PreciseReal, typename CoarseReal>
template<class G>
ulps_plot<F, PreciseReal, CoarseReal>& ulps_plot<F, PreciseReal, CoarseReal>::add_fn(G g, std::string const & color)
{
    using std::abs;
    size_t samples = precise_abscissas_.size();
    std::vector<CoarseReal> ulps(samples);
    for (size_t i = 0; i < samples; ++i)
    {
        PreciseReal y_hi_acc = precise_ordinates_[i];
        PreciseReal y_lo_acc = static_cast<PreciseReal>(g(coarse_abscissas_[i]));
        PreciseReal absy = abs(y_hi_acc);
        PreciseReal dist = static_cast<PreciseReal>(nextafter(static_cast<CoarseReal>(absy), (std::numeric_limits<CoarseReal>::max)()) - static_cast<CoarseReal>(absy));
        ulps[i] = static_cast<CoarseReal>((y_lo_acc - y_hi_acc)/dist);
    }
    ulp_list_.emplace_back(ulps);
    colors_.emplace_back(color);
    return *this;
}




} // namespace boost::math::tools
#endif
