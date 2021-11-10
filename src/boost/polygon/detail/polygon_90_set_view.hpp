/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_90_SET_VIEW_HPP
#define BOOST_POLYGON_POLYGON_90_SET_VIEW_HPP
namespace boost { namespace polygon{
  struct operator_provides_storage {};
  struct operator_requires_copy {};

  template <typename value_type, typename arg_type>
  inline void insert_into_view_arg(value_type& dest, const arg_type& arg, orientation_2d orient);

  template <typename ltype, typename rtype, typename op_type>
  class polygon_90_set_view;

  template <typename ltype, typename rtype, typename op_type>
  struct polygon_90_set_traits<polygon_90_set_view<ltype, rtype, op_type> > {
    typedef typename polygon_90_set_view<ltype, rtype, op_type>::coordinate_type coordinate_type;
    typedef typename polygon_90_set_view<ltype, rtype, op_type>::iterator_type iterator_type;
    typedef typename polygon_90_set_view<ltype, rtype, op_type>::operator_arg_type operator_arg_type;

    static inline iterator_type begin(const polygon_90_set_view<ltype, rtype, op_type>& polygon_set);
    static inline iterator_type end(const polygon_90_set_view<ltype, rtype, op_type>& polygon_set);

    static inline orientation_2d orient(const polygon_90_set_view<ltype, rtype, op_type>& polygon_set);

    static inline bool clean(const polygon_90_set_view<ltype, rtype, op_type>& polygon_set);

    static inline bool sorted(const polygon_90_set_view<ltype, rtype, op_type>& polygon_set);
  };

    template <typename value_type, typename ltype, typename rtype, typename op_type>
    struct compute_90_set_value {
      static
      void value(value_type& output_, const ltype& lvalue_, const rtype& rvalue_, orientation_2d orient_) {
        value_type linput_(orient_);
        value_type rinput_(orient_);
        orientation_2d orient_l = polygon_90_set_traits<ltype>::orient(lvalue_);
        orientation_2d orient_r = polygon_90_set_traits<rtype>::orient(rvalue_);
        //std::cout << "compute_90_set_value-0 orientations (left, right, out):\t" << orient_l.to_int()
        //        << "," << orient_r.to_int() << "," << orient_.to_int() << std::endl;
        insert_into_view_arg(linput_, lvalue_, orient_l);
        insert_into_view_arg(rinput_, rvalue_, orient_r);
        output_.applyBooleanBinaryOp(linput_.begin(), linput_.end(),
                                     rinput_.begin(), rinput_.end(), boolean_op::BinaryCount<op_type>());
      }
    };

    template <typename value_type, typename lcoord, typename rcoord, typename op_type>
    struct compute_90_set_value<value_type, polygon_90_set_data<lcoord>, polygon_90_set_data<rcoord>, op_type> {
      static
      void value(value_type& output_, const polygon_90_set_data<lcoord>& lvalue_,
                 const polygon_90_set_data<rcoord>& rvalue_, orientation_2d orient_) {
        orientation_2d orient_l = lvalue_.orient();
        orientation_2d orient_r = rvalue_.orient();
        value_type linput_(orient_);
        value_type rinput_(orient_);
        //std::cout << "compute_90_set_value-1 orientations (left, right, out):\t" << orient_l.to_int()
        //          << "," << orient_r.to_int() << "," << orient_.to_int() << std::endl;
        if((orient_ == orient_l) && (orient_== orient_r)){ // assume that most of the time this condition is met
          lvalue_.sort();
          rvalue_.sort();
          output_.applyBooleanBinaryOp(lvalue_.begin(), lvalue_.end(),
                                       rvalue_.begin(), rvalue_.end(), boolean_op::BinaryCount<op_type>());
        }else if((orient_ != orient_l) && (orient_!= orient_r)){ // both the orientations are not equal to input
          // easier way is to ignore the input orientation and use the input data's orientation, but not done so
          insert_into_view_arg(linput_, lvalue_, orient_l);
          insert_into_view_arg(rinput_, rvalue_, orient_r);
          output_.applyBooleanBinaryOp(linput_.begin(), linput_.end(),
                                       rinput_.begin(), rinput_.end(), boolean_op::BinaryCount<op_type>());
        }else if(orient_ != orient_l){ // left hand side orientation is different
          insert_into_view_arg(linput_, lvalue_, orient_l);
          rvalue_.sort();
          output_.applyBooleanBinaryOp(linput_.begin(), linput_.end(),
                                       rvalue_.begin(), rvalue_.end(), boolean_op::BinaryCount<op_type>());
        } else if(orient_ != orient_r){ // right hand side orientation is different
          insert_into_view_arg(rinput_, rvalue_, orient_r);
          lvalue_.sort();
          output_.applyBooleanBinaryOp(lvalue_.begin(), lvalue_.end(),
                                       rinput_.begin(), rinput_.end(), boolean_op::BinaryCount<op_type>());
        }
      }
    };

    template <typename value_type, typename lcoord, typename rtype, typename op_type>
    struct compute_90_set_value<value_type, polygon_90_set_data<lcoord>, rtype, op_type> {
      static
      void value(value_type& output_, const polygon_90_set_data<lcoord>& lvalue_,
                 const rtype& rvalue_, orientation_2d orient_) {
         value_type rinput_(orient_);
         lvalue_.sort();
         orientation_2d orient_r = polygon_90_set_traits<rtype>::orient(rvalue_);
         //std::cout << "compute_90_set_value-2 orientations (right, out):\t" << orient_r.to_int()
         //          << "," << orient_.to_int() << std::endl;
         insert_into_view_arg(rinput_, rvalue_, orient_r);
         output_.applyBooleanBinaryOp(lvalue_.begin(), lvalue_.end(),
                                      rinput_.begin(), rinput_.end(), boolean_op::BinaryCount<op_type>());
      }
    };

    template <typename value_type, typename ltype, typename rcoord, typename op_type>
    struct compute_90_set_value<value_type, ltype, polygon_90_set_data<rcoord>, op_type> {
      static
      void value(value_type& output_, const ltype& lvalue_,
                 const polygon_90_set_data<rcoord>& rvalue_, orientation_2d orient_) {
        value_type linput_(orient_);
        orientation_2d orient_l = polygon_90_set_traits<ltype>::orient(lvalue_);
        insert_into_view_arg(linput_, lvalue_, orient_l);
        rvalue_.sort();
        //std::cout << "compute_90_set_value-3 orientations (left, out):\t" << orient_l.to_int()
        //          << "," << orient_.to_int() << std::endl;

        output_.applyBooleanBinaryOp(linput_.begin(), linput_.end(),
                                     rvalue_.begin(), rvalue_.end(), boolean_op::BinaryCount<op_type>());
      }
    };

  template <typename ltype, typename rtype, typename op_type>
  class polygon_90_set_view {
  public:
    typedef typename polygon_90_set_traits<ltype>::coordinate_type coordinate_type;
    typedef polygon_90_set_data<coordinate_type> value_type;
    typedef typename value_type::iterator_type iterator_type;
    typedef polygon_90_set_view operator_arg_type;
  private:
    const ltype& lvalue_;
    const rtype& rvalue_;
    orientation_2d orient_;
    op_type op_;
    mutable value_type output_;
    mutable bool evaluated_;
    polygon_90_set_view& operator=(const polygon_90_set_view&);
  public:
    polygon_90_set_view(const ltype& lvalue,
                     const rtype& rvalue,
                     orientation_2d orient,
                     op_type op) :
      lvalue_(lvalue), rvalue_(rvalue), orient_(orient), op_(op), output_(orient), evaluated_(false) {}

    // get iterator to begin vertex data
  private:
    const value_type& value() const {
      if(!evaluated_) {
        evaluated_ = true;
        compute_90_set_value<value_type, ltype, rtype, op_type>::value(output_, lvalue_, rvalue_, orient_);
      }
      return output_;
    }
  public:
    iterator_type begin() const { return value().begin(); }
    iterator_type end() const { return value().end(); }

    orientation_2d orient() const { return orient_; }
    bool dirty() const { return false; } //result of a boolean is clean
    bool sorted() const { return true; } //result of a boolean is sorted

//     template <typename input_iterator_type>
//     void set(input_iterator_type input_begin, input_iterator_type input_end,
//              orientation_2d orient) const {
//       orient_ = orient;
//       output_.clear();
//       output_.insert(output_.end(), input_begin, input_end);
//       polygon_sort(output_.begin(), output_.end());
//     }
    void sort() const {} //is always sorted
  };

  template <typename ltype, typename rtype, typename op_type>
  struct geometry_concept<polygon_90_set_view<ltype, rtype, op_type> > {
    typedef polygon_90_set_concept type;
  };

  template <typename ltype, typename rtype, typename op_type>
  typename polygon_90_set_traits<polygon_90_set_view<ltype, rtype, op_type> >::iterator_type
  polygon_90_set_traits<polygon_90_set_view<ltype, rtype, op_type> >::
  begin(const polygon_90_set_view<ltype, rtype, op_type>& polygon_set) {
    return polygon_set.begin();
  }
  template <typename ltype, typename rtype, typename op_type>
  typename polygon_90_set_traits<polygon_90_set_view<ltype, rtype, op_type> >::iterator_type
  polygon_90_set_traits<polygon_90_set_view<ltype, rtype, op_type> >::
  end(const polygon_90_set_view<ltype, rtype, op_type>& polygon_set) {
    return polygon_set.end();
  }
//   template <typename ltype, typename rtype, typename op_type>
//   template <typename input_iterator_type>
//   void polygon_90_set_traits<polygon_90_set_view<ltype, rtype, op_type> >::
//   set(polygon_90_set_view<ltype, rtype, op_type>& polygon_set,
//       input_iterator_type input_begin, input_iterator_type input_end,
//       orientation_2d orient) {
//     polygon_set.set(input_begin, input_end, orient);
//   }
  template <typename ltype, typename rtype, typename op_type>
  orientation_2d polygon_90_set_traits<polygon_90_set_view<ltype, rtype, op_type> >::
  orient(const polygon_90_set_view<ltype, rtype, op_type>& polygon_set) {
    return polygon_set.orient(); }
  template <typename ltype, typename rtype, typename op_type>
  bool polygon_90_set_traits<polygon_90_set_view<ltype, rtype, op_type> >::
  clean(const polygon_90_set_view<ltype, rtype, op_type>& polygon_set) {
    return !polygon_set.dirty(); }
  template <typename ltype, typename rtype, typename op_type>
  bool polygon_90_set_traits<polygon_90_set_view<ltype, rtype, op_type> >::
  sorted(const polygon_90_set_view<ltype, rtype, op_type>& polygon_set) {
    return polygon_set.sorted(); }

  template <typename value_type, typename arg_type>
  inline void insert_into_view_arg(value_type& dest, const arg_type& arg, orientation_2d orient) {
    typedef typename polygon_90_set_traits<arg_type>::iterator_type literator;
    literator itr1, itr2;
    itr1 = polygon_90_set_traits<arg_type>::begin(arg);
    itr2 = polygon_90_set_traits<arg_type>::end(arg);
    dest.insert(itr1, itr2, orient);
    dest.sort();
  }

  template <typename T>
  template <typename ltype, typename rtype, typename op_type>
  inline polygon_90_set_data<T>& polygon_90_set_data<T>::operator=(const polygon_90_set_view<ltype, rtype, op_type>& that) {
    set(that.begin(), that.end(), that.orient());
    dirty_ = false;
    unsorted_ = false;
    return *this;
  }

  template <typename T>
  template <typename ltype, typename rtype, typename op_type>
  inline polygon_90_set_data<T>::polygon_90_set_data(const polygon_90_set_view<ltype, rtype, op_type>& that) :
    orient_(that.orient()), data_(that.begin(), that.end()), dirty_(false), unsorted_(false) {}

  template <typename geometry_type_1, typename geometry_type_2>
  struct self_assign_operator_lvalue {
    typedef geometry_type_1& type;
  };

  template <typename type_1, typename type_2>
  struct by_value_binary_operator {
    typedef type_1 type;
  };

    template <typename geometry_type_1, typename geometry_type_2, typename op_type>
    geometry_type_1& self_assignment_boolean_op(geometry_type_1& lvalue_, const geometry_type_2& rvalue_) {
      typedef geometry_type_1 ltype;
      typedef geometry_type_2 rtype;
      typedef typename polygon_90_set_traits<ltype>::coordinate_type coordinate_type;
      typedef polygon_90_set_data<coordinate_type> value_type;
      orientation_2d orient_ = polygon_90_set_traits<ltype>::orient(lvalue_);
      //BM: rvalue_ data set may have its own orientation for scanline
      orientation_2d orient_r = polygon_90_set_traits<rtype>::orient(rvalue_);
      //std::cout << "self-assignment boolean-op (left, right, out):\t" << orient_.to_int()
      //          << "," << orient_r.to_int() << "," << orient_.to_int() << std::endl;
      value_type linput_(orient_);
      // BM: the rinput_ set's (that stores the rvalue_ dataset  polygons) scanline orientation is *forced*
      // to be same as linput
      value_type rinput_(orient_);
      //BM: The output dataset's scanline orient is set as equal to first input dataset's (lvalue_) orientation
      value_type output_(orient_);
      insert_into_view_arg(linput_, lvalue_, orient_);
      // BM: The last argument orient_r is the user initialized scanline orientation for rvalue_ data set.
      // But since rinput (see above) is initialized to scanline orientation consistent with the lvalue_
      // data set, this insertion operation will change the incoming rvalue_ dataset's scanline orientation
      insert_into_view_arg(rinput_, rvalue_, orient_r);
      // BM: boolean operation and output uses lvalue_ dataset's scanline orientation.
      output_.applyBooleanBinaryOp(linput_.begin(), linput_.end(),
                                   rinput_.begin(), rinput_.end(), boolean_op::BinaryCount<op_type>());
      polygon_90_set_mutable_traits<geometry_type_1>::set(lvalue_, output_.begin(), output_.end(), orient_);
      return lvalue_;
    }

  namespace operators {
  struct y_ps90_b : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3< y_ps90_b,
    typename is_polygon_90_set_type<geometry_type_1>::type,
    typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryOr> >::type
  operator|(const geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryOr>
      (lvalue, rvalue,
       polygon_90_set_traits<geometry_type_1>::orient(lvalue),
       boolean_op::BinaryOr());
  }

  struct y_ps90_p : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if<
    typename gtl_and_3< y_ps90_p,
      typename gtl_if<typename is_polygon_90_set_type<geometry_type_1>::type>::type,
      typename gtl_if<typename is_polygon_90_set_type<geometry_type_2>::type>::type>::type,
    polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryOr> >::type
  operator+(const geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryOr>
      (lvalue, rvalue,
       polygon_90_set_traits<geometry_type_1>::orient(lvalue),
       boolean_op::BinaryOr());
  }

  struct y_ps90_s : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3< y_ps90_s,
    typename is_polygon_90_set_type<geometry_type_1>::type,
    typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryAnd> >::type
  operator*(const geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryAnd>
      (lvalue, rvalue,
       polygon_90_set_traits<geometry_type_1>::orient(lvalue),
       boolean_op::BinaryAnd());
  }

  struct y_ps90_a : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3< y_ps90_a,
    typename is_polygon_90_set_type<geometry_type_1>::type,
    typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryAnd> >::type
  operator&(const geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryAnd>
      (lvalue, rvalue,
       polygon_90_set_traits<geometry_type_1>::orient(lvalue),
       boolean_op::BinaryAnd());
  }

  struct y_ps90_x : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3< y_ps90_x,
    typename is_polygon_90_set_type<geometry_type_1>::type,
    typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryXor> >::type
  operator^(const geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryXor>
      (lvalue, rvalue,
       polygon_90_set_traits<geometry_type_1>::orient(lvalue),
       boolean_op::BinaryXor());
  }

  struct y_ps90_m : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3< y_ps90_m,
    typename gtl_if<typename is_polygon_90_set_type<geometry_type_1>::type>::type,
    typename gtl_if<typename is_polygon_90_set_type<geometry_type_2>::type>::type>::type,
                       polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryNot> >::type
  operator-(const geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return polygon_90_set_view<geometry_type_1, geometry_type_2, boolean_op::BinaryNot>
      (lvalue, rvalue,
       polygon_90_set_traits<geometry_type_1>::orient(lvalue),
       boolean_op::BinaryNot());
  }

  struct y_ps90_pe : gtl_yes {};

  template <typename coordinate_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and< y_ps90_pe, typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       polygon_90_set_data<coordinate_type_1> >::type &
  operator+=(polygon_90_set_data<coordinate_type_1>& lvalue, const geometry_type_2& rvalue) {
    lvalue.insert(polygon_90_set_traits<geometry_type_2>::begin(rvalue), polygon_90_set_traits<geometry_type_2>::end(rvalue),
                  polygon_90_set_traits<geometry_type_2>::orient(rvalue));
    return lvalue;
  }

  struct y_ps90_be : gtl_yes {};
  //
  template <typename coordinate_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and< y_ps90_be, typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       polygon_90_set_data<coordinate_type_1> >::type &
  operator|=(polygon_90_set_data<coordinate_type_1>& lvalue, const geometry_type_2& rvalue) {
    return lvalue += rvalue;
  }

  struct y_ps90_pe2 : gtl_yes {};

  //normal self assignment boolean operations
  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3< y_ps90_pe2, typename is_mutable_polygon_90_set_type<geometry_type_1>::type,
                                         typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       geometry_type_1>::type &
  operator+=(geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return self_assignment_boolean_op<geometry_type_1, geometry_type_2, boolean_op::BinaryOr>(lvalue, rvalue);
  }

  struct y_ps90_be2 : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3<y_ps90_be2, typename is_mutable_polygon_90_set_type<geometry_type_1>::type,
                                         typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       geometry_type_1>::type &
  operator|=(geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return self_assignment_boolean_op<geometry_type_1, geometry_type_2, boolean_op::BinaryOr>(lvalue, rvalue);
  }

  struct y_ps90_se : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3<y_ps90_se, typename is_mutable_polygon_90_set_type<geometry_type_1>::type,
                                         typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       geometry_type_1>::type &
  operator*=(geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return self_assignment_boolean_op<geometry_type_1, geometry_type_2, boolean_op::BinaryAnd>(lvalue, rvalue);
  }

  struct y_ps90_ae : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3<y_ps90_ae, typename is_mutable_polygon_90_set_type<geometry_type_1>::type,
                                         typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       geometry_type_1>::type &
  operator&=(geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return self_assignment_boolean_op<geometry_type_1, geometry_type_2, boolean_op::BinaryAnd>(lvalue, rvalue);
  }

  struct y_ps90_xe : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3<y_ps90_xe, typename is_mutable_polygon_90_set_type<geometry_type_1>::type,
                                         typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       geometry_type_1>::type &
  operator^=(geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return self_assignment_boolean_op<geometry_type_1, geometry_type_2, boolean_op::BinaryXor>(lvalue, rvalue);
  }

  struct y_ps90_me : gtl_yes {};

  template <typename geometry_type_1, typename geometry_type_2>
  typename enable_if< typename gtl_and_3< y_ps90_me, typename is_mutable_polygon_90_set_type<geometry_type_1>::type,
                                         typename is_polygon_90_set_type<geometry_type_2>::type>::type,
                       geometry_type_1>::type &
  operator-=(geometry_type_1& lvalue, const geometry_type_2& rvalue) {
    return self_assignment_boolean_op<geometry_type_1, geometry_type_2, boolean_op::BinaryNot>(lvalue, rvalue);
  }

  struct y_ps90_rpe : gtl_yes {};

  template <typename geometry_type_1, typename coordinate_type_1>
  typename enable_if< typename gtl_and_3<y_ps90_rpe,
    typename is_mutable_polygon_90_set_type<geometry_type_1>::type,
    typename gtl_same_type<typename geometry_concept<coordinate_type_1>::type, coordinate_concept>::type>::type,
                       geometry_type_1>::type &
  operator+=(geometry_type_1& lvalue, coordinate_type_1 rvalue) {
    return resize(lvalue, rvalue);
  }

  struct y_ps90_rme : gtl_yes {};

  template <typename geometry_type_1, typename coordinate_type_1>
  typename enable_if< typename gtl_and_3<y_ps90_rme,
    typename is_mutable_polygon_90_set_type<geometry_type_1>::type,
    typename gtl_same_type<typename geometry_concept<coordinate_type_1>::type, coordinate_concept>::type>::type,
                       geometry_type_1>::type &
  operator-=(geometry_type_1& lvalue, coordinate_type_1 rvalue) {
    return resize(lvalue, -rvalue);
  }

  struct y_ps90_rp : gtl_yes {};

  template <typename geometry_type_1, typename coordinate_type_1>
  typename enable_if< typename gtl_and_3<y_ps90_rp,
    typename gtl_if<typename is_mutable_polygon_90_set_type<geometry_type_1>::type>::type,
    typename gtl_if<typename gtl_same_type<typename geometry_concept<coordinate_type_1>::type, coordinate_concept>::type>::type>::type,
  geometry_type_1>::type
  operator+(const geometry_type_1& lvalue, coordinate_type_1 rvalue) {
    geometry_type_1 retval(lvalue);
    retval += rvalue;
    return retval;
  }

  struct y_ps90_rm : gtl_yes {};

  template <typename geometry_type_1, typename coordinate_type_1>
  typename enable_if< typename gtl_and_3<y_ps90_rm,
    typename gtl_if<typename is_mutable_polygon_90_set_type<geometry_type_1>::type>::type,
    typename gtl_if<typename gtl_same_type<typename geometry_concept<coordinate_type_1>::type, coordinate_concept>::type>::type>::type,
  geometry_type_1>::type
  operator-(const geometry_type_1& lvalue, coordinate_type_1 rvalue) {
    geometry_type_1 retval(lvalue);
    retval -= rvalue;
    return retval;
  }
  }
}
}
#endif
