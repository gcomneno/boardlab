package boardlab.tre_sigilli

import java.io.File

fun main(args: Array<String>) {
    require(args.size == 1)

    val lines = File(args[0]).readLines()
    check(lines.size == 5) {
        "Expected five canonical fixture cases"
    }

    for (line in lines) {
        val fields = line.split('\t')
        require(fields.size == 7)

        val id = fields[0]
        val turn = fields[1].toInt()
        val cells = fields[2].split(',').map {
            if (it == ".") null else it
        }
        val state = State(cells, turn)

        val expectedScore = TerritoryScore(
            north = fields[3].toInt(),
            south = fields[4].toInt(),
        )
        val expectedTerminal = fields[5].toBooleanStrict()
        val expectedOutcome = when (fields[6]) {
            "ONGOING" -> null
            "NORTH_WINS" -> GameOutcome.NORTH_WINS
            "SOUTH_WINS" -> GameOutcome.SOUTH_WINS
            "DRAW" -> GameOutcome.DRAW
            else -> error("Unknown expected outcome: ${fields[6]}")
        }

        val actualScore = territoryScore(state)
        check(actualScore == expectedScore) {
            "$id: score expected=$expectedScore actual=$actualScore"
        }

        val actualTerminal = legalMoves(state).isEmpty()
        check(actualTerminal == expectedTerminal) {
            "$id: terminal expected=$expectedTerminal actual=$actualTerminal"
        }

        val actualOutcome = gameOutcome(state)
        check(actualOutcome == expectedOutcome) {
            "$id: outcome expected=$expectedOutcome actual=$actualOutcome"
        }

        println(
            "CASE=$id SCORE=${actualScore.north}:${actualScore.south} " +
                "TERMINAL=$actualTerminal OUTCOME=$actualOutcome PASS"
        )
    }

    println("TERRITORY_CANONICAL_PARITY=PASS")
    println("ORACLE_CASES_VERIFIED=${lines.size}")
}
