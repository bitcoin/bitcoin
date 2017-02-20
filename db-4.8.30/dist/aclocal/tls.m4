# Check for thread local storage support.
# Required when building with DB STL support.
# Based in part on: http://autoconf-archive.cryp.to/ax_tls.html
# by Alan Woodland <ajw05@aber.ac.uk>  

AC_DEFUN([AX_TLS], [
  AC_MSG_CHECKING(for thread local storage (TLS) class)
  AC_SUBST(TLS_decl)
  AC_SUBST(TLS_defn)
  ac_cv_tls=none
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ax_tls_keywords="__thread __declspec(thread) __declspec(__thread)"
  for ax_tls_decl_keyword in $ax_tls_keywords ""; do
      for ax_tls_defn_keyword in $ax_tls_keywords ""; do
          test -z "$ax_tls_decl_keyword" &&
              test -z "$ax_tls_defn_keyword" && continue
          AC_TRY_COMPILE([template <typename T>class TLSClass {
              public: static ] $ax_tls_decl_keyword [ T *tlsvar;
              };
              class TLSClass2 {
              public: static ] $ax_tls_decl_keyword [int tlsvar;
              };
              template<typename T> ] $ax_tls_defn_keyword [ T* TLSClass<T>::tlsvar = NULL;]
              $ax_tls_defn_keyword [int TLSClass2::tlsvar = 1; 
              static $ax_tls_decl_keyword int x = 0;],
              [TLSClass<int>::tlsvar = NULL; TLSClass2::tlsvar = 1;],
              [ac_cv_tls=modifier ; break])
      done
      test "$ac_cv_tls" = none || break
  done
  AC_LANG_RESTORE
  if test "$ac_cv_tls" = "none" ; then
      AC_TRY_COMPILE(
        [#include <stdlib.h>
         #include <pthread.h>

         static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
         static pthread_key_t key;

         static void init_once(void) {
             pthread_key_create(&key, NULL);
         }
         static void *get_tls() {
             return (void *)pthread_getspecific(&key);
         }
         static void set_tls(void *p) {
              pthread_setspecific(&key, p);
         }], [],
         [ac_cv_tls=pthread])
  fi

  case "$ac_cv_tls" in
  none) break ;;
  pthread)
      TLS_decl="#define	HAVE_PTHREAD_TLS"
      TLS_defn="" ;;
  modifier)
      TLS_decl="#define	TLS_DECL_MODIFIER	$ax_tls_decl_keyword"
      TLS_defn="#define	TLS_DEFN_MODIFIER	$ax_tls_defn_keyword" ;;
  esac

  AC_MSG_RESULT([$ac_cv_tls])
])

