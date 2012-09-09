
/**
 * @file mace/reflect/reflect.hpp
 *
 * @brief Defines types and macros used to provide reflection.
 *
 */
#ifndef _FC_STATIC_REFLECT_HPP_
#define _FC_STATIC_REFLECT_HPP_

#include <boost/static_assert.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <string>
#include <typeinfo>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stdint.h>
#include <boost/fusion/container/vector.hpp>
#include <boost/function_types/result_type.hpp>

//#include <mace/void.hpp>
//#include <mace/reflect/typeinfo.hpp>

namespace mace { 
/**
 *  @brief types, methods associated with the MACE.Reflect Library
 */
namespace reflect {

/**
 *  @brief defines visit functions for T
 *  Unless this is specialized, visit() will not be defined for T.
 *
 *  @tparam T - the type that will be visited.
 *
 *  The @ref FC_STATIC_REFLECT(TYPE,MEMBERS) or FC_STATIC_REFLECT_DERIVED(TYPE,BASES,MEMBERS) macro is used to specialize this
 *  class for your type.
 */
template<typename T>
struct reflector{
    typedef T type;
    typedef boost::fusion::vector<> base_class_types;
    typedef boost::false_type is_defined;
    typedef boost::false_type is_enum; 

    /**
     *  @tparam Visitor a function object of the form:
     *    
     *    @code
     *     struct functor {  
     *        template<typename MemberPtr, MemberPtr m>
     *        void operator()( const char* name )const;
     *     };
     *    @endcode
     *
     *  If T is an enum then the functor has the following form:
     *    @code
     *     struct functor {  
     *        template<int Value>
     *        void operator()( const char* name )const;
     *     };
     *    @endcode
     *  
     *  @param v a functor that will be called for each member on T
     *
     *  @note - this method is not defined for non-reflected types.
     */
    #ifdef DOXYGEN
    template<typename Visitor>
    static inline void visit( const Visitor& v ); 
    #endif // DOXYGEN
};

} } // namespace mace::reflect


#ifndef DOXYGEN

#define FC_STATIC_REFLECT_VISIT_BASE(r, visitor, base) \
  fc::static_reflector<base>::visit( visitor );


#ifndef WIN32
  #define TEMPLATE template
#else
  #define TEMPLATE
#endif

#define FC_STATIC_REFLECT_VISIT_MEMBER( r, visitor, elem ) \
  visitor.TEMPLATE operator()<BOOST_TYPEOF(&type::elem), &type::elem>( BOOST_PP_STRINGIZE(elem) );


#define FC_STATIC_REFLECT_BASE_MEMBER_COUNT( r, OP, elem ) \
  OP fc::static_reflector<elem>::member_count

#define FC_STATIC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
template<typename Visitor>\
static inline void visit( const Visitor& v ) { \
    BOOST_PP_SEQ_FOR_EACH( FC_STATIC_REFLECT_VISIT_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( FC_STATIC_REFLECT_VISIT_MEMBER, v, MEMBERS ) \
} 

#define FC_STATIC_REFLECT_DERIVED_IMPL_EXT( TYPE, INHERITS, MEMBERS ) \
template<typename Visitor>\
void fc::static_reflector<TYPE>::visit( const Visitor& v ) { \
    BOOST_PP_SEQ_FOR_EACH( FC_STATIC_REFLECT_VISIT_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( FC_STATIC_REFLECT_VISIT_MEMBER, v, MEMBERS ) \
} 

#endif // DOXYGEN


#define FC_STATIC_REFLECT_VISIT_ENUM( r, visitor, elem ) \
  visitor.TEMPLATE operator()<elem>(BOOST_PP_STRINGIZE(elem));
#define FC_STATIC_REFLECT_ENUM_TO_STRING( r, visitor, elem ) \
  case elem: return BOOST_PP_STRINGIZE(elem);

#define FC_STATIC_REFLECT_ENUM_FROM_STRING( r, visitor, elem ) \
  if( strcmp( s, BOOST_PP_STRINGIZE(elem)  ) == 0 ) return elem;

#define FC_STATIC_REFLECT_ENUM( ENUM, FIELDS ) \
FC_STATIC_REFLECT_TYPEINFO(ENUM) \
namespace mace { namespace reflect { \
template<> struct reflector<ENUM> { \
    typedef boost::true_type is_defined; \
    typedef boost::true_type is_enum; \
    typedef boost::fusion::vector<> base_class_types; \
    template<typename Visitor> \
    static inline void visit( const Visitor& v ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_STATIC_REFLECT_VISIT_ENUM, v, FIELDS ) \
    }\
    static const char* to_string(int64_t i) { \
      switch( ENUM(i) ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_STATIC_REFLECT_ENUM_TO_STRING, v, FIELDS ) \
        default: \
        FC_STATIC_REFLECT_THROW( mace::reflect::unknown_field(), "%1% not in enum '%2%'", %i %BOOST_PP_STRINGIZE(ENUM) ); \
      }\
    } \
    static ENUM from_string( const char* s ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_STATIC_REFLECT_ENUM_FROM_STRING, v, FIELDS ) \
        FC_STATIC_REFLECT_THROW( mace::reflect::unknown_field(), "%1% in enum %2%", %s %BOOST_PP_STRINGIZE(ENUM) ); \
    } \
};  \
} }



/**
 *  @def FC_STATIC_REFLECT_DERIVED(TYPE,INHERITS,MEMBERS)
 *
 *  @brief Specializes fc::static_reflector for TYPE where 
 *         type inherits other reflected classes
 *
 *  @param INHERITS - a sequence of base class names (basea)(baseb)(basec)
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define FC_STATIC_REFLECT_DERIVED( TYPE, INHERITS, MEMBERS ) \
FC_STATIC_REFLECT_TYPEINFO(TYPE) \
namespace fc {  \
template<> struct static_reflector<TYPE> {\
    typedef TYPE type; \
    typedef boost::true_type  is_defined; \
    typedef boost::false_type is_enum;  \
    enum  member_count_enum {  \
      local_member_count = BOOST_PP_SEQ_SIZE(MEMBERS), \
      total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( FC_STATIC_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
    }; \
    FC_STATIC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; } 


/**
 *  @def FC_STATIC_REFLECT(TYPE,MEMBERS)
 *  @brief Specializes fc::static_reflector for TYPE
 *
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 *
 *  @see FC_STATIC_REFLECT_DERIVED
 */
#define FC_STATIC_REFLECT( TYPE, MEMBERS ) \
    FC_STATIC_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define FC_STATIC_REFLECT_FWD( TYPE ) \
FC_STATIC_REFLECT_TYPEINFO(TYPE) \
namespace mace { namespace reflect { \
template<> struct static_reflector<TYPE> {\
    typedef TYPE type; \
    typedef boost::true_type is_defined; \
    enum  member_count_enum {  \
      local_member_count = BOOST_PP_SEQ_SIZE(MEMBERS), \
      total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( FC_STATIC_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
    }; \
    typedef boost::fusion::vector<BOOST_PP_SEQ_ENUM(INHERITS)> base_class_types; \
    template<typename Visitor> static void visit( const Visitor& v ); \
}; } }


#define FC_STATIC_REFLECT_DERIVED_IMPL( TYPE, MEMBERS ) \
    FC_STATIC_REFLECT_IMPL_DERIVED_EXT( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define FC_STATIC_REFLECT_IMPL( TYPE, MEMBERS ) \
    FC_STATIC_REFLECT_DERIVED_IMPL_EXT( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )



#endif