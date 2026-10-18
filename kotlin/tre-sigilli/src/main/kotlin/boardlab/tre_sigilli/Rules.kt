package boardlab.tre_sigilli

fun adjacent(a: Int, b: Int): Boolean =
    kotlin.math.abs(a / 4 - b / 4) +
    kotlin.math.abs(a % 4 - b % 4) == 1

fun maneuverMoves(state: State): List<Move> {
    val player = if (state.turn % 2 == 0) "north" else "south"
    val moves = mutableListOf<Move>()

    for (origin in 0 until 16) {
        if (state.cells[origin] != player) continue

        for (destination in 0 until 16) {
            if (state.cells[destination] == null && adjacent(origin, destination)) {
                moves.add(Slide(origin, destination))
            }
        }
    }

    return moves
}

fun validateState(state: State) {
    require(state.cells.size == 16) { "Board must have 16 cells" }
    require(state.turn in 0..18) { "Turn outside 0..18" }
    require(state.cells.all { it == null || it == "north" || it == "south" }) {
        "Unknown player"
    }

    val expectedNorth = minOf((state.turn + 1) / 2, 3)
    val expectedSouth = minOf(state.turn / 2, 3)

    require(state.cells.count { it == "north" } == expectedNorth) {
        "Invalid NORTH count"
    }
    require(state.cells.count { it == "south" } == expectedSouth) {
        "Invalid SOUTH count"
    }
}

fun legalMoves(state: State): List<Move> {
    validateState(state)

    if (state.turn == 18) return emptyList()

    if (state.turn < 6) {
        return (0 until 16)
            .filter { state.cells[it] == null }
            .map { Place(it) }
    }

    return maneuverMoves(state)
}

fun applyMove(state: State, move: Move): State {
    validateState(state)
    require(move in legalMoves(state)) { "Illegal move" }

    val cells = state.cells.toMutableList()
    val player = if (state.turn % 2 == 0) "north" else "south"

    when (move) {
        is Place -> cells[move.cell] = player
        is Slide -> {
            cells[move.origin] = null
            cells[move.destination] = player
        }
    }

    return State(cells.toList(), state.turn + 1).also(::validateState)
}
