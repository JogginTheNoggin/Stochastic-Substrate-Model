import os
import json

# ----------------------------------------
# Setup paths
# ----------------------------------------

script_dir = os.path.dirname(os.path.abspath(__file__))
workspace_folder = os.path.abspath(os.path.join(script_dir, ".."))
build_dir = "build/AthenaTests"
vscode_dir = os.path.join(workspace_folder, ".vscode")
launch_file = os.path.join(vscode_dir, "launch.json")
build_path = os.path.join(workspace_folder, build_dir)

print(f" Workspace: {workspace_folder}")
print(f" Looking in build path: {build_path}")

# ----------------------------------------
# Load or create launch.json
# ----------------------------------------

if os.path.exists(launch_file):
    with open(launch_file, "r") as f:
        data = json.load(f)
    print(f" Loaded existing launch.json with {len(data.get('configurations', []))} configurations.")
else:
    data = {"version": "0.2.0", "configurations": []}
    print(" Created new launch.json structure.")

# ----------------------------------------
# Remove existing configs pointing to build/AthenaTests
# ----------------------------------------

def is_test_program(program):
    if not isinstance(program, str):
        return False
    normalized = program.replace("\\", "/").lower()
    return build_dir.replace("\\", "/").lower() in normalized

preserved_configs = [
    cfg for cfg in data["configurations"]
    if not is_test_program(cfg.get("program", ""))
]

print(f" Preserved {len(preserved_configs)} non-test configurations.")

# ----------------------------------------
# Find all .exe files in build/AthenaTests
# ----------------------------------------

if not os.path.exists(build_path):
    print(f" Build directory does not exist: {build_path}")
    exit(1)

all_files = os.listdir(build_path)
print(f" Found {len(all_files)} files in build/AthenaTests/: {all_files}")

test_executables = [
    f for f in all_files
    if f.endswith(".exe") and os.path.isfile(os.path.join(build_path, f))
]

print(f" Detected {len(test_executables)} test executables: {test_executables}")

# ----------------------------------------
# Create new debug configs
# ----------------------------------------

test_configs = []
for exe in test_executables:
    exe_path = os.path.join(build_dir, exe).replace("\\", "/")
    print(f" Generating config for: {exe_path}")

    config = {
        "name": f"Debug {exe}",
        "type": "cppdbg",
        "request": "launch",
        "program": f"${{workspaceFolder}}/{exe_path}",
        "args": [],
        "stopAtEntry": False,
        "cwd": f"${{workspaceFolder}}/{build_dir}",
        "environment": [],
        "externalConsole": True,
        "MIMode": "gdb",
        "setupCommands": [
            { "text": "-gdb-set new-console on" },
            { "text": "-enable-pretty-printing" }
        ],
        "preLaunchTask": "build"
    }

    test_configs.append(config)

# ----------------------------------------
# Save launch.json
# ----------------------------------------

data["configurations"] = preserved_configs + test_configs
os.makedirs(vscode_dir, exist_ok=True)

with open(launch_file, "w") as f:
    json.dump(data, f, indent=2)

print(f" Wrote {len(test_configs)} test debug configuration(s) to {launch_file}")
