"""Text presentation for interactive Tre Sigilli games."""

from collections.abc import Callable

from boardlab.engine.game import Outcome
from boardlab.engine.match import Match
from boardlab.games.tre_sigilli import Move, Place, Player, Slide, State, TreSigilli
from boardlab.strategies.alpha_beta import AlphaBetaStrategy
from boardlab.strategies.tre_sigilli_evaluator import evaluate_territory


def render_board(state: State) -> str:
    """Render a 4x4 board with numbered empty cells."""
    if len(state.cells) != 16:
        raise ValueError("Expected a 4x4 board")

    symbols = {
        None: None,
        Player.NORTH: " N",
        Player.SOUTH: " S",
    }

    rows = []

    for row in range(4):
        cells = []

        for column in range(4):
            index = row * 4 + column
            owner = state.cells[index]
            cells.append(symbols[owner] or f"{index:02d}")

        rows.append(" | ".join(cells))

    return "\n".join(rows)


def parse_move(text: str, game: TreSigilli, state: State) -> Move:
    """Parse and validate a human move against the game rules."""
    if game.is_terminal(state):
        raise ValueError("Game is already over")

    parts = text.split()

    if not parts or any(not part.isascii() or not part.isdecimal() for part in parts):
        raise ValueError("Invalid move format")

    if state.turn < 6:
        if len(parts) != 1:
            raise ValueError("Expected one cell index")
        move: Move = Place(int(parts[0]))
    else:
        if len(parts) != 2:
            raise ValueError("Expected origin and destination")
        move = Slide(int(parts[0]), int(parts[1]))

    if move not in game.legal_moves(state):
        raise ValueError("Illegal move")

    return move


def play_human_vs_ai(
    human: Player,
    read: Callable[[str], str],
    write: Callable[[str], None],
) -> bool:
    """Play one match; return False on EOF, True on completion."""
    if not isinstance(human, Player):
        raise ValueError("Invalid human player")

    game = TreSigilli()
    match = Match(game)
    ai = AlphaBetaStrategy(evaluate_territory, depth=2)

    write(
        f"Human: {human.name} | AI: "
        f"{(Player.SOUTH if human is Player.NORTH else Player.NORTH).name}"
    )
    write("Placement: enter a cell (0-15). Movement: enter origin destination.")

    while not game.is_terminal(match.state):
        state = match.state
        north, south = game.territory_scores(state)
        write(render_board(state))
        write(f"Turn {state.turn}/18 | Territory NORTH={north} SOUTH={south}")

        if game.current_player(state) is human:
            try:
                text = read("Your move: ")
            except EOFError:
                write("Game interrupted: end of input.")
                return False

            try:
                move = parse_move(text, game, state)
            except ValueError as exc:
                write(f"Invalid move: {exc}")
                continue

            match.play(move)
        else:
            move = ai.choose_move(game, state)
            write(f"AI move: {move}")
            match.play(move)

    state = match.state
    north, south = game.territory_scores(state)
    write(render_board(state))
    write(f"Final territory: NORTH={north} SOUTH={south}")

    outcome = game.outcome(state, human)
    if outcome is Outcome.WIN:
        write("Result: HUMAN WIN")
    elif outcome is Outcome.LOSS:
        write("Result: AI WIN")
    else:
        write("Result: DRAW")

    return True


def main() -> int:
    """Run one interactive Human vs AI match."""
    print("Tre Sigilli — Human vs AI")
    print("Choose your side: N = NORTH, S = SOUTH")

    try:
        while True:
            choice = input("Your side [N/S]: ").strip().upper()

            if choice in ("N", "S"):
                break

            print("Invalid side. Enter N or S.")

        human = Player.NORTH if choice == "N" else Player.SOUTH
        completed = play_human_vs_ai(human, input, print)
        return 0 if completed else 1

    except (EOFError, KeyboardInterrupt):
        print("\nGame interrupted.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
