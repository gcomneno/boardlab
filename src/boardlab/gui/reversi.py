"""Local two-player Tkinter interface for the existing Reversi engine."""

import sys
import tkinter as tk
from functools import partial
from tkinter import ttk

from boardlab.engine.game import Outcome
from boardlab.games.reversi import Move, Pass, Place, Player, Reversi


class ReversiApp:
    """Present game state and delegate every transition to Reversi."""

    def __init__(self, root: tk.Tk) -> None:
        self.game = Reversi()
        self.state = self.game.initial_state()
        root.title("BoardLab — Reversi 6×6")
        root.resizable(False, False)

        frame = ttk.Frame(root, padding=16)
        frame.grid()
        ttk.Label(
            frame,
            text="Reversi 6×6 — due giocatori locali",
            font=("TkDefaultFont", 14, "bold"),
        ).grid(row=0, column=0, columnspan=2, pady=(0, 8))

        self.status = tk.StringVar(master=root)
        ttk.Label(frame, textvariable=self.status).grid(
            row=1,
            column=0,
            columnspan=2,
            pady=(0, 8),
        )

        board = ttk.Frame(frame)
        board.grid(row=2, column=0, columnspan=2)
        self.cells: list[tk.Button] = []
        for index in range(self.game.CELL_COUNT):
            button = tk.Button(
                board,
                width=3,
                height=1,
                font=("TkDefaultFont", 26),
                command=partial(self.play, Place(index)),
                takefocus=True,
            )
            button.grid(row=index // self.game.SIZE, column=index % self.game.SIZE)
            self.cells.append(button)

        ttk.Label(
            frame,
            text="Le caselle evidenziate indicano le mosse legali.",
        ).grid(row=3, column=0, columnspan=2, pady=8)
        self.pass_button = ttk.Button(
            frame,
            text="Passa",
            command=partial(self.play, Pass()),
        )
        self.pass_button.grid(row=4, column=0, sticky="w")
        ttk.Button(frame, text="Nuova partita", command=self.new_game).grid(
            row=4,
            column=1,
            sticky="e",
        )
        self.refresh()

    def new_game(self) -> None:
        """Restore the opening position and all controls."""
        self.state = self.game.initial_state()
        self.refresh()

    def play(self, move: Move) -> None:
        """Ignore unavailable actions without consuming a turn."""
        if move not in self.game.legal_moves(self.state):
            return
        self.state = self.game.apply_move(self.state, move)
        self.refresh()

    def refresh(self) -> None:
        """Render legal actions, counts, and the engine's terminal result."""
        moves = self.game.legal_moves(self.state)
        available = {move.cell for move in moves if isinstance(move, Place)}
        for index, button in enumerate(self.cells):
            owner = self.state.cells[index]
            foreground = "#111111" if owner is Player.BLACK else "#ffffff"
            background = "#d6bc58" if index in available else "#28754b"
            button.configure(
                text="●" if owner is not None else ("·" if index in available else " "),
                foreground=foreground,
                disabledforeground=foreground,
                background=background,
                activebackground=background,
                activeforeground=foreground,
                state=tk.NORMAL if index in available else tk.DISABLED,
            )

        self.pass_button.configure(
            state=tk.NORMAL if Pass() in moves else tk.DISABLED,
        )
        black = self.state.cells.count(Player.BLACK)
        white = self.state.cells.count(Player.WHITE)
        score = f"Nero {black} — Bianco {white}"

        if self.game.is_terminal(self.state):
            outcome = self.game.outcome(self.state, Player.BLACK)
            if outcome is Outcome.DRAW:
                message = "Pareggio"
            elif outcome is Outcome.WIN:
                message = "Vince Nero"
            else:
                message = "Vince Bianco"
            self.status.set(f"{message} | {score}")
        else:
            player = self.game.current_player(self.state)
            name = "Nero" if player is Player.BLACK else "Bianco"
            instruction = " — premi Passa" if Pass() in moves else ""
            self.status.set(f"Turno: {name}{instruction} | {score}")


def main() -> int:
    """Open a local game; report an unavailable Tk display clearly."""
    try:
        root = tk.Tk()
    except tk.TclError as exc:
        print(f"Impossibile aprire la GUI Tkinter: {exc}", file=sys.stderr)
        return 1
    ReversiApp(root)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
