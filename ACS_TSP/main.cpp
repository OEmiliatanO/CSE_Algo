#include <iostream>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cmath>
#include <random>
#include <fstream>

using int64 = long long;
using city_t = std::tuple<int, int64, int64>;
using cities_t = std::vector<city_t>;
using ans_t = std::pair<double, std::vector<int>>;

constexpr int MAXN = 64; // maximum city number

constexpr int MAX_ANT_N = 20;
constexpr double alpha = 0.1;
constexpr double beta = 2;
constexpr double q0 = 0.9;
constexpr double P = 0.1;
double pheromone_init = 0.001; // shall be modified dynamically

double d[MAXN + 1][MAXN + 1];       // d[i][j] := distance between city i, j
double phero[MAXN + 1][MAXN + 1];   // pheromone (tau)
double dphero[MAXN + 1][MAXN + 1];  // Delta pheromone (Delta tau)

std::random_device rd; 
std::mt19937 mt(rd());

// an ant choose which city to go next
int choose_city(std::vector<std::pair<int, double>>& prob) // prob: [{city, probability}, ..., {city, probability}]
{
	std::uniform_real_distribution<double> qgen(0.0, 1.0);
	if (qgen(mt) <= q0)
	{
		auto nex_city = std::max_element(prob.begin(), prob.end(), [&](auto& lhs, auto& rhs){ return lhs.second < rhs.second; });
		return (*nex_city).first;
		//return 0;
	}

	if (prob.size() <= 0)
	{
		std::cerr << "error in choose_city\nprob.size <= 0\n";
		exit(0);
	}
	if (prob.size() == 1)
		return prob[0].first;

	double psum = 0;
	for (size_t i = 0; i < prob.size(); ++i)
		psum += prob[i].second;
	
	// random float point generator
	std::uniform_real_distribution<double> unid(0.0, psum); // random float point in [0, psum]
	double p = unid(mt), choose = 0;
	for (size_t i = 0; i < prob.size(); ++i)
	{
		choose += prob[i].second;
		if (p <= choose)
			return prob[i].first;
	}
	std::cerr << "error in choose_city\nchoose nothing\n";
	exit(0);
}

double dist(const city_t& a, const city_t& b)
{
	return sqrt( (std::get<1>(a) - std::get<1>(b))*(std::get<1>(a) - std::get<1>(b)) + (std::get<2>(a) - std::get<2>(b))*(std::get<2>(a) - std::get<2>(b)));
}

// ants generate solutions
void generateSol(int n, int ant_n, ans_t& best_sol)
{
	// random int generator
	std::uniform_int_distribution<int> unid(1, n); // random integer in [1, n]

	std::vector<std::pair<int, double>> prob;
	ans_t ant_sol; // ans_t: {length, [city, ...]}
	for (int k = 0; k < ant_n; ++k)
	{
		ant_sol.first = 0;
		ant_sol.second.clear();
		ant_sol.second.emplace_back(unid(mt)); // choose random city to begin
		
		// if the i-th bit of "vis" is 1, this ant have visited there already
		long long vis = (1LL << ant_sol.second.back());

		while (ant_sol.second.size() < (size_t)n)
		{
			int from = ant_sol.second.back();
			
			prob.clear();
			
			// find which city haven't been visited yet
			for (int j = 1; j <= n; ++j)
			{
				if ((1LL<<j) & vis) continue;
				if (from == j) continue;
				// calculate probability
				prob.emplace_back(j, (phero[from][j] * pow(1/d[from][j], beta)));
			}
			
			// choose a city to visit
			int to = choose_city(prob);

			ant_sol.second.emplace_back(to);
			vis |= (1LL<<to);
			ant_sol.first += d[from][to];
		}
		// this ant have completed the journey
		ant_sol.first += d[ant_sol.second.back()][ant_sol.second.front()];
		
		// local update
		for (auto it = ant_sol.second.begin(); it+1 != ant_sol.second.end(); ++it)
			phero[*it][*(it+1)] = phero[*(it+1)][*it] = (1-P) * phero[*it][*(it+1)] + P*pheromone_init;
		phero[ant_sol.second.back()][ant_sol.second.front()] = phero[ant_sol.second.front()][ant_sol.second.back()] = \
			(1-P) * phero[ant_sol.second.back()][ant_sol.second.front()] + P*pheromone_init;

		// update best solution
		if (best_sol.first > ant_sol.first)
		{
			best_sol.first = ant_sol.first;
			best_sol.second.swap(ant_sol.second);
		}
	}
}
void gobal_phero_update(ans_t best_sol, double a = alpha)
{
	for (auto it = best_sol.second.begin(); it+1 != best_sol.second.end(); ++it)
	{
		phero[*it][*(it+1)] = phero[*(it+1)][*it] = (1-a) * phero[*it][*(it+1)] + a / best_sol.first;
	}
	phero[best_sol.second.back()][best_sol.second.front()] = phero[best_sol.second.front()][best_sol.second.back()] = \
		(1-a) * phero[best_sol.second.back()][best_sol.second.front()] + a / best_sol.first;
}

double GE(const cities_t& ct)
{
	cities_t cities = ct;
	cities_t ans_ct;
	double ans = 0;
	auto n = cities.size();
	auto dist = [](const city_t &a, const city_t &b) { int64 x1 = std::get<1>(a), x2 = std::get<1>(b), y1 = std::get<2>(a), y2 = std::get<2>(b); return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)); };

	ans_ct.emplace_back(cities[0]);
	while (ans_ct.size() < n)
	{
		std::sort(cities.begin() + ans_ct.size(), cities.end(), [&](const city_t& c1, const city_t& c2) { 
				int64 x0 = std::get<1>(*ans_ct.rbegin()), x1 = std::get<1>(c1), x2 = std::get<1>(c2), y0 = std::get<2>(*ans_ct.rbegin()), y1 = std::get<2>(c1), y2 = std::get<2>(c2); 
				return ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1)) < ((x0 - x2) * (x0 - x2) + (y0 - y2) * (y0 - y2));
			});
#ifdef DEBUGing
		std::cerr << "the nearest city to " << std::get<0>(*ans_ct.rbegin()) << " is " << std::get<0>(*(cities.begin() + ans_ct.size())) << '\n';
		std::cerr << "the distance is " << dist(*ans_ct.rbegin(), *(cities.begin() + ans_ct.size())) << '\n';
#endif
		ans += dist(*ans_ct.rbegin(), *(cities.begin() + ans_ct.size()));
		ans_ct.emplace_back(*(cities.begin() + ans_ct.size()));
	}
	ans += dist(*ans_ct.rbegin(), *ans_ct.begin());

	return ans;
}
void init(int n, const cities_t& cities)
{
	double Lnn = GE(cities);
	pheromone_init = 1/(n*Lnn);
	for (int i = 0; i <= n; ++i)
		for (int j = 0; j <= n; ++j)
			phero[i][j] = pheromone_init;
}

ans_t ACO(const cities_t& cities, int n = 30, int t = 1000, int ant_n = MAX_ANT_N)
{
	ans_t best_sol{std::numeric_limits<double>::infinity(), std::vector<int>{}};
	double avg = 0;
	init(n, cities);
	// n runs
	for (int i = 0; i < n; ++i)
	{
		init(cities.size() - 1, cities);
		for (int j = 0; j < t; ++j)
		{
			generateSol(cities.size() - 1, ant_n, best_sol);
			gobal_phero_update(best_sol);

			std::cerr << '\r' << std::fixed << (double) (j+i*t+1)*100/(t*n) << '%';
		}
		avg += best_sol.first;
	}
	avg /= 30;
	std::cerr << '\n';
	std::cout << "avg = " << avg << '\n';
	return best_sol;
}

int main()
{
	size_t n;
	int64 x, y;
 	cities_t cities;
	cities.emplace_back(0, 0, 0);
	while(std::cin >> n >> x >> y)
		cities.emplace_back(n, x, y);
	
	std::sort(cities.begin(), cities.end());
	
	n = cities.size() - 1;
	memset(d, 0, sizeof(d));
	for (size_t i = 1; i <= n; ++i)
		for (size_t j = 1; j <= n; ++j)
			d[i][j] = dist(cities[i], cities[j]);

	ans_t ans = ACO(cities, 30, 1000);

	// I/O
	std::fstream plottxt;
	plottxt.open("plot.txt", std::ios::out);
	
	std::cout << "the minimal length is " << ans.first << '\n';
	for (auto& cy : ans.second)
	{
		std::cout << std::get<0>(cities[cy]) << '\n';
		plottxt << std::get<0>(cities[cy]) << " " << std::get<1>(cities[cy]) << " " << std::get<2>(cities[cy]) << '\n';
	}
	if (ans.second.size())
		plottxt << std::get<0>(cities[ans.second[0]]) << " " << std::get<1>(cities[ans.second[0]]) << " " << std::get<2>(cities[ans.second[0]]) << '\n';
	plottxt.close();
	return 0;
}
