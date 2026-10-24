package boardlab.android

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.layout.size
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.TextButton
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.listSaver
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.unit.dp
import boardlab.tre_sigilli.GameController
import boardlab.tre_sigilli.GameMode
import boardlab.tre_sigilli.GameScreenState
import boardlab.tre_sigilli.GameSnapshot
import boardlab.tre_sigilli.State
import boardlab.tre_sigilli.GameOutcome
import boardlab.tre_sigilli.Place
import boardlab.tre_sigilli.Slide
import boardlab.tre_sigilli.cellInfluence
import boardlab.tre_sigilli.gameOutcome
import boardlab.tre_sigilli.territoryScore

private val Ivory = Color(0xFFF5F0E6)
private val Graphite = Color(0xFF33434B)
private val Terracotta = Color(0xFFA65E49)
private val Sand = Color(0xFFD7C7AD)
private val LegalMove = Color(0xFFE6DAC8)
private val Selection = Color(0xFFF0D6A5)

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme {
                Surface(color = Ivory) {
                    TreSigilliScreen()
                }
            }
        }
    }
}

private val snapshotSaver = listSaver<GameSnapshot, Int>(
    save = { snapshot ->
        listOf(
            snapshot.mode.ordinal,
            snapshot.game.turn,
            snapshot.selectedCell ?: -1,
        ) + snapshot.game.cells.map { cell ->
            when (cell) {
                null -> 0
                "north" -> 1
                "south" -> 2
                else -> error("Unknown player")
            }
        }
    },
    restore = { values ->
        require(values.size == 19) { "Invalid snapshot length" }
        val mode = GameMode.entries.getOrNull(values[0])
            ?: throw IllegalArgumentException("Invalid game mode")
        val cells = values.drop(3).map { value ->
            when (value) {
                0 -> null
                1 -> "north"
                2 -> "south"
                else -> throw IllegalArgumentException("Invalid cell")
            }
        }
        GameSnapshot(
            game = State(cells, values[1]),
            mode = mode,
            selectedCell = values[2].takeUnless { it == -1 },
        ).also { GameController().restore(it) }
    },
)

@Composable
fun TreSigilliScreen() {
    var snapshot by rememberSaveable(stateSaver = snapshotSaver) {
        mutableStateOf(GameController().snapshot())
    }
    val controller = remember {
        GameController().apply { restore(snapshot) }
    }
    var showingRules by rememberSaveable { mutableStateOf(false) }
    var optionsExpanded by rememberSaveable { mutableStateOf(false) }
    var exploring by rememberSaveable { mutableStateOf(false) }
    var inspectedCell by rememberSaveable { mutableStateOf<Int?>(null) }

    val screen = GameScreenState(snapshot.game, snapshot.selectedCell)
    val mode = snapshot.mode
    val score = territoryScore(screen.game)
    val outcome = gameOutcome(screen.game)

    fun resetGame(newMode: GameMode? = null) {
        if (newMode == null) {
            controller.newGame()
        } else {
            controller.newGame(newMode)
        }
        snapshot = controller.snapshot()
        showingRules = false
        optionsExpanded = false
        exploring = false
        inspectedCell = null
    }

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .verticalScroll(rememberScrollState())
            .padding(horizontal = 10.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.spacedBy(7.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(
                "Tre Sigilli",
                style = MaterialTheme.typography.titleLarge,
                color = Graphite,
            )

            Row(verticalAlignment = Alignment.CenterVertically) {
                TextButton(onClick = { showingRules = !showingRules }) {
                    Text(
                        if (showingRules) "Chiudi guida" else "Come si gioca",
                        style = MaterialTheme.typography.labelSmall,
                    )
                }

                androidx.compose.foundation.layout.Box {
                    TextButton(onClick = { optionsExpanded = true }) {
                        Text("Opzioni", style = MaterialTheme.typography.labelSmall)
                    }
                    DropdownMenu(
                        expanded = optionsExpanded,
                        onDismissRequest = { optionsExpanded = false },
                    ) {
                        DropdownMenuItem(
                            text = { Text("Contro Android") },
                            onClick = { resetGame(GameMode.HUMAN_VS_ANDROID) },
                        )
                        DropdownMenuItem(
                            text = { Text("Due giocatori") },
                            onClick = { resetGame(GameMode.LOCAL_TWO_PLAYERS) },
                        )
                        DropdownMenuItem(
                            text = { Text("Nuova partita") },
                            onClick = { resetGame() },
                        )
                    }
                }
            }
        }

        Surface(
            modifier = Modifier.fillMaxWidth(),
            color = Sand.copy(alpha = 0.32f),
            shape = androidx.compose.foundation.shape.RoundedCornerShape(12.dp),
        ) {
            Column(
                modifier = Modifier.padding(10.dp),
                verticalArrangement = Arrangement.spacedBy(3.dp),
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    Text(
                        "✦ Nord ${score.north}",
                        color = Graphite,
                        style = MaterialTheme.typography.titleSmall,
                    )
                    Text(
                        "⬡ Sud ${score.south}",
                        color = Terracotta,
                        style = MaterialTheme.typography.titleSmall,
                    )
                }

                Text(
                    "Territorio · Nord: ${score.north} · Sud: ${score.south}",
                    style = MaterialTheme.typography.labelSmall,
                    color = Graphite,
                )

                if (outcome == null) {
                    val player = if (screen.game.turn % 2 == 0) "Nord" else "Sud"
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                    ) {
                        Text(
                            "Turno ${screen.game.turn + 1} · $player",
                            style = MaterialTheme.typography.labelSmall,
                        )
                        Text(
                            if (screen.game.turn < 6) {
                                "Fase: piazzamento"
                            } else {
                                "Fase: manovra"
                            },
                            style = MaterialTheme.typography.labelSmall,
                        )
                    }
                }
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
            Text(
                when {
                    exploring ->
                        "Esplora il territorio: tocca una casella senza muovere sigilli"
                    screen.game.turn < 6 ->
                        "Piazza un sigillo in una casella verde"
                    screen.selectedCell == null ->
                        "Seleziona un tuo sigillo evidenziato"
                    else ->
                        "Scegli una destinazione verde"
                },
                style = MaterialTheme.typography.bodySmall,
            )
        }

        Board(
            screen = screen,
            exploring = exploring,
            inspectedCell = inspectedCell,
            onTap = { cell ->
                if (exploring) {
                    inspectedCell = cell
                } else {
                    controller.tap(cell)
                    snapshot = controller.snapshot()
                }
            },
        )

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(
                if (mode == GameMode.HUMAN_VS_ANDROID) {
                    "Modalità: Contro Android"
                } else {
                    "Modalità: Due giocatori"
                },
                style = MaterialTheme.typography.labelSmall,
            )
            TextButton(
                onClick = {
                    exploring = !exploring
                    inspectedCell = null
                },
            ) {
                Text(
                    if (exploring) "Torna a giocare" else "Mostra territori",
                    style = MaterialTheme.typography.labelSmall,
                )
            }
        }

        if (outcome != null) {
            TextButton(
                onClick = {
                    exploring = true
                    inspectedCell = null
                },
            ) {
                Text("Spiegami il risultato")
            }
            if (exploring) {
                Text(
                    "Stella: ${score.north} caselle · " +
                        "Esagono: ${score.south} caselle. " +
                        "Tocca le caselle per scoprire come sono assegnati i punti."
                )
            }
        }

        if (exploring) {
            val inspected = inspectedCell
            if (inspected != null) {
                val influence = cellInfluence(screen.game, inspected)
                val ownership = when {
                    influence.north > influence.south ->
                        "Controllata dalla Stella: +1 punto"
                    influence.south > influence.north ->
                        "Controllata dall'Esagono: +1 punto"
                    influence.north == 0 ->
                        "Casella neutra: 0 punti"
                    else ->
                        "Casella contesa: 0 punti"
                }
                Text("Casella ${inspected / 4 + 1}, ${inspected % 4 + 1}")
                Text(
                    "Influenza Stella: ${influence.north} · " +
                        "Esagono: ${influence.south}"
                )
                Text(ownership)
            }
        }

        if (showingRules) {
            Column(
                verticalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Text(
                    "Obiettivo",
                    style = MaterialTheme.typography.titleMedium,
                )
                Text(
                    "Controlla più caselle del tuo avversario. " +
                        "Non bisogna allineare tre sigilli."
                )

                Text(
                    "1. Piazzamento",
                    style = MaterialTheme.typography.titleMedium,
                )
                Text(
                    "A turno, ciascun giocatore piazza tre sigilli " +
                        "nelle caselle libere."
                )

                Text(
                    "2. Manovra",
                    style = MaterialTheme.typography.titleMedium,
                )
                Text(
                    "Sposta un tuo sigillo di una casella libera " +
                        "in orizzontale o verticale. " +
                        "Le diagonali non sono consentite."
                )

                Text(
                    "3. Punteggio",
                    style = MaterialTheme.typography.titleMedium,
                )
                Text(
                    "Ogni sigillo influenza la propria casella e " +
                        "le quattro caselle adiacenti. " +
                        "Ogni casella vale un punto per chi ha " +
                        "più sigilli influenti. " +
                        "In caso di parità o assenza di influenza " +
                        "vale zero punti."
                )

                Text(
                    "4. Vittoria",
                    style = MaterialTheme.typography.titleMedium,
                )
                Text(
                    "La partita termina dopo 18 turni oppure " +
                        "quando il giocatore di turno non può " +
                        "effettuare una manovra. " +
                        "Vince chi controlla più caselle; " +
                        "un punteggio uguale è un pareggio."
                )

                Text(
                    "Suggerimento: usa «Mostra territori» " +
                        "per esplorare i punti senza eseguire mosse."
                )
            }
        }

    }
}

@Composable
private fun Board(
    screen: GameScreenState,
    exploring: Boolean,
    inspectedCell: Int?,
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

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .background(Graphite, RoundedCornerShape(12.dp))
            .padding(4.dp),
        verticalArrangement = Arrangement.spacedBy(3.dp),
    ) {
        repeat(4) { row ->
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(3.dp),
            ) {
                repeat(4) { column ->
                    val cell = row * 4 + column
                    val occupant = screen.game.cells[cell]
                    val selected = screen.selectedCell == cell
                    val background = when {
                        exploring && inspectedCell == cell -> Selection
                        exploring -> {
                            val influence = cellInfluence(screen.game, cell)
                            when {
                                influence.north > influence.south ->
                                    Color(0xFFCBD5D6)
                                influence.south > influence.north ->
                                    Color(0xFFE5C4B9)
                                else -> Sand
                            }
                        }
                        selected -> Selection
                        cell in legalTargets -> LegalMove
                        occupant != null -> Ivory
                        (row + column) % 2 == 0 -> Color(0xFFF1E9DC)
                        else -> Color(0xFFEAE0D1)
                    }

                    Column(
                        modifier = Modifier
                            .weight(1f)
                            .aspectRatio(1f)
                            .testTag("board_cell_$cell")
                            .semantics {
                                stateDescription = when {
                                    exploring -> "Esplorazione territorio"
                                    selected -> "Sigillo selezionato"
                                    cell in legalTargets -> "Mossa disponibile"
                                    else -> "Mossa non disponibile"
                                }
                            }
                            .background(
                                background,
                                RoundedCornerShape(4.dp),
                            )
                            .clickable { onTap(cell) },
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.Center,
                    ) {
                        if (occupant != null) {
                            Seal(occupant)
                        } else {
                            Text(
                                "·",
                                color = if (!exploring && cell in legalTargets) {
                                    Terracotta
                                } else {
                                    Graphite.copy(alpha = 0.45f)
                                },
                            )
                        }
                    }
                }
            }
        }
    }
}


@Composable
private fun Seal(player: String) {
    require(player == "north" || player == "south")

    val north = player == "north"
    val color = if (north) Graphite else Terracotta
    val label = if (north) "Sigillo Stella" else "Sigillo Esagono"

    Canvas(
        modifier = Modifier
            .size(36.dp)
            .semantics { contentDescription = label },
    ) {
        val unit = size.minDimension / 100f
        val cx = size.width / 2f
        val cy = size.height / 2f

        drawCircle(
            color = color,
            radius = 43f * unit,
            center = Offset(cx, cy),
        )

        fun point(x: Float, y: Float): Offset =
            Offset(cx + (x - 50f) * unit, cy + (y - 50f) * unit)

        if (north) {
            val vertices = listOf(
                50f to 14f,
                59f to 41f,
                86f to 50f,
                59f to 59f,
                50f to 86f,
                41f to 59f,
                14f to 50f,
                41f to 41f,
            )

            val star = Path().apply {
                vertices.forEachIndexed { index, vertex ->
                    val p = point(vertex.first, vertex.second)
                    if (index == 0) moveTo(p.x, p.y)
                    else lineTo(p.x, p.y)
                }
                close()
            }

            drawPath(star, color = Ivory)
        } else {
            val vertices = listOf(
                50f to 19f,
                77f to 34f,
                77f to 66f,
                50f to 81f,
                23f to 66f,
                23f to 34f,
            )

            val hexagon = Path().apply {
                vertices.forEachIndexed { index, vertex ->
                    val p = point(vertex.first, vertex.second)
                    if (index == 0) moveTo(p.x, p.y)
                    else lineTo(p.x, p.y)
                }
                close()
            }

            drawPath(
                hexagon,
                color = Ivory,
                style = androidx.compose.ui.graphics.drawscope.Stroke(
                    width = 5f * unit,
                ),
            )

            drawCircle(
                color = Ivory,
                radius = 6f * unit,
                center = Offset(cx, cy),
            )
        }
    }
}
