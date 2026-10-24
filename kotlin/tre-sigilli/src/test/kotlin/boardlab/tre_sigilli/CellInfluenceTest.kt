package boardlab.tre_sigilli

fun main() {
    val empty = State(List(16) { null }, 0)

    for (cell in 0 until 16) {
        check(cellInfluence(empty, cell) == CellInfluence(0, 0))
    }

    val first = applyMove(empty, Place(5))
    check(cellInfluence(first, 5) == CellInfluence(1, 0))
    check(cellInfluence(first, 1) == CellInfluence(1, 0))
    check(cellInfluence(first, 4) == CellInfluence(1, 0))
    check(cellInfluence(first, 6) == CellInfluence(1, 0))
    check(cellInfluence(first, 9) == CellInfluence(1, 0))
    check(cellInfluence(first, 0) == CellInfluence(0, 0))

    val contested = applyMove(first, Place(6))
    check(cellInfluence(contested, 5) == CellInfluence(1, 1))
    check(cellInfluence(contested, 6) == CellInfluence(1, 1))

    for (state in listOf(empty, first, contested)) {
        var north = 0
        var south = 0

        for (cell in 0 until 16) {
            val influence = cellInfluence(state, cell)
            when {
                influence.north > influence.south -> north++
                influence.south > influence.north -> south++
            }
        }

        check(territoryScore(state) == TerritoryScore(north, south))
    }

    for (cell in listOf(-1, 16)) {
        var rejected = false
        try {
            cellInfluence(empty, cell)
        } catch (_: IllegalArgumentException) {
            rejected = true
        }
        check(rejected)
    }

    println("CELL_INFLUENCE_REGRESSION=PASS")
}
