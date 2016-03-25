#ifndef _INC_SERV    //まだ読み込まれていなければ以下の処理をする

#define _INC_SERV    //すでに読み込まれているという目印をつける

class device_server
{
public:
  device_server();
  ~device_server();
  void Activate();
  void disp();
};

#endif
