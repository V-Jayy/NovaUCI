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

# ----------------------------
# ✅ Build Stockfish
# ----------------------------
echo "🧱 Building Stockfish from source..."
cd "STOCKFISH SRC/src"
make build ARCH=x86-64-modern -j$(nproc)
cd ../..

mkdir -p stockfish
cp "STOCKFISH SRC/src/stockfish" stockfish/stockfish
chmod +x stockfish/stockfish
echo "✅ Stockfish built and moved to ./stockfish"
