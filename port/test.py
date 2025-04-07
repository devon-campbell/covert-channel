def count_byte_differences(file1_path, file2_path):
    try:
        with open(file1_path, 'rb') as file1, open(file2_path, 'rb') as file2:
            file1_bytes = file1.read()
            file2_bytes = file2.read()

        # Compare byte by byte
        differences = sum(b1 != b2 for b1, b2 in zip(file1_bytes, file2_bytes))

        # Account for length differences
        differences += abs(len(file1_bytes) - len(file2_bytes))

        return differences
    except FileNotFoundError as e:
        print(f"Error: {e}")
        return None

if __name__ == "__main__":
    file1 = input("Enter the path to the first file: ")
    file2 = input("Enter the path to the second file: ")
    diff_count = count_byte_differences(file1, file2)
    if diff_count is not None:
        try:
            with open(file1, 'rb') as f:
                file1_size = len(f.read())
            percentage_diff = (diff_count / file1_size) * 100 if file1_size > 0 else 0
            print(f"Number of differing bytes: {diff_count}")
            print(f"Percentage difference from the first file: {percentage_diff:.2f}%")
        except FileNotFoundError as e:
            print(f"Error: {e}")