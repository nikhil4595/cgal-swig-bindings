#ifndef CGAL_SWIG_JAVA_INPUT_ITERATOR_WRAPPER_H
#define CGAL_SWIG_JAVA_INPUT_ITERATOR_WRAPPER_H

#include <boost/iterator/iterator_facade.hpp>
#include <SWIG_CGAL/Java/global_functions.h>

#ifndef SWIG
struct Ref_counted_jdata{
  jobject jiterator; 
  jclass it_class,pt_class;
  int* counter;
  
  Ref_counted_jdata():jiterator(NULL),counter(new int(1)){}
  
  Ref_counted_jdata(jobject jit):counter(new int(1))
  {
    jiterator=JNU_GetEnv()->NewGlobalRef(jit);
    it_class=(jclass) JNU_GetEnv()->NewGlobalRef( JNU_GetEnv()->GetObjectClass(jiterator) );
  }
  
  void cleanup()
  {
    if (--(*counter) == 0)
    {
      delete counter;
      if (jiterator!=NULL){
        JNU_GetEnv()->DeleteGlobalRef(jiterator);
        JNU_GetEnv()->DeleteGlobalRef(it_class);
        JNU_GetEnv()->DeleteGlobalRef(pt_class);
      }
    }
  }
  
  void copy(const Ref_counted_jdata& source)
  {
    jiterator=source.jiterator;
    it_class=source.it_class;
    pt_class=source.pt_class;
    counter=source.counter;
    ++(*counter);
  }
  
  Ref_counted_jdata(const Ref_counted_jdata& source)
  {
    copy(source);
  }
  
  ~Ref_counted_jdata(){cleanup();}
  
  Ref_counted_jdata& operator=(const Ref_counted_jdata& source)
  {
    if (this!=&source){
      cleanup();
      copy(source);
    }
    return *this;
  }
  
};
#endif

template <class Cpp_wrapper,class Cpp_base>
class Input_iterator_wrapper:
public boost::iterator_facade<
    Input_iterator_wrapper<Cpp_wrapper,Cpp_base>,
    Cpp_base,
    boost::single_pass_traversal_tag,
    typename boost::mpl::if_<
          boost::mpl::bool_<internal::Converter<Cpp_wrapper>::is_reference>, 
          const Cpp_base&, Cpp_base
        >::type //this allows to use a reference for dereference when possible
    >
{
  friend class boost::iterator_core_access;
  std::string signature;
  Cpp_wrapper* current_ptr;
  jmethodID getCPtr_id, next_id, hasnext_id;
  Ref_counted_jdata rc;
  
  void update_with_next_point(bool first=false){
    if (static_cast<bool> (JNU_GetEnv()->CallBooleanMethod(rc.jiterator,hasnext_id)) )
    {
      jobject jpoint=JNU_GetEnv()->CallObjectMethod(rc.jiterator,next_id);
      if (first){
        rc.pt_class = (jclass) JNU_GetEnv()->NewGlobalRef( JNU_GetEnv()->GetObjectClass(jpoint) );
        assert(rc.pt_class!=NULL);
        getCPtr_id=JNU_GetEnv()->GetStaticMethodID(rc.pt_class, "getCPtr",signature.c_str());
      }
      assert(getCPtr_id!=NULL);
      assert(rc.pt_class!=NULL);
      jlong jpt=(jlong) JNU_GetEnv()->CallStaticObjectMethod(rc.pt_class,getCPtr_id,jpoint);
      current_ptr = (Cpp_wrapper*) jpt;
      
    }
    else{
      current_ptr=NULL;
    }
  }
  
public:
  

  Input_iterator_wrapper():current_ptr(NULL){}
  Input_iterator_wrapper(const jobject& jiterator_,const char* sign):signature(sign),rc(jiterator_)
  {
    assert(rc.it_class!=NULL);
    hasnext_id=JNU_GetEnv()->GetMethodID(rc.it_class, "hasNext", "()Z");
    assert(hasnext_id!=NULL);
    next_id=JNU_GetEnv()->GetMethodID(rc.it_class, "next","()Ljava/lang/Object;");
    getCPtr_id=NULL;
    rc.pt_class=NULL;
    assert(next_id!=NULL);
    update_with_next_point(true);
  }

  
  void increment(){assert(current_ptr!=NULL); update_with_next_point();}
  bool equal(const Input_iterator_wrapper & other) const{ return current_ptr==other.current_ptr; }
  
  typename boost::mpl::if_<
        boost::mpl::bool_<internal::Converter<Cpp_wrapper>::is_reference>, 
        const Cpp_base&, Cpp_base
      >::type
    dereference() const { return internal::Converter<Cpp_wrapper>::convert(*current_ptr); }
};



#endif// CGAL_SWIG_JAVA_INPUT_ITERATOR_WRAPPER_H