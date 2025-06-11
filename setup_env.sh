#!/bin/bash
set -e

echo "🔧 Updating packages..."
apt update

echo "📦 Installing required build tools..."
apt install -y build-essential cmake g++ make git python3 python3-pip

# ----------------------------
# ✅ Build Nova
# ----------------------------
echo "🧹 Cleaning build directory..."
rm -rf build
mkdir build
cd build
cmake ..
make -j$(nproc)
cd ..
echo "✅ Nova built."

