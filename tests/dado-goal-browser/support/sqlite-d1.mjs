import { DatabaseSync, constants } from "node:sqlite";
import { performance } from "node:perf_hooks";

function integer(value) {
  if (typeof value !== "bigint") return value;
  return value >= BigInt(Number.MIN_SAFE_INTEGER) &&
    value <= BigInt(Number.MAX_SAFE_INTEGER) ? Number(value) : value;
}

function parameter(value) {
  if (typeof value === "boolean") return value ? 1n : 0n;
  if (typeof value === "number" && Number.isInteger(value)) {
    if (!Number.isSafeInteger(value)) {
      throw new RangeError("Bind large SQLite integers as bigint");
    }
    return BigInt(value);
  }
  if (value instanceof ArrayBuffer) return new Uint8Array(value.slice(0));
  if (ArrayBuffer.isView(value)) {
    return new Uint8Array(
      new Uint8Array(value.buffer, value.byteOffset, value.byteLength),
    );
  }
  if (value === null || ["string", "number", "bigint"].includes(typeof value)) {
    return value;
  }
  throw new TypeError("Unsupported SQLite bind value");
}

/**
 * Open an isolated D1-shaped test database using Node 24's built-in SQLite.
 * Defaults to memory; callers may supply a temporary database filename.
 * close() releases the connection but never deletes files or applies migrations.
 *
 * prepare() accepts one SQLite statement with positional parameters. bind()
 * returns a new statement. first(), all(), run() and batch() return promises;
 * close() is synchronous. SQL errors reject with the native SQLite error.
 *
 * Safe integers are numbers, larger integers remain bigint, and BLOBs remain
 * Uint8Array values. The latter two are intentional local extensions rather
 * than D1's JSON transport. Only measurable metadata (changes, last_row_id and
 * local duration in milliseconds) is exposed. D1 service timings, row scan
 * counts, replication, sessions and distributed concurrency are not simulated.
 */
export function openSqliteD1(filename = ":memory:") {
  const sqlite = new DatabaseSync(filename, { enableForeignKeyConstraints: true });
  const statements = new WeakMap();
  let transactionControl = false;

  // Keep transaction boundaries under batch()'s control, including SQL with
  // comments or whitespace before a transaction command.
  sqlite.setAuthorizer(action => {
    if (!transactionControl &&
        (action === constants.SQLITE_TRANSACTION ||
         action === constants.SQLITE_SAVEPOINT)) {
      return constants.SQLITE_DENY;
    }
    return constants.SQLITE_OK;
  });

  function transaction(sql) {
    transactionControl = true;
    try {
      sqlite.exec(sql);
    } finally {
      transactionControl = false;
    }
  }

  function execute(statement) {
    const definition = statements.get(statement);
    if (!definition) throw new TypeError("Statement belongs to another database");

    const started = performance.now();
    const before = sqlite.prepare("SELECT total_changes() AS total");
    before.setReadBigInts(true);
    const previousTotal = before.get().total;

    // Compile here so a batch may create a table before using it, and a SQL
    // error in a later statement rolls back earlier statements in that batch.
    const native = sqlite.prepare(definition.sql);
    native.setReadBigInts(true);
    const results = native.all(...definition.values).map(row =>
      Object.fromEntries(Object.entries(row).map(([key, value]) =>
        [key, integer(value)])),
    );
    const counters = sqlite.prepare(`
      SELECT total_changes() AS total, changes() AS changes,
             last_insert_rowid() AS last_row_id
    `);
    counters.setReadBigInts(true);
    const after = counters.get();

    // SELECT leaves changes() untouched. Use total_changes() to distinguish
    // reads from writes without resetting the value needed by the next SQL.
    return {
      success: true,
      results,
      meta: {
        changes: integer(after.total === previousTotal ? 0n : after.changes),
        last_row_id: integer(after.last_row_id),
        duration: performance.now() - started,
      },
    };
  }

  function prepared(sql, values = []) {
    if (typeof sql !== "string" || sql.trim().length === 0) {
      throw new TypeError("SQL statement required");
    }
    const statement = Object.freeze({
      bind(...parameters) {
        return prepared(sql, parameters.map(parameter));
      },
      async first(column) {
        const row = execute(statement).results[0] ?? null;
        if (row === null || column === undefined) return row;
        if (!Object.hasOwn(row, column)) {
          throw new RangeError(`Unknown result column: ${column}`);
        }
        return row[column];
      },
      async all() {
        return execute(statement);
      },
      async run() {
        return execute(statement);
      },
    });
    statements.set(statement, { sql, values });
    return statement;
  }

  return Object.freeze({
    prepare: prepared,
    async batch(batchStatements) {
      if (!Array.isArray(batchStatements) ||
          batchStatements.some(statement => !statements.has(statement))) {
        throw new TypeError("Batch requires statements from this database");
      }
      transaction("BEGIN IMMEDIATE");
      try {
        // No awaits: other calls cannot interleave on this connection.
        const results = batchStatements.map(execute);
        transaction("COMMIT");
        return results;
      } catch (error) {
        // ON CONFLICT ROLLBACK may already have ended the transaction.
        if (sqlite.isTransaction) transaction("ROLLBACK");
        throw error;
      }
    },
    close() {
      sqlite.close();
    },
  });
}
