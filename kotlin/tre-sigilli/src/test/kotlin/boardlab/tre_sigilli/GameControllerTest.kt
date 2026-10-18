package boardlab.tre_sigilli

fun main() {
    val controller = GameController()

    check(controller.screen.game.turn == 0)
    check(controller.screen.legalMoves.size == 16)

    controller.tap(0)
    check(controller.screen.game.turn == 1)
    check(controller.screen.game.cells[0] == "north")

    controller.tap(0)
    check(controller.screen.game.turn == 1)

    controller.tap(1)
    controller.tap(2)
    controller.tap(3)
    controller.tap(4)
    controller.tap(5)

    check(controller.screen.game.turn == 6)
    check(controller.screen.selectedCell == null)

    // At turn six, the current player selects a seal and then a destination.
    val beforeManeuver = controller.screen.game
    val slides = controller.screen.legalMoves.filterIsInstance<Slide>()
    check(slides.isNotEmpty())

    val slide = slides.first()
    controller.tap(slide.origin)
    check(controller.screen.selectedCell == slide.origin)
    check(controller.screen.game == beforeManeuver)

    controller.tap(slide.destination)
    check(controller.screen.game.turn == 7)
    check(controller.screen.selectedCell == null)
    check(controller.screen.game.cells[slide.origin] == null)
    check(controller.screen.game.cells[slide.destination] == "north")

    // An invalid selection must not change the game.
    val beforeInvalid = controller.screen.game
    val invalidOrigin = (0..15).first {
        candidate ->
        controller.screen.legalMoves.filterIsInstance<Slide>()
            .none { it.origin == candidate }
    }
    controller.tap(invalidOrigin)
    check(controller.screen.game == beforeInvalid)

    controller.newGame()
    check(controller.screen.game.turn == 0)
    check(controller.screen.game.cells.all { it == null })

    println("GAME_CONTROLLER=PASS")
    println("ANDROID_INDEPENDENT_CONTROLLER=PASS")
}
