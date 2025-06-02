import subprocess
import time

# Paths to your engines
ENGINE_1 = r"./build/nova"
ENGINE_2 = r"./stockfish/stockfish-ubuntu-x86-64-avx2"



# Engine settings
DEPTH = 6

# Start a new process for each engine
def start_engine(path):
    return subprocess.Popen(
        [path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        bufsize=1
    )

# Send command to engine
def send(engine, command):
    engine.stdin.write(command + "\n")
    engine.stdin.flush()

# Read lines until we get desired response
def wait_for(engine, token):
    while True:
        line = engine.stdout.readline()
        if token in line:
            break

# Get best move from engine
def get_best_move(engine, position):
    send(engine, f"position {position}")
    send(engine, f"go depth {DEPTH}")
    while True:
        line = engine.stdout.readline()
        if line.startswith("bestmove"):
            return line.split()[1]

# Main match loop
def play_game():
    board_state = "startpos"
    move_history = []
    turn = 0

    e1 = start_engine(ENGINE_1)
    e2 = start_engine(ENGINE_2)

    for e in [e1, e2]:
        send(e, "uci")
        wait_for(e, "uciok")
        send(e, "isready")
        wait_for(e, "readyok")
        send(e, "ucinewgame")

    while True:
        position = board_state if not move_history else f"{board_state} moves {' '.join(move_history)}"
        current_engine = e1 if turn % 2 == 0 else e2
        move = get_best_move(current_engine, position)
        if move == "(none)" or move == "0000":
            print("Game over.")
            break
        move_history.append(move)
        print(f"{'White' if turn % 2 == 0 else 'Black'} plays: {move}")
        turn += 1
        if turn > 100:
            print("Draw by move limit.")
            break

    for e in [e1, e2]:
        send(e, "quit")
        e.terminate()

# Run the match
if __name__ == "__main__":
    play_game()
