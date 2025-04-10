import os
import subprocess
import time
import pandas as pd
import sys
import random

import matplotlib.pyplot as plt

def run_benchmark(message_file):
    # # Delete existing benchmark results if they exist
    # if os.path.exists("benchmark_results.csv"):
    #     os.remove("benchmark_results.csv")
    #     print("Deleted existing benchmark_results.csv")
    
    # Run the benchmark 10 times
    print(f"Running benchmark with message file: {message_file}")
    
    for i in range(10):
        # subprocess.run(["pkill", "-f", "benchmark"], check=True)

        print(f"Run {i+1}/10...")
        try:
            subprocess.run(["./benchmark", message_file], check=True)
        except subprocess.CalledProcessError as e:
            print(f"Error running benchmark: {e}")
            time.sleep(20)
        if i < 9:  # Don't sleep after the last run
            time.sleep(1)

        # Force kill process named "benchmark" if it is still running
        # subprocess.run(["pkill", "-f", "benchmark"], check=True)
        # time.sleep(1)


def create_graphs(df):
    # Calculate averages for each message length
    grouped = df.groupby('length').agg({
        'bit_accuracy_percent': 'mean',
        'throughput_bps': 'mean',
        'byte_accuracy_percent': 'mean'
    }).reset_index()
    

    # Create bins for message lengths
    bins = [0, 32, 64, 128, 256, 512, 1024]
    bin_labels = ['0-32', '32-64', '64-128', '128-256', '256-512', '512-1024']

    # Assign each message length to a bin
    df['length'] = pd.cut(df['length'], bins=bins, labels=bin_labels, right=False)

    # Calculate averages for each bin
    bin_grouped = df.groupby('length').agg({
        'bit_accuracy_percent': 'mean',
        'throughput_bps': 'mean',
        'byte_accuracy_percent': 'mean'
    }).reset_index()

    # Rename the '0-32' bin to 'phrase.txt'
    # bin_grouped['message_name'] = bin_grouped['length'].replace('0-32', 'phrase.txt')

    # # Rename the '32-64' bin to 'sentence.txt'
    # bin_grouped['message_name'] = bin_grouped['message_name'].replace('32-64', 'sentence.txt')

    # # Rename the '64-128' bin to 'paragraph.txt'
    # bin_grouped['message_name'] = bin_grouped['message_name'].replace('64-128', 'paragraph.txt')

    grouped = bin_grouped
    
    # Create figure with two subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    # Plot message length vs accuracy (both bit and byte) as bar chart with side-by-side bars
    bar_width = 0.35
    x = range(len(grouped['length']))
    
    ax1.bar([i - bar_width/2 for i in x], grouped['bit_accuracy_percent'], 
            width=bar_width, color='blue', label='Bit Accuracy', alpha=0.7)
    ax1.bar([i + bar_width/2 for i in x], grouped['byte_accuracy_percent'], 
            width=bar_width, color='red', label='Byte Accuracy', alpha=0.7)
    
    ax1.set_xlabel('Message Length (bytes)')
    ax1.set_ylabel('Accuracy (%)')
    ax1.set_title('Message Accuracy')
    ax1.set_xticks(x)
    ax1.set_xticklabels(grouped['length'])
    ax1.grid(True, axis='y')
    ax1.legend()
    
    # Set y-axis to start from a value based on the data rather than 0
    # Find the minimum value and set starting point slightly below it
    min_accuracy = min(grouped['bit_accuracy_percent'].min(), grouped['byte_accuracy_percent'].min())
    # Start from 5% below the minimum or 70%, whichever is lower
    y_start = max(min_accuracy - 5, 70)
    ax1.set_ylim(bottom=90,top=101)
    
    # Plot message length vs throughput as bar chart
    ax2.bar(grouped['length'], grouped['throughput_bps'], color='green', alpha=0.7)
    ax2.set_xlabel('Message Length (bytes)')
    ax2.set_ylabel('Throughput (bits/sec)')
    ax2.set_title('Message vs Throughput')
    ax2.grid(True, axis='y')
    
    plt.tight_layout()
    plt.savefig('benchmark_results.png')
    print("Graphs saved to benchmark_results.png")
    plt.show()

def main():
    
    # message_files = ["word.txt", "letter.txt", "noodles.txt", "paragraph.txt", "baseball.txt"]

    # Generate files with random bytes for benchmarking

    def generate_random_file(size_bytes):
        filename = f"random_{size_bytes}.txt"
        with open(filename, 'wb') as f:
            f.write(bytes([random.randint(0, 255) for _ in range(size_bytes)]))
        print(f"Generated {filename} with {size_bytes} random bytes")

        return filename
 
    message_files = []
    # sizes = [2,4,8,16,32,64,128,128+64, 256,256+64, 256+128, 512]
    sizes = [512]

    for size in sizes:
        message_files.append(generate_random_file(size))
    
    # for message_file in message_files:
    #    run_benchmark(message_file)  
    
     # Load the results
    if not os.path.exists("benchmark_results.csv"):
        print("Error: benchmark_results.csv was not created")
        sys.exit(1)
    
    result = pd.read_csv("benchmark_results.csv")
    create_graphs(result)

if __name__ == "__main__":
    main()