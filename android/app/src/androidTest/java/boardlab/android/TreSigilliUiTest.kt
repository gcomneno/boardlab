package boardlab.android

import androidx.compose.ui.semantics.SemanticsProperties
import androidx.compose.ui.test.SemanticsMatcher
import androidx.compose.ui.test.assertCountEquals
import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertTextEquals
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.test.ext.junit.runners.AndroidJUnit4
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

        compose.onNodeWithTag("board_cell_0").performClick()

        compose.onNodeWithTag("board_cell_0").assertTextEquals("N")
        for (cell in 1..15) {
            compose.onNodeWithTag("board_cell_$cell").assertTextEquals("·")
        }
        compose.onNodeWithText("Turno 2 · Sud").assertIsDisplayed()

        compose.onNodeWithText("Nuova partita").performClick()

        compose.onNodeWithText("Turno 1 · Nord").assertIsDisplayed()
        assertEmptyBoard()
    }

    private fun assertEmptyBoard() {
        for (cell in 0..15) {
            compose.onNodeWithTag("board_cell_$cell")
                .assertIsDisplayed()
                .assertTextEquals("·")
        }
    }
}
