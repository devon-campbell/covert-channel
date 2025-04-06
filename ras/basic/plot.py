# Reads output from benchmark.c to determine RAS size

import matplotlib.pyplot as plt

def read_and_plot(filename):
    values = []
    
    with open(filename, "r") as file:
        values = [int(line.strip()) for line in file]

    plt.figure(figsize=(8, 5))
    plt.plot(range(1, len(values)+1), values, marker="o", linestyle="-", color="b")
    plt.xlabel("Recursion Depth")
    plt.ylabel("Cycles")
    plt.title("Recursion Depth vs. Execution Time")
    plt.legend()
    plt.grid(True)
    plt.savefig('results.png')

read_and_plot("results.txt")
