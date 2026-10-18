package boardlab.tre_sigilli

import java.io.File

fun encode(move: Move): String =
    when (move) {
        is Place -> "P${move.cell}"
        is Slide -> "S${move.origin}:${move.destination}"
    }

fun decodeMove(encoded: String): Move =
    when {
        encoded.startsWith("P") ->
            Place(encoded.drop(1).toInt())
        encoded.startsWith("S") -> {
            val parts = encoded.drop(1).split(":")
            require(parts.size == 2)
            Slide(parts[0].toInt(), parts[1].toInt())
        }
        else -> error("Unknown move: $encoded")
    }

fun decodeCells(encoded: String): List<String?> =
    encoded.split(",").map { if (it == ".") null else it }

fun verifyTransitions(path: String) {
    val lines = File(path).readLines()
    require(lines.isNotEmpty())

    for (line in lines) {
        val fields = line.split("\t")
        require(fields.size == 6)

        val before = State(decodeCells(fields[2]), fields[1].toInt())
        val move = decodeMove(fields[3])
        val expected = State(decodeCells(fields[5]), fields[4].toInt())

        val originalCells = before.cells.toList()
        val actual = applyMove(before, move)

        check(actual == expected) {
            "TRANSITION FAIL ${fields[0]} ${fields[3]}: expected=$expected actual=$actual"
        }
        check(before.cells == originalCells) { "Original state mutated" }
    }

    println("TRANSITIONS_VERIFIED=${lines.size}")
    println("APPLY_MOVE_PARITY=PASS")
}

fun verifyInvalidMoves(path: String) {
    val lines = File(path).readLines()
    require(lines.isNotEmpty())

    for (line in lines) {
        val fields = line.split("\t")
        require(fields.size == 4)

        val state = State(decodeCells(fields[2]), fields[1].toInt())
        val snapshot = state.copy(cells = state.cells.toList())
        val move = decodeMove(fields[3])

        val result = runCatching { applyMove(state, move) }

        check(result.exceptionOrNull() is IllegalArgumentException) {
            "ILLEGAL MOVE NOT REJECTED: ${fields[0]} ${fields[3]}"
        }
        check(state == snapshot) {
            "STATE MUTATED AFTER REJECTION: ${fields[0]}"
        }
    }

    println("ORACLE_INVALID_MOVES_REJECTED=${lines.size}")

    val initial = State(List(16) { null }, 0)
    val extraInvalid = listOf<Move>(
        Place(-1),
        Place(16),
        Slide(0, 1),
        Slide(0, 4)
    )

    for (move in extraInvalid) {
        val snapshot = initial.copy(cells = initial.cells.toList())
        check(runCatching { applyMove(initial, move) }.exceptionOrNull()
            is IllegalArgumentException)
        check(initial == snapshot)
    }

    println("EXTRA_INVALID_MOVES_REJECTED=${extraInvalid.size}")
    println("ILLEGAL_MOVE_GATE=PASS")
}

fun main(args: Array<String>) {
    require(args.size == 3)

    verifyTransitions(args[1])
    verifyInvalidMoves(args[2])

    val lines = File(args[0]).readLines()
    var total = 0

    for (line in lines) {
        val fields = line.split("\t")
        require(fields.size == 4)

        val cells = fields[2].split(",").map {
            if (it == ".") null else it
        }
        val state = State(cells, fields[1].toInt())

        val actual = legalMoves(state).map(::encode)
        val expected = if (fields[3].isEmpty()) {
            emptyList()
        } else {
            fields[3].split(",")
        }

        check(actual == expected) {
            "PARITY FAIL ${fields[0]}: expected=$expected actual=$actual"
        }

        println("CASE=${fields[0]} MOVES=${actual.size} PASS")
        total += actual.size
    }

    val empty = List<String?>(16) { null }
    val invalidStates = listOf(
        State(empty, -1),
        State(empty, 19),
        State(empty.take(15), 0),
        State(listOf("unknown") + empty.drop(1), 0),
        State(empty, 1),
        State(listOf("north", "north") + empty.drop(2), 1),
        State(listOf("south") + empty.drop(1), 1)
    )

    for (state in invalidStates) {
        val rejected = runCatching { legalMoves(state) }.isFailure
        check(rejected) { "INVALID STATE ACCEPTED: $state" }
    }

    println("INVALID_STATES_REJECTED=${invalidStates.size}")
    println("STATE_VALIDATION=PASS")

    check(lines.size == 5)
    check(total == 38)

    println("LEGAL_MOVES_PARITY=PASS")
    println("TOTAL_MOVES=$total")
}
