# Nova Chess Engine 🌑

Nova is a high-performance, UCI-compliant chess engine written in C++20. It combines advanced bitboard techniques with modern search algorithms to provide a powerful and efficient chess AI.

## Features 🚀

- **UCI Protocol Support**: Fully compatible with popular chess GUIs like Arena, Cute Chess, and Nibbler.
- **Advanced Search**: Features Iterative Deepening, Alpha-Beta pruning, PVS (Principal Variation Search), and SEE (Static Exchange Evaluation).
- **Efficient Evaluation**: Hand-crafted evaluation engine optimized for speed and positional understanding.
- **Bitboard Representation**: High-performance move generation using modern bitboard techniques.
- **Optimization**: Built for speed with C++20 and optimized for modern CPU architectures.

## Building 🛠️

To build Nova, use the provided `build.bat` script (Windows/MSVC):

```powershell
.\build.bat
```

The output binary `nova.exe` will be generated in the root directory.

## Usage ♟️

Nova is a UCI engine and requires a chess GUI to play against. Alternatively, you can run it directly in a terminal:

```powershell
.\nova.exe
```

Type `uci` to initialize the protocol, and `go depth 10` to start a search.

## Author ✍️

- **JoshK**
- **Antigravity**

## License 📜

This project is licensed under the MIT License - see the LICENSE file for details (if applicable).
