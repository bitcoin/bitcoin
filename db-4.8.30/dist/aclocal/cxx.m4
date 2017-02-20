# C++ language checks

AC_DEFUN(AC_CXX_STDHEADERS, [
AC_SUBST(cxx_have_stdheaders)
AC_MSG_CHECKING(whether C++ supports the ISO C++ standard includes)
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILE([#include <iostream>],[std::ostream *o; return 0;],
	db_cv_cxx_have_stdheaders=yes, db_cv_cxx_have_stdheaders=no)
AC_LANG_RESTORE
AC_MSG_RESULT($db_cv_cxx_have_stdheaders)
if test "$db_cv_cxx_have_stdheaders" = yes; then
	cxx_have_stdheaders="#define	HAVE_CXX_STDHEADERS 1"
fi])

AC_DEFUN(AC_CXX_WSTRING, [
AC_MSG_CHECKING(whether C++ supports the wstring class)
AC_SUBST(WSTRING_decl)
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_LINK_IFELSE(AC_LANG_PROGRAM([#include <string>
	using std::wstring;],
	[wstring ws; ws.find_first_of(ws);]),
	[WSTRING_decl="#define	HAVE_WSTRING 1" ;
	 AC_MSG_RESULT(yes)],
	[WSTRING_decl="#undef	HAVE_WSTRING" ;
	 AC_MSG_RESULT(no)])
AC_LANG_RESTORE
])

AC_DEFUN(AC_CXX_SUPPORTS_TEMPLATES, [
AC_MSG_CHECKING(whether the C++ compiler supports templates for STL)
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILE([#include <iostream>
#include <string>
#include <vector>

using std::string;
using std::vector;
namespace dbstl_configure_test {

template<typename T1, typename T2 = int>
class MyClass
{
public:
	explicit MyClass(int i) { imem = i;}

	MyClass(const T1& t1, const T2& t2, int i)
	{
		mem1 = t1;
		mem2 = t2;
		imem = i;
	}

	template <typename T3>
	T2 templ_mem_func(T1 t1, T3 t3)
	{
		mem2 = t1;
		T3 t32 = t3;
		T2 t2;
		return t2;
	}

	double templ_mem_func(T1 t1, double t3)
	{
		mem1 = t1;
		double t32 = t3;
		return t3;
	}

	template <typename ReturnType, typename T7, typename T8>
	ReturnType templ_mem_func(T7, T8);

	operator T1() const {return mem1;}
private:
	T1 mem1;
	T2 mem2;
	int imem;
};

template<typename T1, typename T2>
template <typename ReturnType, typename T7, typename T8>
ReturnType MyClass<T1, T2>::templ_mem_func(T7, T8)
{
	ReturnType rt;
	return rt;
}

template<>
class MyClass<double, float>
{
public:
	explicit MyClass(int i) { imem = i;}

	MyClass(const double& t1, const float& t2, int i)
	{
		mem1 = t1;
		mem2 = t2;
		imem = i;
	}

	template <typename T3>
	float templ_mem_func(double t1, T3 t3)
	{
		mem2 = t1;
		T3 t32 = t3;
		float t2;
		return t2;
	}

	double templ_mem_func(double t1, double t3)
	{
		mem1 = t1;
		double t32 = t3;
		return t3;
	}

	template <typename ReturnType, typename T7, typename T8>
	ReturnType templ_mem_func(T7, T8);

	operator double() const {return mem1;}
private:
	double mem1;
	float mem2;
	int imem;
};

template <typename ReturnType, typename T7, typename T8>
ReturnType MyClass<double, float>::templ_mem_func(T7, T8)
{
	ReturnType rt;
	return rt;
}

template <typename T1, typename T2> 
class MyClass2 { 
public:
	MyClass2(const T1& t1, const T2&t2){}
}; 

// partial specialization: both template parameters have same type 
template <typename T> 
class MyClass2<T,T> { 
public:
	MyClass2(const T& t1, const T&t2){}
}; 

// partial specialization: second type is int 
template <typename T> 
class MyClass2<T,int> { 
public:
	MyClass2(const T& t1, const int&t2){}
}; 

// partial specialization: both template parameters are pointer types 
template <typename T1, typename T2> 
class MyClass2<T1*,T2*> { 
public:
	MyClass2(const T1* t1, const T2*t2){}
}; 

template <typename T> 
class MyClass2<T*,T*> { 
public:
	MyClass2(const T* t1, const T*t2){}
}; 

template <typename T4, typename T5>
int part_spec_func(T4 t4, T5 t5)
{
	// Zero Initialization should work.
	T4 t44 = T4();
	T5 t55 = T5();

	t44 = t4;
	t55 = t5;
}

template <typename T4>
int part_spec_func(T4 t4, std::vector<T4> t55)
{
	T4 t44 = t4;
	std::vector<T4> abc = t55;
}

// maximum of two int values 
inline int const& max (int const& a, int const& b) 
{ 
    return a<b?b:a; 
} 

// maximum of two values of any type 
template <typename T1, typename T2> 
inline T2 const max (T1 const& a, T2 const& b) 
{ 
    return a<b?b:a; 
} 
// maximum of two values of any type 
template <typename T> 
inline T const& max (T const& a, T const& b) 
{ 
    return a<b?b:a; 
} 

// maximum of three values of any type 
template <typename T> 
inline T const& max (T const& a, T const& b, T const& c) 
{ 
    return max (max(a,b), c); 
}

template <typename T> 
class Base { 
public: 
	void exit2(){}
	Base(){}
};

template <typename T> 
class Derived : public Base<T> {
public: 
	// Call Base<T>() explicitly here, otherwise can't access it. 
	// Kind of like this->.
	Derived() : Base<T>(){}

	void foo() {
        	this->exit2();
	} 
}; 

} // dbstl_configure_test

using namespace dbstl_configure_test;], [
	char cc = 'a';
	int i = 4;
	double pi = 3.14;
	float gold = 0.618;

	MyClass2<int,float> mif(i, gold);	// uses MyClass2<T1,T2> 
	MyClass2<float,float> mff(gold, gold);	// uses MyClass2<T,T> 
	MyClass2<float,int> mfi(gold, i);	// uses MyClass2<T,int> 
	MyClass2<int*,float*> mp(&i, &gold);	// uses MyClass2<T1*,T2*> 
	MyClass2<int*,int*> m(&i, &i);		// uses MyClass2<T*, T*>

	MyClass<char> obj1(i);
	obj1.templ_mem_func(cc, pi);
	obj1.templ_mem_func(cc, gold);
	obj1.templ_mem_func(i, pi);
	obj1.templ_mem_func(cc, cc);
	char ch = (char)obj1;

	string str1("abc"), str2("def");
	MyClass<const char*, std::string> obj2(str1.c_str(), str2, i);
	obj2.templ_mem_func("klm", str2);
	obj2.templ_mem_func("hij", pi);
	
	// Use j to help distinguish, otherwise unable to use the one defined
	// outside of class body.
	int j = obj2.templ_mem_func<int, char, char>(cc, cc); 
	// Call explicitly.
	obj2.templ_mem_func<int, float, double>(gold, pi);
	const char *pch = (const char*)obj2;

	MyClass<double, float> obj3(pi, gold, i);
	obj3.templ_mem_func(pi, i);
	obj3.templ_mem_func(pi, str1);
	obj3.templ_mem_func(pi, pi);
	obj3.templ_mem_func(cc, pi);
	obj3.templ_mem_func(cc, cc);
	double tmpd = (double)obj3;

	MyClass<double, float> obj4(i);
	obj4.templ_mem_func(pi, i);
	obj4.templ_mem_func(pi, str1);
	obj4.templ_mem_func(pi, pi);
	obj4.templ_mem_func(gold, pi);
	tmpd = (double)obj4;

	// Function template partial specialization.
	part_spec_func(pi, gold);
	part_spec_func(gold, i);
	part_spec_func(str1, str2);
	std::vector<std::string> strv;
	part_spec_func(str1, strv);
	std::vector<double> dblv;
	part_spec_func(pi, dblv);

	// Function template overloads and explicit call and deduction.
	dbstl_configure_test::max(7, 42, 68);     // calls the template for three arguments 
	dbstl_configure_test::max(7.0, 42.0);     // calls max<double> (by argument deduction) 
	dbstl_configure_test::max('a', 'b');      // calls max<char> (by argument deduction) 
	dbstl_configure_test::max(7, 42.0);         
	dbstl_configure_test::max<double>(4,4.2); // instantiate T as double
	dbstl_configure_test::max(7, 42);         // calls the nontemplate for two ints 
	dbstl_configure_test::max<>(7, 42);       // calls max<int> (by argument deduction) 
	dbstl_configure_test::max<double, double>(7, 42); // calls max<double> (no argument deduction) 
	dbstl_configure_test::max('a', 42.7);     // calls the nontemplate for two ints 
	
	Base<double> bobj;
	bobj.exit2();
	// Using this-> to access base class members.
	Derived<double> dobj;
	dobj.foo();
	dobj.exit2();
], AC_MSG_RESULT(yes), AC_MSG_ERROR(no))
AC_LANG_RESTORE
])
