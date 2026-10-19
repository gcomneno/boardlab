package boardlab.tre_sigilli

/**
 * Deterministic depth-limited alpha-beta search.
 *
 * The first equally valued legal move wins ties, matching the
 * canonical Python implementation.
 */
class AlphaBetaStrategy(private val depth: Int = 2) {
    init {
        require(depth >= 1) { "Depth must be positive" }
    }

    fun chooseMove(state: State): Move {
        val moves = legalMoves(state)
        require(moves.isNotEmpty()) { "No legal moves" }

        val rootPlayer = if (state.turn % 2 == 0) "north" else "south"

        fun evaluate(position: State): Double {
            val result = gameOutcome(position)

            if (result != null) {
                return when (result) {
                    GameOutcome.NORTH_WINS ->
                        if (rootPlayer == "north") {
                            Double.POSITIVE_INFINITY
                        } else {
                            Double.NEGATIVE_INFINITY
                        }

                    GameOutcome.SOUTH_WINS ->
                        if (rootPlayer == "south") {
                            Double.POSITIVE_INFINITY
                        } else {
                            Double.NEGATIVE_INFINITY
                        }

                    GameOutcome.DRAW -> 0.0
                }
            }

            val score = territoryScore(position)
            val difference = score.north - score.south

            return if (rootPlayer == "north") {
                difference.toDouble()
            } else {
                -difference.toDouble()
            }
        }

        fun search(
            position: State,
            remaining: Int,
            lower: Double,
            upper: Double,
        ): Double {
            if (gameOutcome(position) != null || remaining == 0) {
                return evaluate(position)
            }

            val maximizing =
                (position.turn % 2 == 0) == (rootPlayer == "north")

            var alpha = lower
            var beta = upper
            var best = if (maximizing) {
                Double.NEGATIVE_INFINITY
            } else {
                Double.POSITIVE_INFINITY
            }

            for (move in legalMoves(position)) {
                val value = search(
                    applyMove(position, move),
                    remaining - 1,
                    alpha,
                    beta,
                )

                if (maximizing) {
                    best = maxOf(best, value)
                    alpha = maxOf(alpha, best)
                } else {
                    best = minOf(best, value)
                    beta = minOf(beta, best)
                }

                if (alpha >= beta) break
            }

            return best
        }

        var bestMove = moves.first()
        var bestValue = Double.NEGATIVE_INFINITY
        var alpha = Double.NEGATIVE_INFINITY

        for (move in moves) {
            val value = search(
                applyMove(state, move),
                depth - 1,
                alpha,
                Double.POSITIVE_INFINITY,
            )

            if (value > bestValue) {
                bestValue = value
                bestMove = move
            }

            alpha = maxOf(alpha, bestValue)
        }

        return bestMove
    }
}
