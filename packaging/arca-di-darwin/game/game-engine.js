/* L'Arca di Darwin — motore statico per itch.io.
 * Riproduce le regole ecologiche del backend Python.
 * I rompicapi utilizzano soluzioni certificate durante l'esportazione.
 */
"use strict";

window.ArkEngine = (() => {
  let data = null;

  function initialize(gameData) {
    if (!gameData || !Array.isArray(gameData.species) ||
        !Array.isArray(gameData.scenarios) ||
        !Array.isArray(gameData.levels)) {
      throw new Error("Dati del gioco non validi");
    }
    data = gameData;
  }

  function evaluate(arrangement, scenarioId) {
    const environment = data.scenarios.find(s => s.id === scenarioId);
    const catalog = new Map(data.species.map(s => [s.name, s]));

    if (!environment || !Array.isArray(arrangement) ||
        arrangement.length !== environment.capacity ||
        arrangement.filter(name=>name!==null).length !==
          (scenarioId==="isole"?8:4) ||
        new Set(arrangement.filter(name=>name!==null)).size !==
          (scenarioId==="isole"?8:4)) {
      return {valid:false, message:"Occorrono quattro animali diversi."};
    }

    if (arrangement.some(name => name!==null && !catalog.has(name))) {
      return {valid:false, message:"Specie sconosciuta."};
    }

    const chosen = arrangement.filter(name=>name!==null).map(name=>catalog.get(name));
    const food = chosen.reduce((sum,s) => sum+s.food,0);
    const water = chosen.reduce((sum,s) => sum+s.water,0);
    const predators = chosen.filter(s => s.role==="predatore").length;
    const herbivores = chosen.filter(s => s.role==="erbivoro").length;
    const habitats = new Set(chosen.map(s => s.habitat)).size;
    const problems = [];

    // Le posizioni nei dati sono numerate a partire da 1.
    for (const n of environment.blocked_compartments || []) {
      if (arrangement[n-1] !== null) {
        problems.push("Il compartimento "+n+" deve restare libero");
      }
    }

    if (food > environment.food) problems.push("Cibo insufficiente");
    if (water > environment.water) problems.push("Acqua insufficiente");
    if (predators > herbivores)
      problems.push("Troppi predatori rispetto agli erbivori");

    // Capitolo 4: biodiversità minima obbligatoria.
    if (scenarioId==="foresta" && habitats<4)
      problems.push("Servono 4 habitat differenti");

    // Capitolo 5: la tartaruga deve essere imbarcata.
    if (scenarioId==="isole" && !arrangement.includes("tartaruga"))
      problems.push("La tartaruga deve essere imbarcata");

    const valid = problems.length===0;
    let score = valid
      ? 40 + 10*habitats
        + 10*chosen.filter(s=>s.habitat===environment.favored).length
        + 5*(environment.food-food)
        + 5*(environment.water-water)
      : 0;

    let message = valid
      ? "Ecosistema sostenibile! Le specie possono iniziare una nuova vita."
      : "L'ecosistema non è sostenibile: "+problems.join(", ");

    if (valid && scenarioId==="diluvio" && arrangement[0]!=="tartaruga") {
      score=0;
      message="Il primo compartimento è allagato! Qui può stare soltanto la tartaruga.";
      return {valid:false,score,food,water,problems,message,event:environment.event};
    }

    const incompatible = [
      ["leone","zebra"],
      ["lupo","coniglio"]
    ];

    if (valid) {
      for(let i=0;i<arrangement.length-1;i++) {
        if(incompatible.some(pair =>
          pair.includes(arrangement[i]) && pair.includes(arrangement[i+1]))) {
          score=0;
          message="Animali incompatibili in compartimenti vicini! L'Uccellino Azzurro consiglia di cambiare la disposizione.";
          return {valid:false,score,food,water,problems,message,event:environment.event};
        }
      }
      score+=20;
    }

    return {valid,score,food,water,problems,message,event:environment.event};
  }

  function checkLevel(difficulty, arrangement) {
    const level = data.levels.find(l=>l.difficulty===difficulty);
    if (!level || !Array.isArray(arrangement) ||
        arrangement.length!==4 ||
        new Set(arrangement).size!==4) {
      return {valid:false,message:"Non ancora! Rileggi gli indizi e riprova."};
    }

    const valid = arrangement.every((name,i)=>name===level.solution[i]);
    return {
      valid,
      message:valid
        ?"CIP CIP! Rompicapo risolto!"
        :"Non ancora! Rileggi gli indizi e riprova."
    };
  }

  return {initialize,evaluate,checkLevel};
})();
