const state = {
  coins: [], // { id, name, price, change24h, image, history, lastUpdated }
  lastPriceUpdate: 0,
  lastHistoryUpdate: 0,
};

const rowsEl = document.getElementById("rows");
const headerTitleEl = document.getElementById("header-title");
const syncTextEl = document.getElementById("sync-text");
const timerTextEl = document.getElementById("timer-text");
const statusTextEl = document.getElementById("status-text");

headerTitleEl.textContent = CONFIG.title;
document.documentElement.style.setProperty("--header-bg", CONFIG.ui_colors.header_bg);
document.documentElement.style.setProperty("--row-bg", CONFIG.ui_colors.row_bg);
document.documentElement.style.setProperty("--gold", CONFIG.ui_colors.gold);

function apiHeaders() {
  return CONFIG.api_key ? { "x-cg-demo-api-key": CONFIG.api_key } : {};
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

// Retry with backoff so a CoinGecko rate-limit (429) doesn't leave a chart
// blank until the next daily history refresh.
async function fetchWithRetry(fn, attempts = 3, baseDelayMs = 1500) {
  let lastErr;
  for (let i = 0; i < attempts; i++) {
    try {
      return await fn();
    } catch (e) {
      lastErr = e;
      if (i < attempts - 1) await sleep(baseDelayMs * (i + 1));
    }
  }
  throw lastErr;
}

async function fetchMarkets(ids) {
  const url = `https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=${ids.join(",")}`;
  const res = await fetch(url, { headers: apiHeaders() });
  if (!res.ok) throw new Error("markets fetch failed: " + res.status);
  return res.json();
}

async function fetchHistory(id) {
  const to = Math.floor(Date.now() / 1000);
  const from = to - 24 * 3600;
  const url = `https://api.coingecko.com/api/v3/coins/${id}/market_chart/range?vs_currency=usd&from=${from}&to=${to}`;
  const res = await fetch(url, { headers: apiHeaders() });
  if (!res.ok) throw new Error("history fetch failed: " + res.status);
  const data = await res.json();
  return (data.prices || []).map((p) => p[1]);
}

function buildRows() {
  rowsEl.innerHTML = "";
  state.coins.forEach((coin, i) => {
    const row = document.createElement("div");
    row.className = "coin-row";
    row.innerHTML = `
      <img class="coin-icon" src="${coin.image || ""}" onerror="this.style.visibility='hidden'">
      <div class="coin-name">${coin.name}</div>
      <div class="coin-price" id="price-${i}">$${coin.price.toFixed(2)}</div>
      <canvas class="coin-chart" id="chart-${i}"></canvas>
      <div class="coin-pct" id="pct-${i}"></div>
    `;
    rowsEl.appendChild(row);
  });
}

function drawChart(canvas, history, up) {
  const ctx = canvas.getContext("2d");
  const w = canvas.clientWidth;
  const h = canvas.clientHeight;
  canvas.width = w;
  canvas.height = h;
  ctx.clearRect(0, 0, w, h);
  if (!history || history.length < 2) return;
  const min = Math.min(...history);
  const max = Math.max(...history);
  const range = max - min || 1;
  ctx.strokeStyle = up ? "#39d353" : "#ff4136";
  ctx.lineWidth = 2;
  ctx.beginPath();
  history.forEach((v, i) => {
    const x = (i / (history.length - 1)) * w;
    const y = h - ((v - min) / range) * h;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();
}

function renderCoins() {
  state.coins.forEach((coin, i) => {
    const priceEl = document.getElementById(`price-${i}`);
    if (!priceEl) return;
    const pctEl = document.getElementById(`pct-${i}`);
    const chartEl = document.getElementById(`chart-${i}`);
    priceEl.textContent = `$${coin.price.toFixed(2)}`;
    const up = coin.change24h >= 0;
    pctEl.textContent = `${up ? "+" : ""}${coin.change24h.toFixed(2)}%`;
    pctEl.style.color = up ? "#39d353" : "#ff4136";
    drawChart(chartEl, coin.history, up);
  });
}

function setStatus(ok) {
  statusTextEl.textContent = ok ? "SYSTEM: STABLE" : "SYSTEM: UNSTABLE";
  statusTextEl.className = ok ? "stable" : "unstable";
}

async function refreshPrices() {
  try {
    const markets = await fetchWithRetry(() => fetchMarkets(CONFIG.coin_ids));
    const byId = Object.fromEntries(markets.map((m) => [m.id, m]));
    const firstRun = state.coins.length === 0;
    state.coins = CONFIG.coin_ids.map((id, i) => {
      const m = byId[id];
      const prev = state.coins[i];
      return {
        id,
        name: m ? m.name : id,
        price: m ? m.current_price : prev ? prev.price : 0,
        change24h: m ? m.price_change_percentage_24h : prev ? prev.change24h : 0,
        image: m ? m.image : prev ? prev.image : "",
        history: prev ? prev.history : [],
        lastUpdated: m ? m.last_updated : null,
      };
    });
    if (firstRun) buildRows();
    renderCoins();
    setStatus(true);
    if (state.coins[0] && state.coins[0].lastUpdated) {
      syncTextEl.textContent =
        "LAST SYNC: " + new Date(state.coins[0].lastUpdated).toLocaleTimeString();
    }
  } catch (e) {
    console.error(e);
    setStatus(false);
  }
}

async function refreshHistory() {
  for (let i = 0; i < state.coins.length; i++) {
    try {
      state.coins[i].history = await fetchWithRetry(() => fetchHistory(state.coins[i].id));
    } catch (e) {
      console.error(`history fetch failed for ${state.coins[i].id} after retries`, e);
    }
    // Space out requests to stay under the free-tier rate limit.
    if (i < state.coins.length - 1) await sleep(CONFIG.api_delay_ms || 1000);
  }
  renderCoins();
}

function tickTimer() {
  const elapsed = (Date.now() - state.lastPriceUpdate) / 1000;
  const left = Math.max(0, Math.round(CONFIG.update_interval_current_seconds - elapsed));
  const m = String(Math.floor(left / 60)).padStart(2, "0");
  const s = String(left % 60).padStart(2, "0");
  timerTextEl.textContent = `NEXT UPDATE IN: ${m}:${s}`;
}

async function mainLoop() {
  await refreshPrices();
  state.lastPriceUpdate = Date.now();
  await refreshHistory();
  state.lastHistoryUpdate = Date.now();

  setInterval(async () => {
    if ((Date.now() - state.lastHistoryUpdate) / 1000 >= CONFIG.update_interval_historic_seconds) {
      state.lastHistoryUpdate = Date.now();
      await refreshHistory();
    }
    if ((Date.now() - state.lastPriceUpdate) / 1000 >= CONFIG.update_interval_current_seconds) {
      state.lastPriceUpdate = Date.now();
      await refreshPrices();
    }
  }, 5000);

  setInterval(tickTimer, 1000);
}

mainLoop();
