import { getFootballCatalogue } from "./football-catalogue-d1.mjs";

function failure(status, code) {
  return Response.json(
    { error: { code } },
    {
      status,
      headers: {
        "Cache-Control": "no-store",
        "X-Content-Type-Options": "nosniff",
      },
    }
  );
}

export async function footballCatalogue(request, env) {
  const url = new URL(request.url);
  const competitionId = url.searchParams.get("competition");
  const season = url.searchParams.get("season");

  if (
    !competitionId ||
    !season ||
    !/^\d+$/.test(competitionId) ||
    !/^\d{4}$/.test(season)
  ) {
    return failure(400, "INVALID_CATALOGUE_QUERY");
  }

  try {
    const catalogue = await getFootballCatalogue(env.DB, {
      provider: "api-football",
      competitionId,
      season,
    });

    if (!catalogue) {
      return failure(404, "FOOTBALL_CATALOGUE_NOT_IMPORTED");
    }

    const headers = {
      "Cache-Control": "public, max-age=300",
      "X-Content-Type-Options": "nosniff",
    };

    if (request.headers.get("Origin") === "https://gcomneno.github.io") {
      headers["Access-Control-Allow-Origin"] = "https://gcomneno.github.io";
      headers.Vary = "Origin";
    }

    return Response.json(catalogue, { headers });
  } catch {
    return failure(500, "FOOTBALL_CATALOGUE_READ_FAILED");
  }
}
