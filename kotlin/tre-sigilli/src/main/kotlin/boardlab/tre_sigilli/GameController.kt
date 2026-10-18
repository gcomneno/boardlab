package boardlab.tre_sigilli

data class GameScreenState(
    val game: State,
    val selectedCell: Int? = null,
    val legalMoves: List<Move> = legalMoves(game),
)

class GameController {
    var screen = GameScreenState(State(List(16) { null }, 0))
        private set

    fun newGame() {
        screen = GameScreenState(State(List(16) { null }, 0))
    }

    fun tap(cell: Int) {
        require(cell in 0..15) { "Cell outside board" }

        val state = screen.game
        val moves = screen.legalMoves

        if (state.turn < 6) {
            val move = Place(cell)
            if (move in moves) {
                screen = GameScreenState(applyMove(state, move))
            }
            return
        }

        val selected = screen.selectedCell
        if (selected != null) {
            val move = Slide(selected, cell)
            if (move in moves) {
                screen = GameScreenState(applyMove(state, move))
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
}
