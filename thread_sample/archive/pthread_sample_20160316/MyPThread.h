#include <pthread.h>

class MyPThread
{
 public:
  MyPThread();
  virtual ~MyPThread();

  void stop();

  // mutex有無の動作確認用
  void clear();
  void clearWithMutex();

 private:
  static void* launchThread(void *pParam) {
	reinterpret_cast<MyPThread*>(pParam)->execute();
	pthread_exit(NULL);
  }

  void execute();

  pthread_t       tid_;
  pthread_mutex_t mutex_;

  int count_;
  int execCount_;
};
