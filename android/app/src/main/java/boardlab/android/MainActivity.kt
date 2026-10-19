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
import boardlab.tre_sigilli.GameMode
import boardlab.tre_sigilli.GameScreenState
import boardlab.tre_sigilli.GameOutcome
import boardlab.tre_sigilli.Place
import boardlab.tre_sigilli.Slide
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

        Text(
            if (controller.mode == GameMode.HUMAN_VS_ANDROID) {
                "Modalità: Contro Android"
            } else {
                "Modalità: Due giocatori"
            },
        )

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            Button(
                onClick = {
                    controller.newGame(GameMode.HUMAN_VS_ANDROID)
                    screen = controller.screen
                },
            ) {
                Text("Contro Android")
            }
            Button(
                onClick = {
                    controller.newGame(GameMode.LOCAL_TWO_PLAYERS)
                    screen = controller.screen
                },
            ) {
                Text("Due giocatori")
            }
        }

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

        if (outcome == null) {
            Text(
                when {
                    screen.game.turn < 6 ->
                        "Piazza un sigillo in una casella verde"
                    screen.selectedCell == null ->
                        "Seleziona un tuo sigillo evidenziato"
                    else ->
                        "Scegli una destinazione verde"
                },
            )
        }

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
    val legalTargets = when {
        screen.game.turn < 6 ->
            screen.legalMoves.filterIsInstance<Place>()
                .map { it.cell }.toSet()
        screen.selectedCell != null ->
            screen.legalMoves.filterIsInstance<Slide>()
                .filter { it.origin == screen.selectedCell }
                .map { it.destination }.toSet()
        else ->
            screen.legalMoves.filterIsInstance<Slide>()
                .map { it.origin }.toSet()
    }

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
                        cell in legalTargets -> Color(0xFFA5D6A7)
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
