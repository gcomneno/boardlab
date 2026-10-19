package boardlab.android

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.unit.dp
import boardlab.tre_sigilli.GameController
import boardlab.tre_sigilli.GameScreenState
import boardlab.tre_sigilli.GameOutcome
import boardlab.tre_sigilli.gameOutcome
import boardlab.tre_sigilli.territoryScore

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme {
                Surface {
                    TreSigilliScreen()
                }
            }
        }
    }
}

@Composable
fun TreSigilliScreen() {
    val controller = remember { GameController() }
    var screen by remember { mutableStateOf(controller.screen) }
    val score = territoryScore(screen.game)
    val outcome = gameOutcome(screen.game)

    Column(
        modifier = Modifier.padding(20.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Text("Tre Sigilli", style = MaterialTheme.typography.headlineMedium)

        if (outcome != null) {
            Text("Partita terminata")
            Text(
                when (outcome) {
                    GameOutcome.NORTH_WINS -> "Vince Nord"
                    GameOutcome.SOUTH_WINS -> "Vince Sud"
                    GameOutcome.DRAW -> "Pareggio"
                },
            )
        } else {
            val player = if (screen.game.turn % 2 == 0) "Nord" else "Sud"
            Text("Turno ${screen.game.turn + 1} · $player")
            Text(
                if (screen.game.turn < 6) {
                    "Fase: piazzamento"
                } else {
                    "Fase: manovra"
                },
            )
        }

        Text("Territorio · Nord: ${score.north} · Sud: ${score.south}")

        Board(
            screen = screen,
            onTap = { cell ->
                controller.tap(cell)
                screen = controller.screen
            },
        )

        Button(
            onClick = {
                controller.newGame()
                screen = controller.screen
            },
        ) {
            Text("Nuova partita")
        }
    }
}

@Composable
private fun Board(
    screen: GameScreenState,
    onTap: (Int) -> Unit,
) {
    Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
        repeat(4) { row ->
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(4.dp),
            ) {
                repeat(4) { column ->
                    val cell = row * 4 + column
                    val occupant = screen.game.cells[cell]
                    val selected = screen.selectedCell == cell
                    val background = when {
                        selected -> Color(0xFFFFD54F)
                        occupant == "north" -> Color(0xFF90CAF9)
                        occupant == "south" -> Color(0xFFEF9A9A)
                        else -> Color(0xFFE0E0E0)
                    }

                    Column(
                        modifier = Modifier
                            .weight(1f)
                            .aspectRatio(1f)
                            .testTag("board_cell_$cell")
                            .background(background)
                            .clickable { onTap(cell) },
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.Center,
                    ) {
                        Text(
                            when (occupant) {
                                "north" -> "N"
                                "south" -> "S"
                                else -> "·"
                            },
                        )
                    }
                }
            }
        }
    }
}
