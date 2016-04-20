

#ifdef __cplusplus
class SampleClass {
 public:
  SampleClass();
  ~SampleClass();
  void disp();
};


extern "C" {
  // constructor and destructor
  CSampleClass newCSampleClass();
  void delCSampleClass(CSampleClass);
  
  // public method
  void dispC(CSampleClass);
}

#endif

// C Interface
typedef void* CSampleClass;



