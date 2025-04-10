import pandas as pd
import numpy as np

import matplotlib.pyplot as plt

# Use suprocess to run ./bitstream_falloff 48
import subprocess
from tqdm import tqdm
from scipy.optimize import curve_fit

gen_patterns = 48
num_reps = 100
cutoff = gen_patterns * 8


# # Run the command and capture the output
# for _ in tqdm(range(num_reps), desc="Running bitstream_falloff"):
#     # Run the command and capture the output
#     result = subprocess.run(['./bitstream_falloff', f'{gen_patterns}'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
#     # # Check if the command was successful
#     # if result.returncode != 0:
#     #     print(f"Error: {result.stderr.decode('utf-8')}")

# Read the CSV file using basic Python
bit_error_count = []
bit_total_count = []
with open("bit_errors.csv", 'r') as file:
    # Skip the header line
    next(file)
    
    # Process each line
    for line in file:
        line = line.strip()
        if line:
            bit_index, error = line.split(',')
            bit_index = int(bit_index)
            error = int(error)

            # if bit_index > cutoff: #gen_patterns*8:
            #     continue

            # If index not in list, append it
            if bit_index >= len(bit_error_count):
                bit_error_count.extend([0] * (bit_index - len(bit_error_count) + 1))
                bit_total_count.extend([0] * (bit_index - len(bit_total_count) + 1))
            # Update counts
            bit_error_count[bit_index] += error
            bit_total_count[bit_index] += 1
            
    # Calculate bit error rate
    bit_error_rate = [error / total if total > 0 else 0 for error, total in zip(bit_error_count, bit_total_count)]
    # Average the bit error rates in groups of 5
    group_size = 5
    grouped_error_rates = []
    group_indices = []
    
    for i in range(0, len(bit_error_rate), group_size):
        if i < len(bit_error_rate):
            grouped_error_rates.append(bit_error_rate[i])
            group_indices.append(i)
    
    # # Group the rest as before
    # for i in range(32, len(bit_error_rate), group_size):
    #     # Get the current group of bit error rates
    #     group = bit_error_rate[i:i + group_size]
    #     if len(group) == group_size:  # Only use complete groups
    #         # Calculate the average error rate for this group
    #         avg_error_rate = sum(group) / group_size
    #         # Use the center index of the group as the x-coordinate
    #         center_index = i + group_size // 2
            
    #         grouped_error_rates.append(avg_error_rate)
    #         group_indices.append(center_index)

    # Create a scatter plot
    plt.figure(figsize=(10, 6))
    plt.scatter(group_indices, grouped_error_rates, alpha=0.7)
    
    # Add exponential decay curve fit that asymptotically approaches a value
    # Model: y = a * exp(-b * x) + c, where c is the asymptotic value
    
    def exp_decay(x, a, b, c):
        return a * np.exp(-b * x) + c
    
    # Initial parameter guesses
    p0 = [max(grouped_error_rates) - min(grouped_error_rates), 0.05, min(grouped_error_rates)]
    
    # Fit the exponential model
    popt, _ = curve_fit(exp_decay, group_indices, grouped_error_rates, p0=p0, maxfev=10000)
    a, b, c = popt
    
    # Create smooth curve for plotting
    x_smooth = np.linspace(min(group_indices), max(group_indices), 500)
    y_smooth = exp_decay(x_smooth, *popt)
    
    # Plot the fitted curve
    plt.plot(x_smooth, y_smooth, color='#FF9999', 
             label=f'Asymptotic Fit')
    
    # Add horizontal line showing asymptotic value
    plt.axhline(y=c, color='g', linestyle='--', alpha=0.5,
                label=f'Asymptotic value: {c:.3f}')
    
    plt.xlabel('Bit Index ')
    plt.ylabel('Bit Error Rate')
    plt.title('Bit Error Rate vs Bit Index')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()

    # Save the plot to a file
    plt.savefig('bit_error_rate.png', dpi=300, bbox_inches='tight')


# Optional: display the plot
# plt.show()