package boardlab.tre_sigilli

sealed interface Move

data class Place(val cell: Int) : Move

data class Slide(val origin: Int, val destination: Int) : Move

data class State(val cells: List<String?>, val turn: Int)
