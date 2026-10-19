package boardlab.android

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.graphics.asAndroidBitmap
import androidx.compose.ui.semantics.SemanticsProperties
import androidx.compose.ui.test.captureToImage
import androidx.compose.ui.test.SemanticsMatcher
import androidx.compose.ui.test.assertCountEquals
import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertTextEquals
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.test.ext.junit.runners.AndroidJUnit4
import boardlab.tre_sigilli.GameOutcome
import boardlab.tre_sigilli.Move
import boardlab.tre_sigilli.Place
import boardlab.tre_sigilli.Slide
import boardlab.tre_sigilli.State
import boardlab.tre_sigilli.applyMove
import boardlab.tre_sigilli.gameOutcome
import boardlab.tre_sigilli.legalMoves
import boardlab.tre_sigilli.territoryScore
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class TreSigilliUiTest {
    @get:Rule
    val compose = createAndroidComposeRule<MainActivity>()

    @Test
    fun firstMoveAdvancesTurnAndNewGameResetsBoard() {
        compose.onNodeWithText("Tre Sigilli").assertIsDisplayed()
        compose.onAllNodes(
            SemanticsMatcher("board cell tag") { node ->
                node.config.contains(SemanticsProperties.TestTag) &&
                    node.config[SemanticsProperties.TestTag].startsWith("board_cell_")
            },
        ).assertCountEquals(16)
        assertEmptyBoard()
        compose.onNodeWithText("Turno 1 · Nord").assertIsDisplayed()
        compose.onNodeWithText("Fase: piazzamento").assertIsDisplayed()
        compose.onNodeWithText("Territorio · Nord: 0 · Sud: 0")
            .assertIsDisplayed()

        compose.onNodeWithTag("board_cell_0").performClick()

        compose.onNodeWithTag("board_cell_0").assertTextEquals("N")
        for (cell in 1..15) {
            compose.onNodeWithTag("board_cell_$cell").assertTextEquals("·")
        }
        compose.onNodeWithText("Turno 2 · Sud").assertIsDisplayed()
        compose.onNodeWithText("Territorio · Nord: 3 · Sud: 0")
            .assertIsDisplayed()

        compose.onNodeWithText("Nuova partita").performClick()

        compose.onNodeWithText("Turno 1 · Nord").assertIsDisplayed()
        compose.onNodeWithText("Fase: piazzamento").assertIsDisplayed()
        compose.onNodeWithText("Territorio · Nord: 0 · Sud: 0")
            .assertIsDisplayed()
        assertEmptyBoard()
    }

    @Test
    fun sixPlacementsTransitionToManeuverPhase() {
        for (cell in 0..5) {
            compose.onNodeWithTag("board_cell_$cell").performClick()
        }

        compose.onNodeWithText("Turno 7 · Nord").assertIsDisplayed()
        compose.onNodeWithText("Fase: manovra").assertIsDisplayed()
    }

    @Test
    fun completeGameShowsOutcomeAndCanReset() {
        var state = State(List(16) { null }, 0)
        var movesPlayed = 0

        while (legalMoves(state).isNotEmpty()) {
            check(movesPlayed < 18) { "Game exceeded turn limit" }

            val move: Move = legalMoves(state).first()

            when (move) {
                is Place -> {
                    compose.onNodeWithTag("board_cell_${move.cell}")
                        .performClick()
                }
                is Slide -> {
                    compose.onNodeWithTag("board_cell_${move.origin}")
                        .performClick()
                    compose.onNodeWithTag("board_cell_${move.destination}")
                        .performClick()
                }
            }

            state = applyMove(state, move)
            movesPlayed++

            val score = territoryScore(state)
            compose.onNodeWithText(
                "Territorio · Nord: ${score.north} · Sud: ${score.south}"
            ).assertIsDisplayed()
        }

        val outcome = checkNotNull(gameOutcome(state))
        val expectedLabel = when (outcome) {
            GameOutcome.NORTH_WINS -> "Vince Nord"
            GameOutcome.SOUTH_WINS -> "Vince Sud"
            GameOutcome.DRAW -> "Pareggio"
        }

        compose.onNodeWithText("Partita terminata").assertIsDisplayed()
        compose.onNodeWithText(expectedLabel).assertIsDisplayed()

        compose.onNodeWithText("Nuova partita").performClick()

        compose.onNodeWithText("Turno 1 · Nord").assertIsDisplayed()
        compose.onNodeWithText("Territorio · Nord: 0 · Sud: 0")
            .assertIsDisplayed()
        assertEmptyBoard()
    }

    @Test
    fun contextualInstructionsAndLegalHighlights() {
        compose.onNodeWithText(
            "Piazza un sigillo in una casella verde"
        ).assertIsDisplayed()

        assertCellColor(0, Color(0xFFA5D6A7))

        for (cell in 0..5) {
            compose.onNodeWithTag("board_cell_$cell").performClick()
        }

        compose.onNodeWithText(
            "Seleziona un tuo sigillo evidenziato"
        ).assertIsDisplayed()

        // NORTH owns 0, 2, 4. After six placements,
        // origin 4 can slide to cell 8.
        assertCellColor(4, Color(0xFFA5D6A7))

        compose.onNodeWithTag("board_cell_4").performClick()

        compose.onNodeWithText(
            "Scegli una destinazione verde"
        ).assertIsDisplayed()

        assertCellColor(4, Color(0xFFFFD54F))
        assertCellColor(8, Color(0xFFA5D6A7))

        compose.onNodeWithTag("board_cell_8").performClick()

        compose.onNodeWithText("Turno 8 · Sud").assertIsDisplayed()
        compose.onNodeWithText(
            "Seleziona un tuo sigillo evidenziato"
        ).assertIsDisplayed()
    }

    private fun assertCellColor(cell: Int, expected: Color) {
        val image = compose.onNodeWithTag("board_cell_$cell")
            .captureToImage()
        val bitmap = image.asAndroidBitmap()
        // Sample inside the background, away from the centered glyph.
        val actual = bitmap.getPixel(
            bitmap.width / 8,
            bitmap.height / 8,
        )
        check(actual == expected.toArgb()) {
            "Cell $cell color: expected=${expected.toArgb()} actual=$actual"
        }
    }

    private fun assertEmptyBoard() {
        for (cell in 0..15) {
            compose.onNodeWithTag("board_cell_$cell")
                .assertIsDisplayed()
                .assertTextEquals("·")
        }
    }
}
