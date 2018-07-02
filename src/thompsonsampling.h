#ifndef THOMPSONSAMPLING_H
#define THOMPSONSAMPLING_H

#include "mabalg.h"
#include "utils.h"

class ThompsonSampling: public MABAlgorithm {
protected:
	boost::mt19937* rng;
public:
	ThompsonSampling(string name, MAB& mab, boost::mt19937& rng);
};

class ThompsonSamplingBernoulli: public ThompsonSampling {
private:
	vector<int> alphas, betas;
public:
	ThompsonSamplingBernoulli(string name, MAB& mab, boost::mt19937& rng);
	ArmPull run(vector<vector<double>>& all_pulls, int timestep, bool generate_new_pulls) override;
	void reset() override;
};

class ThompsonSamplingGaussian: public ThompsonSampling {
private:
	vector<double> means;
public:
	ThompsonSamplingGaussian(string name, MAB& mab, boost::mt19937& rng);
	ArmPull run(vector<vector<double>>& all_pulls, int timestep, bool generate_new_pulls) override;
	void reset() override;
};

class GlobalCTS: public ThompsonSampling {
private:
	double gamma;
	vector<double> runlength_distribution; // runlength_distribution[runlength]
	vector<vector<double>> alphas; // alphas[arm][runlength]
	vector<vector<double>> betas; // betas[arm][runlength]
public:
	GlobalCTS(string name, MAB& mab, boost::mt19937& rng, double gamma);
	ArmPull run(vector<vector<double>>& all_pulls, int timestep, bool generate_new_pulls) override;
	void reset() override;
};

class PerArmCTS: public ThompsonSampling {
private:
	double gamma;
	vector<vector<double>> runlength_distribution; // runlength_distribution[arm][runlength]
	vector<vector<double>> alphas; // alphas[arm][runlength]
	vector<vector<double>> betas; // betas[arm][runlength]
public:
	PerArmCTS(string name, MAB& mab, boost::mt19937& rng, double gamma);
	ArmPull run(vector<vector<double>>& all_pulls, int timestep, bool generate_new_pulls) override;
	void reset() override;
};

#endif
