
#include <iostream>
#include <map>

int main(int argc, char const* argv[])
{
  std::map<int,int> m1;

  m1[1] = 100;
  m1[2] = 200;
  m1[3] = 300;
  // []でアクセスできる
  std::cout << "val:" << m1[1] << std::endl;
  std::cout << "val:" << m1[2] << std::endl;
  std::cout << "val:" << m1[3] << std::endl;
  // イテレータになる
  for (auto& x:m1) {
	std::cout << x.first << " => " << x.second << std::endl;
  }

  // 存在しないキーでアクセスすると自動的に追加
  std::map<int,int> m2;
  std::cout << "size : " << m2.size() << std::endl;// 当然 0
  std::cout << "アクセスしてみる : " << m2[1] << std::endl;// 存在していないキーにアクセスする
  std::cout << "size : " << m2.size() << std::endl;// 上の行のアクセスの結果、１要素追加されている
  for (auto& x:m2) {
	// キー1, 値 0 が表示される
	std::cout << x.first << " => " << x.second << std::endl;
  }

  // キーでmapを検索
  std::map<int, int>::iterator it;
  it = m1.find(2);
  std::cout << it->first << " => " << it->second << std::endl;

  // キーの存在を確認する
  if(m1.find(2)==m1.end()) std::cout << "not find" << std::endl;
  else std::cout << "find" << std::endl;
  if(m1.find(5)==m1.end()) std::cout << "not find" << std::endl;
  else std::cout << "find" << std::endl;
  // countって方法もある
  std::cout << "count : " << m1.count(2) << std::endl;
  std::cout << "count : " << m1.count(4) << std::endl;

  return 0;
}



