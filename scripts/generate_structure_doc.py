import os

def generate_file_structure(root, prefix=""):
    entries = []
    for item in sorted(os.listdir(root)):
        path = os.path.join(root, item)
        if os.path.isdir(path):
            entries.append(f"{prefix}- {item}/")
            entries += generate_file_structure(path, prefix + "  ")
        else:
            entries.append(f"{prefix}- {item}")
    return entries

def main():
    current_dir = os.getcwd()
    parent_dir = os.path.abspath(os.path.join(current_dir, ".."))
    target_dirs = ["src", "tests"]
    output_lines = ["# Current Code Directory Structure\n"]

    for folder in target_dirs:
        path = os.path.join(parent_dir, folder)
        if os.path.exists(path):
            output_lines.append(f"## `{folder}/`")
            structure = generate_file_structure(path)
            output_lines.extend(structure)
            output_lines.append("")  # Blank line for spacing
        else:
            output_lines.append(f"## `{folder}/` not found.\n")

    docs_dir = os.path.join(parent_dir, "docs")
    os.makedirs(docs_dir, exist_ok=True)
    output_file = os.path.join(docs_dir, "_current_code_dir_structure_.md")

    with open(output_file, "w") as f:
        f.write("\n".join(output_lines))

    print(f"File structure written to {output_file}")

if __name__ == "__main__":
    main()
