import {
  createManagementDocument,
  exportManagementJson,
  importManagementJson,
} from "./team-management-store.mjs";
import { validateManagerClub } from "./manager-profile.mjs";

export const MANAGER_SCHEMA_VERSION = 1;
export const MANAGER_STORAGE_KEY = "dado-goal.manager.v1";

function validateDocument(data) {
  if (!data || data.schemaVersion !== MANAGER_SCHEMA_VERSION) {
    throw new TypeError("unsupported manager schema");
  }

  validateManagerClub(data.club);

  if (!data.management || typeof data.management !== "object") {
    throw new TypeError("manager management state is required");
  }

  return data;
}

export function createManagerDocument({
  club,
  management = createManagementDocument(),
} = {}) {
  validateManagerClub(club);

  const validatedManagement = importManagementJson(
    exportManagementJson(management),
  );

  return {
    schemaVersion: MANAGER_SCHEMA_VERSION,
    club: structuredClone(club),
    management: structuredClone(validatedManagement),
  };
}

export function exportManagerJson(document) {
  validateDocument(document);

  const canonical = createManagerDocument({
    club: document.club,
    management: document.management,
  });

  return JSON.stringify(canonical, null, 2);
}

export function importManagerJson(source) {
  if (typeof source !== "string") {
    throw new TypeError("manager JSON must be a string");
  }

  let data;
  try {
    data = JSON.parse(source);
  } catch {
    throw new TypeError("invalid manager JSON");
  }

  validateDocument(data);

  return createManagerDocument({
    club: data.club,
    management: data.management,
  });
}

export function saveManager(storage, document) {
  if (!storage || typeof storage.setItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  storage.setItem(MANAGER_STORAGE_KEY, exportManagerJson(document));
}

export function loadManager(storage) {
  if (!storage || typeof storage.getItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  const source = storage.getItem(MANAGER_STORAGE_KEY);
  return source === null ? null : importManagerJson(source);
}

export function clearManager(storage) {
  if (!storage || typeof storage.removeItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  storage.removeItem(MANAGER_STORAGE_KEY);
}
