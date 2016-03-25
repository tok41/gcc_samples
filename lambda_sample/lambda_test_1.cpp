#include <iostream>

template<typename Func>
void f(Func func){
  // 関数を渡してもらう
  func();
}


void f(Func func){
  // 関数を渡してもらう
  func();
}

int main(int argc, char const* argv[])
{
  // 簡単なラムダ式の例
  []{ std::cout<<"Hello World"<<std::endl;}();

  f( []{
	  std::cout<<"Hello world"<<std::endl;
	} );

  return 0;
}
