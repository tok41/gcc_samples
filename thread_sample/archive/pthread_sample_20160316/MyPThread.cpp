#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>

#include "MyPThread.h"

const int kSleepSec  = 1;

// コンストラクタ
MyPThread::MyPThread()
  :tid_()
  ,mutex_()
  ,count_(0)
  ,execCount_(0)
{
  pthread_mutex_init(&mutex_, NULL);
  pthread_create(&tid_, NULL, MyPThread::launchThread, this);
}

// デストラクタ
MyPThread::~MyPThread()
{
  pthread_cancel(tid_);
  pthread_join(tid_, NULL);
  pthread_mutex_destroy(&mutex_);
}


void MyPThread::stop()
{
  pthread_cancel(tid_);
}

void MyPThread::clear()
{
  count_ = 0;
  // 呼び出し元が別スレッドであることの確認のためprint
  std::cout << pthread_self() << " : clear" << std::endl;
}

void MyPThread::clearWithMutex()
{
  pthread_mutex_lock(&mutex_);
  {
	count_ = 0;
	std::cout << pthread_self() << " : clear" << std::endl;
  }
  pthread_mutex_unlock(&mutex_);
}


void MyPThread::execute()
{
  pthread_mutex_lock(&mutex_);
  {
	while (execCount_ < 5) {
	  pthread_testcancel();

	  printf("%x:", pthread_self());
	  for (int i=0; i<10; i++) {
		printf("%3d:", count_);
		count_++;
	  }
	  printf("\n");
	  sleep(kSleepSec);

	  execCount_++;
	}
  }
  pthread_mutex_unlock(&mutex_);

  printf("finish\n");
}
