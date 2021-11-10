//  (C) Copyright John Maddock 2006.
//  (C) Copyright Paul A. Bristow 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STATS_COMPLEMENT_HPP
#define BOOST_STATS_COMPLEMENT_HPP

//
// This code really defines our own tuple type.
// It would be nice to reuse boost::math::tuple
// while retaining our own type safety, but it's
// not clear if that's possible.  In any case this
// code is *very* lightweight.
//
namespace boost{ namespace math{

template <class Dist, class RealType>
struct complemented2_type
{
   complemented2_type(
      const Dist& d, 
      const RealType& p1)
      : dist(d), 
        param(p1) {}

   const Dist& dist;
   const RealType& param;

private:
   complemented2_type& operator=(const complemented2_type&);
};

template <class Dist, class RealType1, class RealType2>
struct complemented3_type
{
   complemented3_type(
      const Dist& d, 
      const RealType1& p1,
      const RealType2& p2)
      : dist(d), 
        param1(p1), 
        param2(p2) {}

   const Dist& dist;
   const RealType1& param1;
   const RealType2& param2;
private:
   complemented3_type& operator=(const complemented3_type&);
};

template <class Dist, class RealType1, class RealType2, class RealType3>
struct complemented4_type
{
   complemented4_type(
      const Dist& d, 
      const RealType1& p1,
      const RealType2& p2,
      const RealType3& p3)
      : dist(d), 
        param1(p1), 
        param2(p2), 
        param3(p3) {}

   const Dist& dist;
   const RealType1& param1;
   const RealType2& param2;
   const RealType3& param3;
private:
   complemented4_type& operator=(const complemented4_type&);
};

template <class Dist, class RealType1, class RealType2, class RealType3, class RealType4>
struct complemented5_type
{
   complemented5_type(
      const Dist& d, 
      const RealType1& p1,
      const RealType2& p2,
      const RealType3& p3,
      const RealType4& p4)
      : dist(d), 
        param1(p1), 
        param2(p2), 
        param3(p3), 
        param4(p4) {}

   const Dist& dist;
   const RealType1& param1;
   const RealType2& param2;
   const RealType3& param3;
   const RealType4& param4;
private:
   complemented5_type& operator=(const complemented5_type&);
};

template <class Dist, class RealType1, class RealType2, class RealType3, class RealType4, class RealType5>
struct complemented6_type
{
   complemented6_type(
      const Dist& d, 
      const RealType1& p1,
      const RealType2& p2,
      const RealType3& p3,
      const RealType4& p4,
      const RealType5& p5)
      : dist(d), 
        param1(p1), 
        param2(p2), 
        param3(p3), 
        param4(p4), 
        param5(p5) {}

   const Dist& dist;
   const RealType1& param1;
   const RealType2& param2;
   const RealType3& param3;
   const RealType4& param4;
   const RealType5& param5;
private:
   complemented6_type& operator=(const complemented6_type&);
};

template <class Dist, class RealType1, class RealType2, class RealType3, class RealType4, class RealType5, class RealType6>
struct complemented7_type
{
   complemented7_type(
      const Dist& d, 
      const RealType1& p1,
      const RealType2& p2,
      const RealType3& p3,
      const RealType4& p4,
      const RealType5& p5,
      const RealType6& p6)
      : dist(d), 
        param1(p1), 
        param2(p2), 
        param3(p3), 
        param4(p4), 
        param5(p5), 
        param6(p6) {}

   const Dist& dist;
   const RealType1& param1;
   const RealType2& param2;
   const RealType3& param3;
   const RealType4& param4;
   const RealType5& param5;
   const RealType6& param6;
private:
   complemented7_type& operator=(const complemented7_type&);
};

template <class Dist, class RealType>
inline complemented2_type<Dist, RealType> complement(const Dist& d, const RealType& r)
{
   return complemented2_type<Dist, RealType>(d, r);
}

template <class Dist, class RealType1, class RealType2>
inline complemented3_type<Dist, RealType1, RealType2> complement(const Dist& d, const RealType1& r1, const RealType2& r2)
{
   return complemented3_type<Dist, RealType1, RealType2>(d, r1, r2);
}

template <class Dist, class RealType1, class RealType2, class RealType3>
inline complemented4_type<Dist, RealType1, RealType2, RealType3> complement(const Dist& d, const RealType1& r1, const RealType2& r2, const RealType3& r3)
{
   return complemented4_type<Dist, RealType1, RealType2, RealType3>(d, r1, r2, r3);
}

template <class Dist, class RealType1, class RealType2, class RealType3, class RealType4>
inline complemented5_type<Dist, RealType1, RealType2, RealType3, RealType4> complement(const Dist& d, const RealType1& r1, const RealType2& r2, const RealType3& r3, const RealType4& r4)
{
   return complemented5_type<Dist, RealType1, RealType2, RealType3, RealType4>(d, r1, r2, r3, r4);
}

template <class Dist, class RealType1, class RealType2, class RealType3, class RealType4, class RealType5>
inline complemented6_type<Dist, RealType1, RealType2, RealType3, RealType4, RealType5> complement(const Dist& d, const RealType1& r1, const RealType2& r2, const RealType3& r3, const RealType4& r4, const RealType5& r5)
{
   return complemented6_type<Dist, RealType1, RealType2, RealType3, RealType4, RealType5>(d, r1, r2, r3, r4, r5);
}

template <class Dist, class RealType1, class RealType2, class RealType3, class RealType4, class RealType5, class RealType6>
inline complemented7_type<Dist, RealType1, RealType2, RealType3, RealType4, RealType5, RealType6> complement(const Dist& d, const RealType1& r1, const RealType2& r2, const RealType3& r3, const RealType4& r4, const RealType5& r5, const RealType6& r6)
{
   return complemented7_type<Dist, RealType1, RealType2, RealType3, RealType4, RealType5, RealType6>(d, r1, r2, r3, r4, r5, r6);
}

} // namespace math
} // namespace boost

#endif // BOOST_STATS_COMPLEMENT_HPP

