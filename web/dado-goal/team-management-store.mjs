export const MANAGEMENT_SCHEMA_VERSION = 1;
export const MANAGEMENT_STORAGE_KEY = "dado-goal.team-management.v1";

function validateDocument(data) {
  if (!data || data.schemaVersion !== MANAGEMENT_SCHEMA_VERSION) {
    throw new TypeError("unsupported team-management schema");
  }

  if (!Array.isArray(data.ratings) || !Array.isArray(data.matches)) {
    throw new TypeError("invalid team-management document");
  }

  return data;
}

export function createManagementDocument({
  ratings = [],
  matches = [],
} = {}) {
  if (!Array.isArray(ratings) || !Array.isArray(matches)) {
    throw new TypeError("ratings and matches must be arrays");
  }

  return {
    schemaVersion: MANAGEMENT_SCHEMA_VERSION,
    ratings: structuredClone(ratings),
    matches: structuredClone(matches),
  };
}

export function exportManagementJson(document) {
  return JSON.stringify(validateDocument(document), null, 2);
}

export function importManagementJson(source) {
  if (typeof source !== "string") {
    throw new TypeError("team-management JSON must be a string");
  }

  let data;
  try {
    data = JSON.parse(source);
  } catch {
    throw new TypeError("invalid team-management JSON");
  }

  validateDocument(data);

  return createManagementDocument({
    ratings: data.ratings,
    matches: data.matches,
  });
}

export function saveManagement(storage, document) {
  if (!storage || typeof storage.setItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  storage.setItem(
    MANAGEMENT_STORAGE_KEY,
    exportManagementJson(document),
  );
}

export function loadManagement(storage) {
  if (!storage || typeof storage.getItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  const source = storage.getItem(MANAGEMENT_STORAGE_KEY);

  return source === null
    ? createManagementDocument()
    : importManagementJson(source);
}

export function clearManagement(storage) {
  if (!storage || typeof storage.removeItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  storage.removeItem(MANAGEMENT_STORAGE_KEY);
}
