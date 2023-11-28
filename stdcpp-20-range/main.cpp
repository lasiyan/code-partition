#include <algorithm>
#include <iostream>
#include <ranges>
#include <vector>

int main()
{
  int              elem1 = 1;
  std::vector<int> elem2 = { 1, 2, 3, 4, 5 };

  // example 1. std::ranges::range : 범위를 가지고 있는지 확인
  std::cout << std::ranges::range<decltype(elem1)> << std::endl;
  std::cout << std::ranges::range<decltype(elem2)> << std::endl;
  std::cout << "----------------------------------------" << std::endl;

  // example 2. view : 뷰를 생성
  std::ranges::filter_view view1(elem2, [](int i) { return i % 2 != 0; });

  auto view2 = std::ranges::filter_view(elem2, [](int i) { return i % 2 != 0; });
  auto view3 = elem2 | std::views::filter([](int i) { return i % 2 != 0; });
  auto view4 = std::views::filter(elem2, [](int i) { return i % 2 != 0; });

  // example 3. transform
  std::string str = "Hello World !\n";

  auto up_view = str | std::views::transform([](char& c) { return c = std::toupper(c); });
  std::for_each(up_view.begin(), up_view.end(), [](char c) { std::cout << c; });

  return 0;
}