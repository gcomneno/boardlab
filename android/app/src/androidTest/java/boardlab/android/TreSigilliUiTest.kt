package boardlab.android

import androidx.compose.ui.semantics.SemanticsProperties
import androidx.compose.ui.test.SemanticsMatcher
import androidx.compose.ui.test.assertCountEquals
import androidx.compose.ui.test.onNodeWithContentDescription
import androidx.compose.ui.test.assert
import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertTextEquals
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performScrollTo
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
        compose.onNodeWithText("Tre Sigilli").performScrollTo().assertIsDisplayed()
        compose.onAllNodes(
            SemanticsMatcher("board cell tag") { node ->
                node.config.contains(SemanticsProperties.TestTag) &&
                    node.config[SemanticsProperties.TestTag].startsWith("board_cell_")
            },
        ).assertCountEquals(16)
        assertEmptyBoard()
        compose.onNodeWithText("Turno 1 · Nord").performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Fase: piazzamento").performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Territorio · Nord: 0 · Sud: 0")
            .performScrollTo().assertIsDisplayed()

        compose.onNodeWithTag("board_cell_0").performScrollTo().performClick()

        compose.onNodeWithContentDescription("Sigillo Stella").assertExists()
        for (cell in 1..15) {
            compose.onNodeWithTag("board_cell_$cell").assertTextEquals("·")
        }
        compose.onNodeWithText("Turno 2 · Sud").performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Territorio · Nord: 3 · Sud: 0")
            .performScrollTo().assertIsDisplayed()

        compose.onNodeWithText("Opzioni").performScrollTo().performClick()
        compose.onNodeWithText("Nuova partita").performClick()

        compose.onNodeWithText("Turno 1 · Nord").performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Fase: piazzamento").performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Territorio · Nord: 0 · Sud: 0")
            .performScrollTo().assertIsDisplayed()
        assertEmptyBoard()
    }

    @Test
    fun sixPlacementsTransitionToManeuverPhase() {
        for (cell in 0..5) {
            compose.onNodeWithTag("board_cell_$cell").performScrollTo().performClick()
        }

        compose.onNodeWithText("Turno 7 · Nord").performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Fase: manovra").performScrollTo().assertIsDisplayed()
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
                        .performScrollTo().performClick()
                }
                is Slide -> {
                    compose.onNodeWithTag("board_cell_${move.origin}")
                        .performScrollTo().performClick()
                    compose.onNodeWithTag("board_cell_${move.destination}")
                        .performScrollTo().performClick()
                }
            }

            state = applyMove(state, move)
            movesPlayed++

            val score = territoryScore(state)
            compose.onNodeWithText(
                "Territorio · Nord: ${score.north} · Sud: ${score.south}"
            ).performScrollTo().assertIsDisplayed()
        }

        val outcome = checkNotNull(gameOutcome(state))
        val expectedLabel = when (outcome) {
            GameOutcome.NORTH_WINS -> "Vince Nord"
            GameOutcome.SOUTH_WINS -> "Vince Sud"
            GameOutcome.DRAW -> "Pareggio"
        }

        compose.onNodeWithText("Partita terminata").performScrollTo().assertIsDisplayed()
        compose.onNodeWithText(expectedLabel).performScrollTo().assertIsDisplayed()

        compose.onNodeWithText("Spiegami il risultato")
            .performScrollTo().performClick()

        compose.onNodeWithText("Torna a giocare")
            .performScrollTo().assertIsDisplayed()

        compose.onNodeWithTag("board_cell_5")
            .performScrollTo().performClick()

        compose.onNodeWithText(
            "Casella 2, 2"
        ).performScrollTo().assertIsDisplayed()

        compose.onNodeWithText("Opzioni").performScrollTo().performClick()
        compose.onNodeWithText("Nuova partita").performClick()

        compose.onNodeWithText("Turno 1 · Nord").performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Territorio · Nord: 0 · Sud: 0")
            .performScrollTo().assertIsDisplayed()
        assertEmptyBoard()
    }

    @Test
    fun contextualInstructionsAndLegalHighlights() {
        compose.onNodeWithText(
            "Piazza un sigillo in una casella verde"
        ).performScrollTo().assertIsDisplayed()

        assertCellState(0, "Mossa disponibile")

        for (cell in 0..5) {
            compose.onNodeWithTag("board_cell_$cell").performScrollTo().performClick()
        }

        compose.onNodeWithText(
            "Seleziona un tuo sigillo evidenziato"
        ).performScrollTo().assertIsDisplayed()

        // NORTH owns 0, 2, 4. After six placements,
        // origin 4 can slide to cell 8.
        assertCellState(4, "Mossa disponibile")

        compose.onNodeWithTag("board_cell_4").performScrollTo().performClick()

        compose.onNodeWithText(
            "Scegli una destinazione verde"
        ).performScrollTo().assertIsDisplayed()

        assertCellState(4, "Sigillo selezionato")
        assertCellState(8, "Mossa disponibile")

        compose.onNodeWithTag("board_cell_8").performScrollTo().performClick()

        compose.onNodeWithText("Turno 8 · Sud").performScrollTo().assertIsDisplayed()
        compose.onNodeWithText(
            "Seleziona un tuo sigillo evidenziato"
        ).performScrollTo().assertIsDisplayed()
    }

    private fun assertCellState(cell: Int, expected: String) {
        compose.onNodeWithTag("board_cell_$cell")
            .performScrollTo()
            .assert(
                SemanticsMatcher.expectValue(
                    SemanticsProperties.StateDescription,
                    expected,
                )
            )
    }

    @Test
    fun modeSelectorPreservesLocalPlayAndEnablesOfflineAndroid() {
        compose.onNodeWithText("Modalità: Due giocatori")
            .performScrollTo().assertIsDisplayed()

        compose.onNodeWithText("Opzioni").performScrollTo().performClick()
        compose.onNodeWithText("Contro Android").performClick()

        compose.onNodeWithText("Modalità: Contro Android")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Turno 1 · Nord").performScrollTo().assertIsDisplayed()
        assertEmptyBoard()

        compose.onNodeWithTag("board_cell_0").performScrollTo().performClick()

        // Human NORTH and Android SOUTH have both completed one move.
        compose.onNodeWithText("Turno 3 · Nord").performScrollTo().assertIsDisplayed()

        compose.onNodeWithContentDescription("Sigillo Stella").assertExists()

        // Mode changes reset the game and retain the original local flow.
        compose.onNodeWithText("Opzioni").performScrollTo().performClick()
        compose.onNodeWithText("Due giocatori").performClick()

        compose.onNodeWithText("Modalità: Due giocatori")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Turno 1 · Nord").performScrollTo().assertIsDisplayed()
        assertEmptyBoard()

        compose.onNodeWithTag("board_cell_0").performScrollTo().performClick()

        // Local mode advances by exactly one turn.
        compose.onNodeWithText("Turno 2 · Sud").performScrollTo().assertIsDisplayed()

        compose.onNodeWithText("Opzioni").performScrollTo().performClick()
        compose.onNodeWithText("Nuova partita").performClick()
        compose.onNodeWithText("Modalità: Due giocatori")
            .performScrollTo().assertIsDisplayed()
        assertEmptyBoard()
    }

    @Test
    fun localGameSurvivesActivityRecreation() {
        compose.onNodeWithTag("board_cell_0").performScrollTo().performClick()
        compose.onNodeWithTag("board_cell_1").performScrollTo().performClick()

        compose.activityRule.scenario.recreate()

        compose.onNodeWithText("Modalità: Due giocatori")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Turno 3 · Nord")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithContentDescription("Sigillo Stella").assertExists()
        compose.onNodeWithContentDescription("Sigillo Esagono").assertExists()

        // Restored controller must remain interactive.
        compose.onNodeWithTag("board_cell_2").performScrollTo().performClick()
        compose.onNodeWithText("Turno 4 · Sud")
            .performScrollTo().assertIsDisplayed()
    }

    @Test
    fun singlePlayerSurvivesActivityRecreation() {
        compose.onNodeWithText("Opzioni").performScrollTo().performClick()
        compose.onNodeWithText("Contro Android").performClick()
        compose.onNodeWithTag("board_cell_0").performScrollTo().performClick()

        compose.onNodeWithText("Turno 3 · Nord")
            .performScrollTo().assertIsDisplayed()

        compose.activityRule.scenario.recreate()

        compose.onNodeWithText("Modalità: Contro Android")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Turno 3 · Nord")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithContentDescription("Sigillo Stella").assertExists()

        // The next human move must still trigger one Android reply.
        compose.onNodeWithTag("board_cell_1").performScrollTo().performClick()
        compose.onNodeWithText("Turno 5 · Nord")
            .performScrollTo().assertIsDisplayed()
    }


    @Test
    fun exploringTerritoryNeverExecutesMoves() {
        compose.onNodeWithTag("board_cell_0")
            .performScrollTo().performClick()

        compose.onNodeWithText("Turno 2 · Sud")
            .performScrollTo().assertIsDisplayed()

        compose.onNodeWithText("Mostra territori")
            .performScrollTo().performClick()

        // Inspect every square without modifying the match.
        for (cell in 0..15) {
            compose.onNodeWithTag("board_cell_$cell")
                .performScrollTo().performClick()
        }

        compose.onNodeWithText("Turno 2 · Sud")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Territorio · Nord: 3 · Sud: 0")
            .performScrollTo().assertIsDisplayed()

        compose.onNodeWithContentDescription("Sigillo Stella")
            .assertExists()

        compose.onNodeWithText("Torna a giocare")
            .performScrollTo().performClick()

        compose.onNodeWithText("Opzioni").performScrollTo().performClick()
        compose.onNodeWithText("Nuova partita").performClick()

        compose.onNodeWithText("Turno 1 · Nord")
            .performScrollTo().assertIsDisplayed()

        compose.onNodeWithText("Mostra territori")
            .performScrollTo().assertIsDisplayed()

        assertEmptyBoard()
    }

    private fun assertEmptyBoard() {
        for (cell in 0..15) {
            compose.onNodeWithTag("board_cell_$cell")
                .performScrollTo().assertIsDisplayed()
                .assertTextEquals("·")
        }
    }
}
