# Nova Chess Engine

Nova is a simple chess engine implementing the UCI protocol. It currently supports legal move generation including castling and en passant. Evaluation and search are left for future work.

## Building

```
mkdir build
cd build
cmake ..
make
```

If building with Visual Studio on Windows fails due to long file paths,
consider using a shorter source path or the Ninja generator:

```
cmake -G Ninja ..
ninja
```

Alternatively run the provided `cmake.bat` script which configures a Ninja
build in a short temporary directory and compiles the `nova` executable. This
avoids the long path limitation of some Windows setups:

```
cmake.bat
```

The resulting executable `nova` can be used with any UCI compatible chess GUI.

## Usage

Run the engine and communicate using UCI commands. Example:

```
./nova
uci
isready
position startpos
go
```

The engine will respond with a (random) legal move.
