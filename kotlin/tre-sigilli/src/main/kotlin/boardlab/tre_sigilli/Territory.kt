package boardlab.tre_sigilli

data class TerritoryScore(
    val north: Int,
    val south: Int,
)

enum class GameOutcome {
    NORTH_WINS,
    SOUTH_WINS,
    DRAW,
}

data class CellInfluence(
    val north: Int,
    val south: Int,
)

/**
 * Influence on one cell, including the occupying seal.
 * Uses the same calculation as territorial scoring.
 */
fun cellInfluence(state: State, cell: Int): CellInfluence {
    validateState(state)
    require(cell in 0 until 16) { "Cell outside board" }
    return countInfluence(state, cell)
}

private fun countInfluence(state: State, cell: Int): CellInfluence {
    var north = 0
    var south = 0

    for (origin in 0 until 16) {
        if (origin != cell && !adjacent(origin, cell)) continue

        when (state.cells[origin]) {
            "north" -> north++
            "south" -> south++
        }
    }

    return CellInfluence(north, south)
}

fun territoryScore(state: State): TerritoryScore {
    validateState(state)

    var north = 0
    var south = 0

    for (cell in 0 until 16) {
        val influence = countInfluence(state, cell)

        when {
            influence.north > influence.south -> north++
            influence.south > influence.north -> south++
        }
    }

    return TerritoryScore(north, south)
}

fun gameOutcome(state: State): GameOutcome? {
    validateState(state)

    if (legalMoves(state).isNotEmpty()) return null

    val score = territoryScore(state)

    return when {
        score.north > score.south -> GameOutcome.NORTH_WINS
        score.south > score.north -> GameOutcome.SOUTH_WINS
        else -> GameOutcome.DRAW
    }
}
