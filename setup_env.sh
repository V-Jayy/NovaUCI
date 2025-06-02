#!/bin/bash
set -e

echo "🔧 Updating system packages..."
apt update

echo "📦 Installing dependencies..."
apt install -y build-essential cmake g++ make git wget unzip python3 python3-pip

echo "🧹 Cleaning up any stale build directory..."
rm -rf build  # 💥 This clears any old CMakeCache.txt

echo "📁 Creating clean build directory..."
mkdir build
cd build

echo "⚙️ Running CMake..."
cmake ..

echo "🧱 Building Nova engine..."
make -j$(nproc)

echo "✅ Nova built successfully."
