package boardlab.tre_sigilli

fun main() {
    val local = GameController()
    local.tap(0)
    local.tap(1)

    val localSnapshot = local.snapshot()
    val restoredLocal = GameController()
    restoredLocal.restore(localSnapshot)

    check(restoredLocal.mode == GameMode.LOCAL_TWO_PLAYERS)
    check(restoredLocal.screen == local.screen)
    check(restoredLocal.snapshot() == localSnapshot)

    val single = GameController(GameMode.HUMAN_VS_ANDROID)
    single.tap(0)

    val singleSnapshot = single.snapshot()
    val restoredSingle = GameController()
    restoredSingle.restore(singleSnapshot)

    check(restoredSingle.mode == GameMode.HUMAN_VS_ANDROID)
    check(restoredSingle.screen == single.screen)
    check(restoredSingle.screen.game.turn == 2)

    // An invalid snapshot must not partially mutate the controller.
    val previous = restoredSingle.snapshot()

    val invalid = singleSnapshot.copy(
        game = State(List(16) { null }, 9),
    )

    check(
        runCatching { restoredSingle.restore(invalid) }
            .exceptionOrNull() is IllegalArgumentException
    )
    check(restoredSingle.snapshot() == previous)

    // Selection must represent a currently legal maneuver origin.
    var maneuver = GameController()
    for (cell in 0..5) maneuver.tap(cell)

    val origin = maneuver.screen.legalMoves
        .filterIsInstance<Slide>()
        .first().origin

    maneuver.tap(origin)
    check(maneuver.screen.selectedCell == origin)

    val selectedSnapshot = maneuver.snapshot()
    val selectedRestored = GameController()
    selectedRestored.restore(selectedSnapshot)

    check(selectedRestored.screen == maneuver.screen)

    val illegalSelection = selectedSnapshot.copy(selectedCell = 15)
    val beforeInvalidSelection = selectedRestored.snapshot()

    check(
        runCatching { selectedRestored.restore(illegalSelection) }
            .exceptionOrNull() is IllegalArgumentException
    )
    check(selectedRestored.snapshot() == beforeInvalidSelection)

    // Reject a pending Android turn: it would expose an AI move
    // to the human interface without completing the reply.
    val northOnly = State(
        cells = listOf("north") + List(15) { null },
        turn = 1,
    )

    check(
        runCatching {
            restoredSingle.restore(
                GameSnapshot(
                    northOnly,
                    GameMode.HUMAN_VS_ANDROID,
                    null,
                )
            )
        }.exceptionOrNull() is IllegalArgumentException
    )
    check(restoredSingle.snapshot() == previous)

    println("LOCAL_RESTORATION=PASS")
    println("SINGLE_PLAYER_RESTORATION=PASS")
    println("SELECTED_CELL_RESTORATION=PASS")
    println("INVALID_SNAPSHOT_FAIL_CLOSED=PASS")
}
