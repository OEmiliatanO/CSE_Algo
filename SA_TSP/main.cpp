#include <iostream>
#include <cstring>
#include <algorithm>
#include <random>
#include <vector>
#include <cmath>
#include <thread>
#include <future>

using int64 = long long;
using city_t = std::tuple<int, int64, int64>;
using cities_t = std::vector<city_t>;
using ans_t = std::pair<double, cities_t>;

/*
struct city
{
	int n;
	int64 x, y;
	city(int _n, const int64& _x, const int64& _y): n(_n), x(_x), y(_y) {}
	city(int&& _n, int64&& _x, int64&& _y): n(std::move(_n)), x(std::move(_x)), y(std::move(_y)) {}
	city(): n(0), x(0), y(0) {}
};*/


std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<double> unid(0.0, 1.0);

double dist(const city_t& c1, const city_t& c2)
{
	const int64& x1 = std::get<1>(c1), x2 = std::get<1>(c2), y1 = std::get<2>(c1), y2 = std::get<2>(c2);
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

double E(const cities_t& vec)
{
	double res = 0;
	for (auto it = vec.begin() + 1; it != vec.end(); ++it)
		res += dist(*it, *(it - 1));
	return res + dist(*vec.rbegin(), *vec.begin());
}

cities_t rdChange(const cities_t& vec)
{
	cities_t res(vec);
	iter_swap(res.begin() + res.size() * unid(mt), res.begin() + res.size() * unid(mt));
	return res;
}

constexpr int MAXIter = (int)1e3;
constexpr double Rt = 0.97;
constexpr double EndT = 1e-4;
constexpr double InitT = 1e4;
constexpr double Thre = 1 / InitT;
ans_t SA(const cities_t& cities)
{
	cities_t current(cities), best(cities), nex;
	double minE = std::numeric_limits<double>::infinity(), T = InitT;
	auto willSwap = [&](const double dt) {
		return unid(mt) < Thre * T; 
		//return unid(mt) < exp(-dt / T); 
	};
	while (T > EndT)
	{
		for(int _ = 0; _ < MAXIter; ++_)
		{
			nex = std::move(rdChange(current));
			auto Enex = E(nex), Ecurr = E(current);
			auto dt = Enex - Ecurr;
			if (dt < 0 or willSwap(dt))
			{
				current.swap(nex);
				if (minE > Enex)
				{
					minE = Enex;
					best.swap(nex);
				}
			}
		}
		T *= Rt;
	}
	return make_pair(minE, current);
}

int main()
{
	int n, x, y;
 	cities_t cities;
	while(std::cin >> n >> x >> y)
		cities.emplace_back(n, x, y);

	std::vector<ans_t> ansPool;
	std::vector< std::future<ans_t> > threadPool(5);

	for (auto& it : threadPool)
		it = async(SA, cities);
	for (auto& it : threadPool)
		ansPool.emplace_back(it.get());

	sort(ansPool.begin(), ansPool.end());

	const ans_t& ans = *ansPool.begin();
	std::cout << "the minimal length is " << ans.first << '\n';
	std::cout << "and the path is (";
	for (auto& it : ans.second)
		std::cout << std::get<0>(it) << " -> ";
	std::cout << std::get<0>(*ans.second.begin()) << ')' << '\n';
	
	return 0;
}