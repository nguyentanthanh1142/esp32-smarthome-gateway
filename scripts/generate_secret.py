Import("env")
import os
import shutil

source_file = "include/secret.h.example"
target_file = "include/secret.h"

if not os.path.exists(target_file):
    if os.path.exists(source_file):
        print("\n" + "="*60)
        print(f"[AUTO] {target_file} not found.")
        print(f"[AUTO] Auto-generating from {source_file}...")
        shutil.copy(source_file, target_file)
        print(f"[AUTO] SUCCESS! Please edit '{target_file}' with your real credentials.")
        print("="*60 + "\n")
    else:
        print("\n" + "="*60)
        print(f"[ERROR] {source_file} is missing! Cannot generate {target_file}.")
        print("="*60 + "\n")
else:
    print(f"\n[INFO] {target_file} already exists. Using current configuration.\n")