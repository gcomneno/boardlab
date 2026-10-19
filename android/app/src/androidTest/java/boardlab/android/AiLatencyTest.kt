package boardlab.android

import android.app.Instrumentation
import android.os.Bundle
import android.util.Log
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.ext.junit.runners.AndroidJUnit4
import boardlab.tre_sigilli.AlphaBetaStrategy
import boardlab.tre_sigilli.GameController
import boardlab.tre_sigilli.GameMode
import boardlab.tre_sigilli.Place
import boardlab.tre_sigilli.Slide
import boardlab.tre_sigilli.State
import boardlab.tre_sigilli.applyMove
import boardlab.tre_sigilli.legalMoves
import org.junit.Test
import org.junit.runner.RunWith
import java.util.Locale

@RunWith(AndroidJUnit4::class)
class AiLatencyTest {
    private val strategy = AlphaBetaStrategy(depth = 2)

    private fun report(message: String) {
        val tagged = "BOARDLAB_AI_LATENCY $message"

        Log.i("BOARDLAB_AI_LATENCY", message)
        println(tagged)

        // Emit a structured instrumentation status message too.
        // This can be captured by the Android test runner.
        InstrumentationRegistry.getInstrumentation().sendStatus(
            0,
            Bundle().apply {
                putString(Instrumentation.REPORT_KEY_STREAMRESULT, tagged)
            },
        )
    }

    private fun ms(value: Long): String =
        String.format(Locale.ROOT, "%.3f", value / 1_000_000.0)

    private fun percentile(values: List<Long>, percentile: Int): Long {
        val sorted = values.sorted()
        return sorted[(sorted.size - 1) * percentile / 100]
    }

    private fun benchmark(
        label: String,
        repetitions: Int = 100,
        operation: () -> Unit,
    ) {
        // Separate initial execution and warm measurements.
        val coldStart = System.nanoTime()
        operation()
        val cold = System.nanoTime() - coldStart

        repeat(20) { operation() }

        val samples = List(repetitions) {
            val start = System.nanoTime()
            operation()
            System.nanoTime() - start
        }

        report(
            "$label " +
                "COLD_MS=${ms(cold)} " +
                "P50_MS=${ms(percentile(samples, 50))} " +
                "P95_MS=${ms(percentile(samples, 95))} " +
                "MAX_MS=${ms(samples.max())} " +
                "SAMPLES=$repetitions",
        )
    }

    @Test
    fun measureAiSearchOnAndroidRuntime() {
        var state = State(List(16) { null }, 0)
        val positions = mutableListOf<State>()

        while (legalMoves(state).isNotEmpty()) {
            if (state.turn in setOf(1, 5, 7, 11, 15)) {
                positions += state
            }
            state = applyMove(state, legalMoves(state).first())
        }

        check(positions.size >= 3)

        for (position in positions) {
            val expected = strategy.chooseMove(position)
            check(expected in legalMoves(position))

            benchmark("SEARCH_TURN=${position.turn}") {
                check(strategy.chooseMove(position) == expected)
            }
        }

        report("SEARCH_BENCHMARK=PASS")
    }

    @Test
    fun measureControllerTapIncludingAiReply() {
        benchmark("CONTROLLER_HUMAN_PLUS_ANDROID_TURN") {
            val controller = GameController(GameMode.HUMAN_VS_ANDROID)
            controller.tap(0)

            check(controller.screen.game.turn == 2)
            check(controller.screen.game.cells.count { it == "north" } == 1)
            check(controller.screen.game.cells.count { it == "south" } == 1)
        }

        report("CONTROLLER_BENCHMARK=PASS")
    }
}
