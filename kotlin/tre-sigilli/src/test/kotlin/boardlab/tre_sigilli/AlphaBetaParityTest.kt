package boardlab.tre_sigilli

import java.io.File

private fun decodeAiMove(encoded: String): Move =
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

fun main(args: Array<String>) {
    require(args.size == 1)

    val lines = File(args[0]).readLines()
    check(lines.size == 5)

    val strategy = AlphaBetaStrategy(depth = 2)
    var playable = 0
    var terminal = 0

    for (line in lines) {
        val fields = line.split('\t')
        require(fields.size == 4)

        val id = fields[0]
        val state = State(
            cells = fields[2].split(",").map {
                if (it == ".") null else it
            },
            turn = fields[1].toInt(),
        )

        val original = state.copy(cells = state.cells.toList())
        val moves = legalMoves(state)

        if (fields[3] == "TERMINAL") {
            check(moves.isEmpty()) { "$id: expected terminal state" }

            check(
                runCatching { strategy.chooseMove(state) }
                    .exceptionOrNull() is IllegalArgumentException
            ) { "$id: terminal state was not rejected" }

            terminal++
            println("CASE=$id TERMINAL_REJECTED=PASS")
        } else {
            val expected = decodeAiMove(fields[3])
            check(expected in moves) { "$id: oracle move is illegal" }

            val actual = strategy.chooseMove(state)
            check(actual == expected) {
                "$id: expected=$expected actual=$actual"
            }
            check(strategy.chooseMove(state) == actual) {
                "$id: nondeterministic result"
            }

            playable++
            println("CASE=$id AI_MOVE=$actual PASS")
        }

        check(state == original) { "$id: original state mutated" }
    }

    check(playable > 0)
    check(terminal > 0)

    println("AI_PLAYABLE_CASES=$playable")
    println("AI_TERMINAL_CASES=$terminal")
    println("ALPHA_BETA_CANONICAL_PARITY=PASS")
}
