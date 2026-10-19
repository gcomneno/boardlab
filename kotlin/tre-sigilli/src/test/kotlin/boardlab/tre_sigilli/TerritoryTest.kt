package boardlab.tre_sigilli

fun main() {
    val initial = State(List(16) { null }, 0)

    check(territoryScore(initial) == TerritoryScore(0, 0))
    check(gameOutcome(initial) == null)

    val afterFirstMove = applyMove(initial, Place(0))
    check(territoryScore(afterFirstMove) == TerritoryScore(3, 0))
    check(gameOutcome(afterFirstMove) == null)

    val placement = listOf(0, 15, 1, 14, 4, 11)
        .fold(initial) { state, cell ->
            applyMove(state, Place(cell))
        }

    check(placement.turn == 6)
    check(gameOutcome(placement) == null)

    val terminal = State(
        cells = placement.cells,
        turn = 18,
    )

    check(legalMoves(terminal).isEmpty())

    val score = territoryScore(terminal)
    val expected = when {
        score.north > score.south -> GameOutcome.NORTH_WINS
        score.south > score.north -> GameOutcome.SOUTH_WINS
        else -> GameOutcome.DRAW
    }

    check(gameOutcome(terminal) == expected)

    println("TERRITORY_SCORE=PASS")
    println("TERMINAL_OUTCOME=PASS")
}
