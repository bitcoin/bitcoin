///////////////////////////////////////////////////////////////////////////////
// droppable_accumulator.hpp
//
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_FRAMEWORK_ACCUMULATORS_DROPPABLE_ACCUMULATOR_HPP_EAN_13_12_2005
#define BOOST_ACCUMULATORS_FRAMEWORK_ACCUMULATORS_DROPPABLE_ACCUMULATOR_HPP_EAN_13_12_2005

#include <new>
#include <boost/assert.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/accumulators/framework/depends_on.hpp> // for feature_of
#include <boost/accumulators/framework/parameters/accumulator.hpp> // for accumulator

namespace boost { namespace accumulators
{

    template<typename Accumulator>
    struct droppable_accumulator;

    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////////////
        // add_ref_visitor
        //   a fusion function object for add_ref'ing accumulators
        template<typename Args>
        struct add_ref_visitor
        {
            explicit add_ref_visitor(Args const &args)
              : args_(args)
            {
            }

            template<typename Accumulator>
            void operator ()(Accumulator &acc) const
            {
                typedef typename Accumulator::feature_tag::dependencies dependencies;

                acc.add_ref(this->args_);

                // Also add_ref accumulators that this feature depends on
                this->args_[accumulator].template
                    visit_if<detail::contains_feature_of_<dependencies> >(
                        *this
                );
            }

        private:
            add_ref_visitor &operator =(add_ref_visitor const &);
            Args const &args_;
        };

        template<typename Args>
        add_ref_visitor<Args> make_add_ref_visitor(Args const &args)
        {
            return add_ref_visitor<Args>(args);
        }

        ///////////////////////////////////////////////////////////////////////////////
        // drop_visitor
        //   a fusion function object for dropping accumulators
        template<typename Args>
        struct drop_visitor
        {
            explicit drop_visitor(Args const &args)
              : args_(args)
            {
            }

            template<typename Accumulator>
            void operator ()(Accumulator &acc) const
            {
                if(typename Accumulator::is_droppable())
                {
                    typedef typename Accumulator::feature_tag::dependencies dependencies;

                    acc.drop(this->args_);
                    // Also drop accumulators that this feature depends on
                    this->args_[accumulator].template
                        visit_if<detail::contains_feature_of_<dependencies> >(
                            *this
                    );
                }
            }

        private:
            drop_visitor &operator =(drop_visitor const &);
            Args const &args_;
        };

        template<typename Args>
        drop_visitor<Args> make_drop_visitor(Args const &args)
        {
            return drop_visitor<Args>(args);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // droppable_accumulator_base
    template<typename Accumulator>
    struct droppable_accumulator_base
      : Accumulator
    {
        typedef droppable_accumulator_base base;
        typedef mpl::true_ is_droppable;
        typedef typename Accumulator::result_type result_type;

        template<typename Args>
        droppable_accumulator_base(Args const &args)
          : Accumulator(args)
          , ref_count_(0)
        {
        }

        droppable_accumulator_base(droppable_accumulator_base const &that)
          : Accumulator(*static_cast<Accumulator const *>(&that))
          , ref_count_(that.ref_count_)
        {
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            if(!this->is_dropped())
            {
                this->Accumulator::operator ()(args);
            }
        }

        template<typename Args>
        void add_ref(Args const &)
        {
            ++this->ref_count_;
        }

        template<typename Args>
        void drop(Args const &args)
        {
            BOOST_ASSERT(0 < this->ref_count_);
            if(1 == this->ref_count_)
            {
                static_cast<droppable_accumulator<Accumulator> *>(this)->on_drop(args);
            }
            --this->ref_count_;
        }

        bool is_dropped() const
        {
            return 0 == this->ref_count_;
        }

    private:
        int ref_count_;
    };

    //////////////////////////////////////////////////////////////////////////
    // droppable_accumulator
    //   this can be specialized for any type that needs special handling
    template<typename Accumulator>
    struct droppable_accumulator
      : droppable_accumulator_base<Accumulator>
    {
        template<typename Args>
        droppable_accumulator(Args const &args)
          : droppable_accumulator::base(args)
        {
        }

        droppable_accumulator(droppable_accumulator const &that)
          : droppable_accumulator::base(*static_cast<typename droppable_accumulator::base const *>(&that))
        {
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // with_cached_result
    template<typename Accumulator>
    struct with_cached_result
      : Accumulator
    {
        typedef typename Accumulator::result_type result_type;

        template<typename Args>
        with_cached_result(Args const &args)
          : Accumulator(args)
          , cache()
        {
        }

        with_cached_result(with_cached_result const &that)
          : Accumulator(*static_cast<Accumulator const *>(&that))
          , cache()
        {
            if(that.has_result())
            {
                this->set(that.get());
            }
        }

        ~with_cached_result()
        {
            // Since this is a base class of droppable_accumulator_base,
            // this destructor is called before any of droppable_accumulator_base's
            // members get cleaned up, including is_dropped, so the following
            // call to has_result() is valid.
            if(this->has_result())
            {
                this->get().~result_type();
            }
        }

        template<typename Args>
        void on_drop(Args const &args)
        {
            // cache the result at the point this calculation was dropped
            BOOST_ASSERT(!this->has_result());
            this->set(this->Accumulator::result(args));
        }

        template<typename Args>
        result_type result(Args const &args) const
        {
            return this->has_result() ? this->get() : this->Accumulator::result(args);
        }

    private:
        with_cached_result &operator =(with_cached_result const &);

        void set(result_type const &r)
        {
            ::new(this->cache.address()) result_type(r);
        }

        result_type const &get() const
        {
            return *static_cast<result_type const *>(this->cache.address());
        }

        bool has_result() const
        {
            typedef with_cached_result<Accumulator> this_type;
            typedef droppable_accumulator_base<this_type> derived_type;
            return static_cast<derived_type const *>(this)->is_dropped();
        }

        aligned_storage<sizeof(result_type)> cache;
    };

    namespace tag
    {
        template<typename Feature>
        struct as_droppable
        {
            typedef droppable<Feature> type;
        };

        template<typename Feature>
        struct as_droppable<droppable<Feature> >
        {
            typedef droppable<Feature> type;
        };

        //////////////////////////////////////////////////////////////////////////
        // droppable
        template<typename Feature>
        struct droppable
          : as_feature<Feature>::type
        {
            typedef typename as_feature<Feature>::type feature_type;
            typedef typename feature_type::dependencies tmp_dependencies_;

            typedef
                typename mpl::transform<
                    typename feature_type::dependencies
                  , as_droppable<mpl::_1>
                >::type
            dependencies;

            struct impl
            {
                template<typename Sample, typename Weight>
                struct apply
                {
                    typedef
                        droppable_accumulator<
                            typename mpl::apply2<typename feature_type::impl, Sample, Weight>::type
                        >
                    type;
                };
            };
        };
    }

    // make droppable<tag::feature(modifier)> work
    template<typename Feature>
    struct as_feature<tag::droppable<Feature> >
    {
        typedef tag::droppable<typename as_feature<Feature>::type> type;
    };

    // make droppable<tag::mean> work with non-void weights (should become
    // droppable<tag::weighted_mean>
    template<typename Feature>
    struct as_weighted_feature<tag::droppable<Feature> >
    {
        typedef tag::droppable<typename as_weighted_feature<Feature>::type> type;
    };

    // for the purposes of feature-based dependency resolution,
    // droppable<Foo> provides the same feature as Foo
    template<typename Feature>
    struct feature_of<tag::droppable<Feature> >
      : feature_of<Feature>
    {
    };

    // Note: Usually, the extractor is pulled into the accumulators namespace with
    // a using directive, not the tag. But the droppable<> feature doesn't have an
    // extractor, so we can put the droppable tag in the accumulators namespace
    // without fear of a name conflict.
    using tag::droppable;

}} // namespace boost::accumulators

#endif
