package boardlab.tre_sigilli

data class GameScreenState(
    val game: State,
    val selectedCell: Int? = null,
    val legalMoves: List<Move> = legalMoves(game),
)

enum class GameMode {
    LOCAL_TWO_PLAYERS,
    HUMAN_VS_ANDROID,
}

/**
 * Minimal snapshot; legal moves are always recomputed.
 */
data class GameSnapshot(
    val game: State,
    val mode: GameMode,
    val selectedCell: Int?,
)

class GameController(
    initialMode: GameMode = GameMode.LOCAL_TWO_PLAYERS,
    private val androidStrategy: AlphaBetaStrategy = AlphaBetaStrategy(depth = 2),
) {
    var mode: GameMode = initialMode
        private set

    var screen = GameScreenState(State(List(16) { null }, 0))
        private set

    fun snapshot(): GameSnapshot =
        GameSnapshot(
            game = screen.game,
            mode = mode,
            selectedCell = screen.selectedCell,
        )

    fun restore(snapshot: GameSnapshot) {
        validateState(snapshot.game)

        // A human-vs-Android checkpoint is always on NORTH's turn,
        // unless the game has ended.
        require(
            snapshot.mode != GameMode.HUMAN_VS_ANDROID ||
                snapshot.game.turn % 2 == 0 ||
                legalMoves(snapshot.game).isEmpty()
        ) { "Android turn cannot be restored as a human turn" }

        val moves = legalMoves(snapshot.game)
        val selected = snapshot.selectedCell

        require(
            selected == null ||
                (
                    snapshot.game.turn >= 6 &&
                        moves.filterIsInstance<Slide>()
                            .any { it.origin == selected }
                )
        ) { "Invalid selected cell" }

        // Commit only after all validation has succeeded.
        mode = snapshot.mode
        screen = GameScreenState(
            game = snapshot.game,
            selectedCell = selected,
        )
    }

    fun newGame(mode: GameMode = this.mode) {
        this.mode = mode
        screen = GameScreenState(State(List(16) { null }, 0))
    }

    fun tap(cell: Int) {
        require(cell in 0..15) { "Cell outside board" }

        val state = screen.game
        val moves = screen.legalMoves

        if (moves.isEmpty()) return
        if (mode == GameMode.HUMAN_VS_ANDROID && state.turn % 2 != 0) return

        if (state.turn < 6) {
            val move = Place(cell)
            if (move in moves) completeHumanMove(move)
            return
        }

        val selected = screen.selectedCell
        if (selected != null) {
            val move = Slide(selected, cell)
            if (move in moves) {
                completeHumanMove(move)
                return
            }
        }

        val validOrigins = moves.filterIsInstance<Slide>()
            .map { it.origin }
            .toSet()

        screen = screen.copy(
            selectedCell = cell.takeIf { it in validOrigins },
        )
    }

    private fun completeHumanMove(move: Move) {
        val next = applyMove(screen.game, move)

        if (mode != GameMode.HUMAN_VS_ANDROID || legalMoves(next).isEmpty()) {
            screen = GameScreenState(next)
            return
        }

        // The human always plays NORTH; a completed NORTH move
        // gives SOUTH its turn. Apply exactly one offline AI move.
        check(next.turn % 2 == 1)
        val reply = androidStrategy.chooseMove(next)
        screen = GameScreenState(applyMove(next, reply))
    }
}
