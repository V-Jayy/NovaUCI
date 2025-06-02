import subprocess

ENGINE_1 = r"./build/nova"        # Nova
ENGINE_2 = r"./stockfish/stockfish"  # Stockfish
DEPTH = 5  # Depth used for both engines

# Utility functions ---------------------------------------------------------

def start_engine(path):
    return subprocess.Popen(
        [path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True
    )

def send(engine, command):
    """Send a single command to an engine."""
    engine.stdin.write(command + "\n")
    engine.stdin.flush()

def wait_for(engine, token):
    """Read lines until a token is found."""
    while True:
        line = engine.stdout.readline()
        if token in line:
            break

def get_best_move(engine, fen):
    """Ask the given engine for a move from the provided FEN."""
    send(engine, f"position fen {fen}")
    send(engine, f"go depth {DEPTH}")
    while True:
        line = engine.stdout.readline()
        if line.startswith("bestmove"):
            return line.split()[1]

def apply_move(board_engine, fen, move):
    """Apply a move on board_engine and return the resulting FEN.
    If the move is illegal, the returned FEN will equal the input."""
    send(board_engine, f"position fen {fen} moves {move}")
    send(board_engine, "d")
    new_fen = fen
    while True:
        line = board_engine.stdout.readline().strip()
        if line.startswith("Fen: "):
            new_fen = line[5:]
        if line == "Checkers:":
            break
    return new_fen

def init(engine):
    send(engine, "uci")
    wait_for(engine, "uciok")
    send(engine, "isready")
    wait_for(engine, "readyok")
    send(engine, "ucinewgame")


def play_game():
    """Play a single game of Nova vs Stockfish.
    Returns True if Nova wins, False if it loses, None for draw."""

    nova = start_engine(ENGINE_1)
    stock = start_engine(ENGINE_2)
    verifier = start_engine(ENGINE_2)

    for e in (nova, stock, verifier):
        init(e)

    fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    turn = 0

    result = None

    while True:
        engine = nova if turn % 2 == 0 else stock
        name = "Nova" if turn % 2 == 0 else "Stockfish"
        move = get_best_move(engine, fen)

        if move in ("0000", "(none)"):
            if name == "Nova":
                print("❌ Nova has no moves - it loses")
                result = False
            else:
                print("✅ Stockfish has no moves - Nova wins?!")
                result = True
            break

        new_fen = apply_move(verifier, fen, move)
        if new_fen == fen:
            if name == "Nova":
                print(f"Illegal move by Nova: {move}")
                result = False
            else:
                print(f"Illegal move by Stockfish: {move}")
                result = True
            break

        fen = new_fen
        print(f"{name} plays: {move}")

        # Check if side to move has any legal move
        send(verifier, f"position fen {fen}")
        send(verifier, "go depth 1")
        next_move = None
        while True:
            line = verifier.stdout.readline()
            if line.startswith("bestmove"):
                next_move = line.split()[1]
                break
        if next_move in ("0000", "(none)"):
            if name == "Nova":
                print("Stockfish has no moves - Nova wins!")
                result = True
            else:
                print("Nova has no moves - Stockfish wins!")
                result = False
            break

        turn += 1
        if turn > 300:
            print("Move limit reached - declaring draw")
            result = None
            break

    for e in (nova, stock, verifier):
        send(e, "quit")
        e.terminate()

    return result

if __name__ == "__main__":
    game_count = 0
    while True:
        game_count += 1
        print(f"\n🎮 Game {game_count} begins...\n")
        result = play_game()
        if result is True:
            print(f"\n🏁 Game {game_count} result: Nova wins!")
            break
        elif result is False:
            print(f"\n🏁 Game {game_count} result: Nova loses")
            break
        else:
            print(f"\n🏁 Game {game_count} result: Draw - retrying...\n")
