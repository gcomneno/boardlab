package boardlab.android

import android.graphics.Bitmap
import android.util.Base64
import android.util.Log
import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performScrollTo
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import java.io.ByteArrayOutputStream
import java.security.MessageDigest

@RunWith(AndroidJUnit4::class)
class ScreenshotExportTest {
    @get:Rule
    val compose = createAndroidComposeRule<MainActivity>()

    private fun capture(name: String) {
        compose.waitForIdle()

        val bitmap = requireNotNull(
            InstrumentationRegistry.getInstrumentation()
                .uiAutomation.takeScreenshot()
        ) { "Android screenshot unavailable: $name" }

        val bytes = ByteArrayOutputStream().use { stream ->
            check(bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream)) {
                "PNG encoding failed: $name"
            }
            stream.toByteArray()
        }
        bitmap.recycle()

        check(bytes.size >= 1000) { "Unexpectedly small PNG: $name" }

        val digest = MessageDigest.getInstance("SHA-256")
            .digest(bytes)
            .joinToString("") { "%02x".format(it) }

        val encoded = Base64.encodeToString(bytes, Base64.NO_WRAP)
        val chunks = encoded.chunked(1500)

        Log.i(
            "BOARDLAB_SCREENSHOT",
            "BEGIN name=$name bytes=${bytes.size} " +
                "sha256=$digest chunks=${chunks.size}",
        )

        chunks.forEachIndexed { index, chunk ->
            Log.i(
                "BOARDLAB_SCREENSHOT",
                "CHUNK name=$name index=$index data=$chunk",
            )
        }

        Log.i("BOARDLAB_SCREENSHOT", "END name=$name")
    }

    @Test
    fun captureThreeGameplayScreens() {
        // 1. Initial local two-player game.
        compose.onNodeWithText("Modalità: Due giocatori")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithTag("board_cell_5").performScrollTo()
        capture("01-initial")

        // 2. Offline Android reply after one human move.
        compose.onNodeWithText("Opzioni").performScrollTo().performClick()
        compose.onNodeWithText("Contro Android").performClick()
        compose.onNodeWithTag("board_cell_0")
            .performScrollTo().performClick()
        compose.onNodeWithText("Turno 3 · Nord")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithTag("board_cell_5").performScrollTo()
        capture("02-offline-ai")

        // 3. Enter the maneuver phase in local mode.
        compose.onNodeWithText("Opzioni").performScrollTo().performClick()
        compose.onNodeWithText("Due giocatori").performClick()

        for (cell in 0..5) {
            compose.onNodeWithTag("board_cell_$cell")
                .performScrollTo().performClick()
        }

        compose.onNodeWithText("Turno 7 · Nord")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithText("Fase: manovra")
            .performScrollTo().assertIsDisplayed()
        compose.onNodeWithTag("board_cell_5").performScrollTo()
        capture("03-maneuver")

        Log.i("BOARDLAB_SCREENSHOT", "CAPTURE_GATE=PASS count=3")
    }
}
