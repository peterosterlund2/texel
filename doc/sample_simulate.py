#!/usr/bin/python

# Simulate the sampling process that estimates the number of legal chess positions

import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import norm
from scipy import special

# Sampling parameters
N = 2 * 3969 * 2**62 * 2**30 * 5**30 * 6   # Size of space to sample from
N1 = 10 * np.floor(N / 4.8e44 * 6e5)       # First sample size
N2 = 100000                                # Second sample size

# Assumed true value of the parameter to estimate
N_legal = 4.8e44

# Assumed true value of the nuisance parameter
q = (100000 - 17) / 100000      # N_legal / N_candidates
N_candidates = N_legal / q

# Simulation parameters
num_simulations = 100_000_000
target_conf_level = 0.95

# Introduce some help quantities
p = N_candidates / N
# q = N_legal / N_candidates
n = N1
m = N2

# Sample from the following distributions
# C = N_n1_candidates ~ Binomial(n, p)
# L | C = N_n1_legal | N_n1_candidates ~ Binomial(C, q)
# H | C,L = N_s2_legal | N_n1_candidates, N_n1_legal ~ HyperGeometric(C, L, m)

rng = np.random.default_rng()
C = rng.binomial(n=n, p=p, size=num_simulations)
L = rng.binomial(n=C, p=q, size=num_simulations)
H = rng.hypergeometric(ngood=L, nbad=C-L, nsample=N2)

p_hat = C / n
q_hat = H / m
N_hat = float(N) * p_hat * q_hat

a_hat = (1 - p_hat) / (n * p_hat)
b_hat = (1 - q_hat) / (m * q_hat)
std_N_hat_estimated = N_hat * np.sqrt(a_hat + b_hat * (1 + a_hat))

a = (1 - p) / (n * p)
b = (1 - q) / (m * q)
std_N = N_legal * np.sqrt(a + b * (1 + a))

mean_N_hat_simulated = np.mean(N_hat)
std_N_hat_simulated = np.std(N_hat, ddof=1)

k = np.sqrt(2) * special.erfinv(target_conf_level)

interval_low = N_hat - k * std_N_hat_estimated
interval_hi  = N_hat + k * std_N_hat_estimated

x=(N_legal < interval_low)
y=(interval_hi < N_legal)
fail_rate = (np.count_nonzero(x)+np.count_nonzero(y)) / num_simulations

# Print result
print(f'Simulations        : {num_simulations:.4g}')
print(f'Parameter N_legal  : {N_legal}')
print(f'Parameter q        : {q}')

print(f'Exact mean         : {N_legal}')
print(f'Exact SD           : {std_N}')

print(f'Simulated mean     : {mean_N_hat_simulated}')
print(f'Simulated SD       : {std_N_hat_simulated}')
print(f'Mean estimated SD  : {np.mean(std_N_hat_estimated)}')

print(f'Confidence level   : {target_conf_level}')
print(f'Confidence factor  : {k}')

print(f'Empirical coverage : {1-fail_rate}')
