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

fun territoryScore(state: State): TerritoryScore {
    validateState(state)

    var north = 0
    var south = 0

    for (cell in 0 until 16) {
        var northInfluence = 0
        var southInfluence = 0

        for (origin in 0 until 16) {
            if (origin != cell && !adjacent(origin, cell)) continue

            when (state.cells[origin]) {
                "north" -> northInfluence++
                "south" -> southInfluence++
            }
        }

        when {
            northInfluence > southInfluence -> north++
            southInfluence > northInfluence -> south++
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
