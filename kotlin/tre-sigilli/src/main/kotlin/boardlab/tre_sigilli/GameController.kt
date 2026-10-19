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

class GameController(
    initialMode: GameMode = GameMode.LOCAL_TWO_PLAYERS,
    private val androidStrategy: AlphaBetaStrategy = AlphaBetaStrategy(depth = 2),
) {
    var mode: GameMode = initialMode
        private set

    var screen = GameScreenState(State(List(16) { null }, 0))
        private set

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
