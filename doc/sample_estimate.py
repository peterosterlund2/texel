#!/usr/bin/python

# Compute estimate based on one instance of the sampling procedure

import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import norm
from scipy import special

# Sampling parameters
N = 2 * 3969 * 2**62 * 2**30 * 5**30 * 6   # Size of space to sample from
N1 = 10 * np.floor(N / 4.8e44 * 6e5)       # First sample size
N2 = 100000                                # Second sample size

# Sampling result
N_n1_candidates = 6030640
N_s2_illegal = 17

# Confidence level
target_conf_level = 0.95
k = np.sqrt(2) * special.erfinv(target_conf_level)

# Compute estimates
p_hat = N_n1_candidates / N1
q_hat = (N2 - N_s2_illegal) / N2
n = N1
m = N2
a = (1 - p_hat) / (n * p_hat)
b = (1 - q_hat) / (m * q_hat)
N_legal_est = float(N) * p_hat * q_hat
std_N_legal_est = N_legal_est * np.sqrt(a + b * (1 + a))

# Compute confidence interval
interval_lo = N_legal_est - k * std_N_legal_est
interval_hi = N_legal_est + k * std_N_legal_est

# Print result
print(f'Estimate           : {N_legal_est:7e}')
print(f'Standard deviation : {std_N_legal_est:7e}')
print(f'Confidence level   : {target_conf_level}')
print(f'Confidence factor  : {k:.7g}')
print(f'Low                : {interval_lo:7e}')
print(f'High               : {interval_hi:7e}')
print(f'Half width         : {(interval_hi - interval_lo)/2:7e}')
print(f'a                  : {a:7g}')
print(f'b                  : {b:7g}')
print(f'a / (a + b)        : {a/(a+b):7g}')
print(f'a + b + a * b      : {a+b+a*b:7g}')
