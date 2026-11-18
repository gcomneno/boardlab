export function nextStatisticsSort(current, column) {
  if (typeof column !== "string" || column.length === 0) {
    throw new TypeError("invalid statistics column");
  }

  if (current?.column !== column) {
    return { column, direction: "asc" };
  }

  if (current.direction === "asc") {
    return { column, direction: "desc" };
  }

  return null;
}

export function sortStatisticsRows(rows, sorting) {
  if (!Array.isArray(rows)) {
    throw new TypeError("statistics rows must be an array");
  }

  const result = [...rows];
  if (!sorting) return result;

  const { column, direction } = sorting;
  if (typeof column !== "string" ||
      !["asc", "desc"].includes(direction)) {
    throw new TypeError("invalid statistics sorting");
  }

  const factor = direction === "asc" ? 1 : -1;
  const collator = new Intl.Collator(undefined, {
    numeric: true,
    sensitivity: "base",
  });

  return result
    .map((row, index) => ({ row, index }))
    .sort((a, b) => {
      const left = a.row[column];
      const right = b.row[column];
      const leftMissing = left == null;
      const rightMissing = right == null;

      // Missing values stay last in either direction.
      if (leftMissing !== rightMissing) {
        return leftMissing ? 1 : -1;
      }

      let comparison = 0;
      if (!leftMissing) {
        comparison = typeof left === "number" &&
          typeof right === "number"
          ? Math.sign(left - right)
          : collator.compare(String(left), String(right));
      }

      return comparison * factor || a.index - b.index;
    })
    .map(({ row }) => row);
}
