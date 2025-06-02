import subprocess
import time

ENGINE_1 = r"./build/nova"
ENGINE_2 = r"./stockfish/stockfish"
DEPTH = 5  # You can raise this to make Stockfish tougher

def start_engine(path):
    return subprocess.Popen(
        [path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True
    )

def send(engine, command):
    engine.stdin.write(command + "\n")
    engine.stdin.flush()

def wait_for(engine, token):
    while True:
        line = engine.stdout.readline()
        if token in line:
            break

def get_best_move(engine, position):
    send(engine, f"position {position}")
    send(engine, f"go depth {DEPTH}")
    while True:
        line = engine.stdout.readline()
        if line.startswith("bestmove"):
            return line.split()[1]

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
        if move == "0000" or move == "(none)":
            if turn % 2 == 0:
                print("❌ Nova (White) lost")
                return False
            else:
                print("✅ Nova (Black) drew or won")
                return True
        move_history.append(move)
        print(f"{'White' if turn % 2 == 0 else 'Black'} plays: {move}")
        turn += 1
        if turn > 100:
            print("✅ Draw by move limit (100)")
            return True

    for e in [e1, e2]:
        send(e, "quit")
        e.terminate()

if __name__ == "__main__":
    game_count = 0
    while True:
        game_count += 1
        print(f"\n🎮 Game {game_count} begins...\n")
        success = play_game()
        if success:
            print(f"\n🏁 Game {game_count} result: Nova survived (draw or win)!")
            break
        else:
            print(f"\n🔁 Game {game_count} result: Nova lost. Retrying...\n")
