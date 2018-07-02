#include "ucb.h"
#include "utils.h"


UCB::UCB(string name, MAB& mab) : MABAlgorithm(name, mab) {}

UCB1::UCB1(string name, MAB& mab) : UCB(name, mab) {
	this->reset();
}

UCB1_With_Exploration::UCB1_With_Exploration(string name, MAB& mab, double alpha) : UCB(name, mab) {
	this->alpha = alpha;
	this->reset();
}

UCBT::UCBT(string name, MAB& mab) : UCB(name, mab) {
	this->reset();
}

D_UCB::D_UCB(string name, MAB& mab, double gamma, double B, double epsilon) : UCB(name, mab) {
	this->reset();
	this->gamma = gamma;
	this->B = B;
	this->epsilon = epsilon;
}

SW_UCB::SW_UCB(string name, MAB& mab, int tau, double B, double epsilon) : UCB(name, mab) {
	this->reset();
	this->tau = tau;
	this->B = B;
	this->epsilon = epsilon;
}


void UCB1::reset() {
	mabalg_reset();
	this->means.assign(this->num_of_arms, 0.);
}

void UCB1_With_Exploration::reset() {
	mabalg_reset();
	this->means.assign(this->num_of_arms, 0.);
}

void UCBT::reset() {
	mabalg_reset();
	this->means.assign(this->num_of_arms, 0.);
	this->collected_rewards.resize(this->num_of_arms);
}

void D_UCB::reset() {
	mabalg_reset();
	this->means.assign(this->num_of_arms, 0.);
	this->ns.assign(this->num_of_arms, 0.);
}

void SW_UCB::reset() {
	mabalg_reset();
	this->windowed_arm_pulls.clear();
	this->windowed_arm_pulls_values.clear();
	this->windowed_arm_pulls.resize(this->num_of_arms);
	this->windowed_arm_pulls_values.resize(this->num_of_arms);
}


ArmPull UCB1::run(vector<vector<double>>& all_pulls, int timestep, bool generate_new_pulls) {
	vector<double> pulls = all_pulls[timestep];
	int arm_to_pull = -1;

	// Choose arm to pull
	int tot_pulls = accumulate(this->num_of_pulls.begin(), this->num_of_pulls.end(), 0.);
	if (tot_pulls < this->num_of_arms) {
		// First phase: pull each arm once
		arm_to_pull = tot_pulls;
	}
	else {
		// Second phase: pull arm that maximizes Q+B
		double bestQB = -100000;
		int best_arm = -1;
		for (int i = 0; i < this->num_of_arms; i++) {
			double Q = this->means[i];
			double B = sqrt((2*log(tot_pulls + 1)) / (this->num_of_pulls[i]));
			double QB = Q + B;
			if (QB > bestQB) {
				bestQB = QB;
				best_arm = i;
			}
		}
		arm_to_pull = best_arm;
	}

	// Pull arm
	ArmPull armpull = this->pull_arm(all_pulls, timestep, generate_new_pulls, arm_to_pull);

	// Update algorithm statistics
	this->means[arm_to_pull] = (this->means[arm_to_pull] * (this->num_of_pulls[arm_to_pull] - 1) + armpull.reward) / this->num_of_pulls[arm_to_pull];

	return armpull;
}

ArmPull UCB1_With_Exploration::run(vector<vector<double>>& all_pulls, int timestep, bool generate_new_pulls) {
	vector<double> pulls = all_pulls[timestep];
	int arm_to_pull = -1;

	// Choose arm to pull
	int tot_pulls = accumulate(this->num_of_pulls.begin(), this->num_of_pulls.end(), 0.);
	if (tot_pulls < this->num_of_arms) {
		// First phase: pull each arm once
		arm_to_pull = tot_pulls;
	}
	else {
		// Second phase: pull arm that maximizes Q+B or random exploration
		float r = random_unit();
		if (r < this->alpha) { // explore
			arm_to_pull = rand() % this->num_of_arms;
		} else { // maximize Q+B
			double bestQB = -100000;
			int best_arm = -1;
			for (int i = 0; i < this->num_of_arms; i++) {
				double Q = this->means[i];
				double B = sqrt((2*log(tot_pulls + 1)) / (this->num_of_pulls[i]));
				double QB = Q + B;
				if (QB > bestQB) {
					bestQB = QB;
					best_arm = i;
				}
			}
			arm_to_pull = best_arm;
		}
	}

	// Pull arm
	ArmPull armpull = this->pull_arm(all_pulls, timestep, generate_new_pulls, arm_to_pull);

	// Update algorithm statistics
	this->means[arm_to_pull] = (this->means[arm_to_pull] * (this->num_of_pulls[arm_to_pull] - 1) + armpull.reward) / this->num_of_pulls[arm_to_pull];

	return armpull;
}


ArmPull UCBT::run(vector<vector<double>>& all_pulls, int timestep, bool generate_new_pulls) {
	vector<double> pulls = all_pulls[timestep];
	int arm_to_pull = -1;

	// Choose arm to pull
	int tot_pulls = accumulate(this->num_of_pulls.begin(), this->num_of_pulls.end(), 0.);
	if (tot_pulls < this->num_of_arms) {
		// First phase: pull each arm once
		arm_to_pull = tot_pulls;
	}
	else {
		// Second phase: pull arm that maximizes Q+B
		double bestQB = -100000;
		int best_arm = -1;
		for (int i = 0; i < this->num_of_arms; i++) {
			double Q = this->means[i];
			// It's not possible to compute the variance online since the mean of the arm is updated at each step!
			double variance = 0;
			for (int j = 0; j < this->collected_rewards[i].size(); j++) {
				variance += pow((this->collected_rewards[i][j] - Q), 2);
			}
			variance /= this->num_of_pulls[i];
			double B = sqrt((2*log(tot_pulls + 1)*min(0.25, variance))/(this->num_of_pulls[i]));
			double QB = Q + B;
			if (QB > bestQB) {
				bestQB = QB;
				best_arm = i;
			}
		}
		arm_to_pull = best_arm;
	}

	// Pull arm
	ArmPull armpull = this->pull_arm(all_pulls, timestep, generate_new_pulls, arm_to_pull);

	// Update algorithm statistics
	this->means[arm_to_pull] = (this->means[arm_to_pull] * (this->num_of_pulls[arm_to_pull] - 1) + armpull.reward) / this->num_of_pulls[arm_to_pull];

	return armpull;
}

ArmPull D_UCB::run(vector<vector<double>>& all_pulls, int timestep, bool generate_new_pulls) {
	vector<double> pulls = all_pulls[timestep];
	int arm_to_pull = -1;

	// Choose arm to pull
	int tot_pulls = accumulate(this->num_of_pulls.begin(), this->num_of_pulls.end(), 0.);
	if (tot_pulls < this->num_of_arms) {
		// First phase: pull each arm once
		arm_to_pull = tot_pulls;
	}
	else {
		// Second phase: pull arm that maximizes Q+B
		double bestQB = -100000;
		int best_arm = -1;
		double ns_sum = accumulate(this->ns.begin(), this->ns.end(), 0.);
		for (int i = 0; i < this->num_of_arms; i++) {
			double Q = this->means[i];
			double B = (2 * this->B) * sqrt((this->epsilon * log(ns_sum)) / (this->ns[i]));
			double QB = Q + B;
			if (QB > bestQB) {
				bestQB = QB;
				best_arm = i;
			}
		}
		arm_to_pull = best_arm;
	}

	// Pull arm
	ArmPull armpull = this->pull_arm(all_pulls, timestep, generate_new_pulls, arm_to_pull);

	// Update algorithm statistics

	// Update this->ns (it's like this->num_of_pulls, but discounted with this->gamma)
	vector<double> old_ns;
	old_ns.assign(this->num_of_arms, 0.);;
	copy(this->ns.begin(), this->ns.end(), old_ns.begin());
	for (int i = 0; i < this->num_of_arms; i++) {
		this->ns[i] *= this->gamma;
	}
	this->ns[arm_to_pull] += 1.0;

	// Update this->means (it's like a normal mean, but discounted with this->gamma)
	for (int i = 0; i < this->num_of_arms; i++) {
		this->means[i] *= old_ns[i];
	}
	this->means[arm_to_pull] += armpull.reward;
	for (int i = 0; i < this->num_of_arms; i++) {
		if (this->ns[i] > 0) { // Because we don't want to divide by zero...
			this->means[i] *= this->gamma / this->ns[i];
		} else {
			this->means[i] = 0;
		}
	}

	return armpull;
}

ArmPull SW_UCB::run(vector<vector<double>>& all_pulls, int timestep, bool generate_new_pulls) {
	vector<double> pulls = all_pulls[timestep];
	int arm_to_pull = -1;

	// Choose arm to pull
	int tot_pulls = accumulate(this->num_of_pulls.begin(), this->num_of_pulls.end(), 0.);
	if (tot_pulls < this->num_of_arms) {
		// First phase: pull each arm once
		arm_to_pull = tot_pulls;
	}
	else {
		// Second phase: pull arm that maximizes Q+B
		double bestQB = -100000;
		int best_arm = -1;
		for (int i = 0; i < this->num_of_arms; i++) {
			int windowed_N = accumulate(this->windowed_arm_pulls[i].begin(), this->windowed_arm_pulls[i].end(), 0);
			double windowed_reward = accumulate(this->windowed_arm_pulls_values[i].begin(), this->windowed_arm_pulls_values[i].end(), 0.);
			double Q = windowed_reward / windowed_N;
			double B = this->B * sqrt((this->epsilon * log(min(this->tau, tot_pulls + 1))) / windowed_N);
			double QB = Q + B;

			if (QB > bestQB) {
				bestQB = QB;
				best_arm = i;
			}
		}
		arm_to_pull = best_arm;
	}

	// Pull arm
	ArmPull armpull = this->pull_arm(all_pulls, timestep, generate_new_pulls, arm_to_pull);

	// Update algorithm statistics

	// Move the window
	for (int i = 0; i < this->num_of_arms; i++) {
		if (i == arm_to_pull) {
			this->windowed_arm_pulls[i].push_back(1);
			this->windowed_arm_pulls_values[i].push_back(armpull.reward);
		} else {
			this->windowed_arm_pulls[i].push_back(0);
			this->windowed_arm_pulls_values[i].push_back(0.);
		}

		if (this->windowed_arm_pulls[i].size() > this->tau)
			this->windowed_arm_pulls[i].erase(this->windowed_arm_pulls[i].begin());
		if (this->windowed_arm_pulls_values[i].size() > this->tau)
			this->windowed_arm_pulls_values[i].erase(this->windowed_arm_pulls_values[i].begin());
	}

	return armpull;
}
