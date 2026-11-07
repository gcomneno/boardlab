const TEAM_COLORS = Object.freeze({
  // Serie A 2024
  "487": ["#78B9E6", "#F4F4F4"], // Lazio
  "489": ["#D71920", "#111111"], // AC Milan
  "490": ["#174A8B", "#C8202F"], // Cagliari
  "492": ["#4AADE0"],            // Napoli
  "494": ["#111111", "#F4F4F4"], // Udinese
  "495": ["#C8202F", "#174A8B"], // Genoa
  "496": ["#F4F4F4", "#111111"], // Juventus
  "497": ["#F5A623", "#8E1F2D"], // AS Roma
  "499": ["#1E71B8", "#111111"], // Atalanta
  "500": ["#174A8B", "#B32032"], // Bologna
  "502": ["#5B2C83"],            // Fiorentina
  "503": ["#7A263A", "#8B2332"], // Torino
  "504": ["#F2C500", "#173F7A"], // Hellas Verona
  "505": ["#111111", "#0068A8"], // Inter
  "511": ["#1665A8"],            // Empoli
  "517": ["#177245", "#111111"], // Venezia
  "523": ["#1E4B9B", "#F4D000"], // Parma
  "867": ["#D71920", "#F5D000"], // Lecce
  "895": ["#2878B8"],            // Como
  "1579": ["#F4F4F4", "#D71920"], // Monza

  // Serie B 2024
  "488": ["#178B4B", "#111111"], // Sassuolo
  "498": ["#1769AA", "#D71920"], // Sampdoria
  "508": ["#D71920", "#F4F4F4"], // Bari
  "509": ["#111111", "#F4F4F4"], // Cesena
  "510": ["#B32032", "#F4D000"], // Cittadella
  "512": ["#174A8B", "#F4D000"], // Frosinone
  "514": ["#7A263A", "#F4F4F4"], // Salernitana
  "515": ["#F4F4F4", "#111111"], // Spezia
  "518": ["#1769AA", "#F4F4F4"], // Brescia
  "520": ["#D71920", "#B8B8B8"], // Cremonese
  "522": ["#E8A6BE", "#111111"], // Palermo
  "801": ["#174A8B", "#111111"], // Pisa
  "863": ["#F4D000", "#174A8B"], // Juve Stabia
  "880": ["#9E1B32", "#F4F4F4"], // Reggiana
  "899": ["#173F7A", "#F4D000"], // Modena
  "1578": ["#D71920", "#F4F4F4"], // Sudtirol
  "1581": ["#2878B8", "#F4D000"], // Carrarese
  "1687": ["#D71920", "#F4D000"], // Catanzaro
  "1693": ["#D71920", "#F4F4F4"], // Mantova
  "10137": ["#174A8B", "#D71920"], // Cosenza

  // Premier League 2024
  "33": ["#DA291C", "#FBE122"], // Manchester United
  "34": ["#111111", "#F4F4F4"], // Newcastle
  "35": ["#DA291C", "#111111"], // Bournemouth
  "36": ["#F4F4F4", "#111111"], // Fulham
  "39": ["#FDB913", "#111111"], // Wolves
  "40": ["#C8102E"],            // Liverpool
  "41": ["#D71920", "#F4F4F4"], // Southampton
  "42": ["#EF0107", "#F4F4F4"], // Arsenal
  "45": ["#003399", "#F4F4F4"], // Everton
  "46": ["#003090", "#F4F4F4"], // Leicester
  "47": ["#F4F4F4", "#132257"], // Tottenham
  "48": ["#7A263A", "#1BB1E7"], // West Ham
  "49": ["#034694", "#F4F4F4"], // Chelsea
  "50": ["#6CABDD", "#F4F4F4"], // Manchester City
  "51": ["#0057B8", "#F4F4F4"], // Brighton
  "52": ["#1B458F", "#C4122E"], // Crystal Palace
  "55": ["#D71920", "#F4F4F4"], // Brentford
  "57": ["#0057B8", "#F4F4F4"], // Ipswich
  "65": ["#DD0000", "#F4F4F4"], // Nottingham Forest
  "66": ["#670E36", "#95BFE5"], // Aston Villa

  // Championship 2024
  "38": ["#F4D000", "#D71920"], // Watford
  "43": ["#0057B8", "#F4F4F4"], // Cardiff
  "44": ["#6C1D45", "#99D6EA"], // Burnley
  "56": ["#D71920", "#F4F4F4"], // Bristol City
  "58": ["#173F7A", "#F4F4F4"], // Millwall
  "59": ["#F4F4F4", "#173F7A"], // Preston
  "60": ["#173F7A", "#F4F4F4"], // West Brom
  "62": ["#D71920", "#F4F4F4"], // Sheffield Utd
  "63": ["#F4F4F4", "#174A8B"], // Leeds
  "64": ["#F5A623", "#111111"], // Hull City
  "67": ["#1769AA", "#F4F4F4"], // Blackburn
  "69": ["#F4F4F4", "#111111"], // Derby
  "70": ["#D71920", "#F4F4F4"], // Middlesbrough
  "71": ["#F4D000", "#177245"], // Norwich
  "72": ["#1769AA", "#F4F4F4"], // QPR
  "74": ["#1769AA", "#F4F4F4"], // Sheffield Wednesday
  "75": ["#D71920", "#F4F4F4"], // Stoke City
  "76": ["#F4F4F4", "#111111"], // Swansea
  "746": ["#D71920", "#F4F4F4"], // Sunderland
  "1338": ["#F4D000", "#173F7A"], // Oxford United
  "1346": ["#78B9E6", "#F4F4F4"], // Coventry
  "1355": ["#174A8B", "#F4F4F4"], // Portsmouth
  "1357": ["#177245", "#F4F4F4"], // Plymouth
  "1359": ["#F58220", "#173F7A"], // Luton
});

export function socialColors(providerTeamId) {
  const colors = TEAM_COLORS[String(providerTeamId)];

  if (!colors) {
    throw new TypeError(
      `social colors missing for team ${providerTeamId}`
    );
  }

  return [...colors];
}
