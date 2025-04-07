import os
import random
import string


with open("big.txt", "w") as f:
    for _ in range(1024):  # Calculate the number of lines needed
        random_chars = ''.join(random.choices(string.ascii_letters + string.digits, k=15))
        f.write(random_chars + '\n')  # Write 15 random characters followed by a newline