package boardlab.tre_sigilli

fun main() {
    // The existing zero-argument controller keeps local two-player behavior.
    val local = GameController()
    check(local.mode == GameMode.LOCAL_TWO_PLAYERS)
    local.tap(0)
    check(local.screen.game.turn == 1)
    check(local.screen.game.cells[0] == "north")
    local.tap(1)
    check(local.screen.game.turn == 2)
    check(local.screen.game.cells[1] == "south")

    // Single-player: each legal human placement receives one AI reply.
    val single = GameController(GameMode.HUMAN_VS_ANDROID)
    check(single.screen.game.turn == 0)
    single.tap(0)
    check(single.screen.game.turn == 2)
    check(single.screen.game.cells.count { it == "north" } == 1)
    check(single.screen.game.cells.count { it == "south" } == 1)
    check(single.screen.selectedCell == null)

    // Invalid placement must not cause an AI move.
    val previous = single.screen
    single.tap(0)
    check(single.screen == previous)

    // A fresh controller makes the same deterministic AI response.
    val repeat = GameController(GameMode.HUMAN_VS_ANDROID)
    repeat.tap(0)
    check(repeat.screen == single.screen)

    // Complete a single-player game using legal NORTH moves.
    var interactions = 0
    while (single.screen.legalMoves.isNotEmpty()) {
        check(interactions < 40) { "Unexpected game progression" }
        val current = single.screen
        check(current.game.turn % 2 == 0) {
            "Android turn exposed to human input"
        }

        val move = current.legalMoves.first()
        when (move) {
            is Place -> single.tap(move.cell)
            is Slide -> {
                single.tap(move.origin)
                check(single.screen.selectedCell == move.origin)
                single.tap(move.destination)
            }
        }

        check(single.screen.game.turn > current.game.turn)
        check(single.screen.game.turn <= 18)
        interactions++
    }

    check(gameOutcome(single.screen.game) != null)
    val terminal = single.screen
    single.tap(0)
    check(single.screen == terminal)

    // Reset and mode changes are explicit; default reset preserves mode.
    single.newGame()
    check(single.mode == GameMode.HUMAN_VS_ANDROID)
    check(single.screen.game.turn == 0)

    single.newGame(GameMode.LOCAL_TWO_PLAYERS)
    check(single.mode == GameMode.LOCAL_TWO_PLAYERS)
    single.tap(0)
    check(single.screen.game.turn == 1)

    println("LOCAL_TWO_PLAYER_REGRESSION=PASS")
    println("SINGLE_PLAYER_AUTOMATIC_REPLY=PASS")
    println("SINGLE_PLAYER_COMPLETE_GAME=PASS")
    println("MODE_SWITCH_AND_RESET=PASS")
}
